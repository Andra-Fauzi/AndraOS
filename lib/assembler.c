#include "assembler.h"
#include "util.h"

// x86-32 register encoding and names
typedef struct {
    char *name;
    uint8_t code;
} Register;

static Register registers[] = {
    {"eax", 0},
    {"ecx", 1},
    {"edx", 2},
    {"ebx", 3},
    {"esp", 4},
    {"ebp", 5},
    {"esi", 6},
    {"edi", 7},
    {NULL, 0}
};

// Operand types
typedef enum {
    OP_NONE,
    OP_REG,      // register
    OP_IMM,      // immediate value
    OP_MEM,      // memory [reg + offset]
} OperandType;

typedef struct {
    OperandType type;
    int reg;     // register code (0-7)
    int value;   // immediate or offset
} Operand;

// Instruction encoding helper
typedef struct {
    uint8_t opcode;
    uint8_t modrm_byte;  // Combined encoding
    int has_immediate;
    int immediate;
} EncodedInst;

// ELF32 header structure
typedef struct {
    uint8_t e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} ELF32_Header;

// Program header
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} ELF32_ProgramHeader;

// Simple helper to write little-endian integers
static void write_u32_le(uint8_t *buf, uint32_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
    buf[2] = (val >> 16) & 0xFF;
    buf[3] = (val >> 24) & 0xFF;
}

static void write_u16_le(uint8_t *buf, uint16_t val) {
    buf[0] = val & 0xFF;
    buf[1] = (val >> 8) & 0xFF;
}

// Get register code from name
static int get_register_code(const char *name, size_t len) {
    for (int i = 0; registers[i].name; i++) {
        const char *r = registers[i].name;
        size_t rlen = 0;
        while (r[rlen]) rlen++;
        if (len == rlen && strncmp(name, r, len) == 0) {
            return registers[i].code;
        }
    }
    return -1;
}

// Trim whitespace
static const char *trim_left(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static const char *trim_right(const char *s, const char *end) {
    while (end > s && (*(end-1) == ' ' || *(end-1) == '\t')) end--;
    return end;
}

// Parse operand (register or immediate)
static int parse_operand(const char *op_str, size_t op_len, Operand *op) {
    op_str = trim_left(op_str);
    const char *op_end = op_str + op_len;
    op_end = trim_right(op_str, op_end);
    op_len = op_end - op_str;
    
    if (op_len == 0) {
        op->type = OP_NONE;
        return 0;
    }
    
    // Check if register
    if (op_str[0] == '%') {
        int reg_code = get_register_code(op_str + 1, op_len - 1);
        if (reg_code >= 0) {
            op->type = OP_REG;
            op->reg = reg_code;
            return 1;
        }
    }
    
    // Check if immediate (starts with $)
    if (op_str[0] == '$') {
        op->type = OP_IMM;
        op->value = 0;
        for (const char *p = op_str + 1; p < op_end; p++) {
            if (*p >= '0' && *p <= '9') {
                op->value = op->value * 10 + (*p - '0');
            }
        }
        return 1;
    }
    
    // Check if memory reference like (%ebp) or 4(%ebp)
    if (op_str[0] == '(') {
        // Parse register inside parentheses
        int paren_end = 0;
        while (paren_end < (int)op_len && op_str[paren_end] != ')') paren_end++;
        int reg_code = get_register_code(op_str + 1, paren_end - 1);
        if (reg_code >= 0) {
            op->type = OP_MEM;
            op->reg = reg_code;
            op->value = 0;
            return 1;
        }
    } else if (op_len > 1 && op_str[op_len-1] == ')') {
        // Parse offset(%register)
        int paren_pos = 0;
        while (paren_pos < (int)op_len && op_str[paren_pos] != '(') paren_pos++;
        if (paren_pos < (int)op_len) {
            // Parse offset
            op->value = 0;
            int neg = 0;
            const char *num_start = op_str;
            if (op_str[0] == '-') {
                neg = 1;
                num_start++;
            }
            for (const char *p = num_start; p < op_str + paren_pos; p++) {
                if (*p >= '0' && *p <= '9') {
                    op->value = op->value * 10 + (*p - '0');
                }
            }
            if (neg) op->value = -op->value;
            
            // Parse register
            int reg_code = get_register_code(op_str + paren_pos + 1, op_len - paren_pos - 2);
            if (reg_code >= 0) {
                op->type = OP_MEM;
                op->reg = reg_code;
                return 1;
            }
        }
    }
    
    return 0;
}

// Encode x86 instruction
static size_t encode_instruction(const char *mnem, size_t mnem_len, 
                                 Operand *op1, Operand *op2,
                                 uint8_t *output, size_t output_max) {
    if (output_max == 0) return 0;
    
    size_t bytes = 0;
    
    // NOP
    if (mnem_len == 3 && strncmp(mnem, "nop", 3) == 0) {
        output[bytes++] = 0x90;
        return bytes;
    }
    
    // RET
    if (mnem_len == 3 && strncmp(mnem, "ret", 3) == 0) {
        output[bytes++] = 0xC3;
        return bytes;
    }
    
    // PUSH reg  (0x50 + reg)
    if (mnem_len == 4 && strncmp(mnem, "push", 4) == 0) {
        if (op1->type == OP_REG) {
            output[bytes++] = 0x50 + op1->reg;
            return bytes;
        }
        // PUSH imm32: 0x68
        if (op1->type == OP_IMM && bytes + 5 <= output_max) {
            output[bytes++] = 0x68;
            output[bytes++] = op1->value & 0xFF;
            output[bytes++] = (op1->value >> 8) & 0xFF;
            output[bytes++] = (op1->value >> 16) & 0xFF;
            output[bytes++] = (op1->value >> 24) & 0xFF;
            return bytes;
        }
    }
    
    // POP reg  (0x58 + reg)
    if (mnem_len == 3 && strncmp(mnem, "pop", 3) == 0) {
        if (op1->type == OP_REG) {
            output[bytes++] = 0x58 + op1->reg;
            return bytes;
        }
    }
    
    // MOV instructions
    if (mnem_len == 3 && strncmp(mnem, "mov", 3) == 0) {
        if (bytes + 2 > output_max) return 0;
        
        // mov reg32, reg32 (0x89 /r)
        if (op1->type == OP_REG && op2->type == OP_REG) {
            output[bytes++] = 0x89;
            output[bytes++] = 0xC0 | (op2->reg << 3) | op1->reg;
            return bytes;
        }
        
        // mov imm32, reg32 (0xB8 + reg)
        if (op1->type == OP_IMM && op2->type == OP_REG && bytes + 5 <= output_max) {
            output[bytes++] = 0xB8 + op2->reg;
            output[bytes++] = op1->value & 0xFF;
            output[bytes++] = (op1->value >> 8) & 0xFF;
            output[bytes++] = (op1->value >> 16) & 0xFF;
            output[bytes++] = (op1->value >> 24) & 0xFF;
            return bytes;
        }
        
        // mov [reg], reg32 (0x8B /r)
        if (op1->type == OP_MEM && op2->type == OP_REG && bytes + 2 <= output_max) {
            output[bytes++] = 0x8B;
            output[bytes++] = 0x00 | (op2->reg << 3) | op1->reg;
            return bytes;
        }
        
        // mov reg32, [reg] (0x89 /r)
        if (op1->type == OP_REG && op2->type == OP_MEM && bytes + 2 <= output_max) {
            output[bytes++] = 0x89;
            output[bytes++] = 0x00 | (op1->reg << 3) | op2->reg;
            return bytes;
        }
    }
    
    // SUB imm32, reg32  (0x83 0xEC imm8)
    if (mnem_len == 3 && strncmp(mnem, "sub", 3) == 0) {
        if (op1->type == OP_IMM && op2->type == OP_REG && bytes + 3 <= output_max) {
            output[bytes++] = 0x83;
            output[bytes++] = 0xE8 | op2->reg;  // 0xEC for esp
            output[bytes++] = op1->value & 0xFF;
            return bytes;
        }
    }
    
    // ADD instructions
    if (mnem_len == 3 && strncmp(mnem, "add", 3) == 0) {
        // add reg, reg
        if (op1->type == OP_REG && op2->type == OP_REG && bytes + 2 <= output_max) {
            output[bytes++] = 0x01;
            output[bytes++] = 0xC0 | (op2->reg << 3) | op1->reg;
            return bytes;
        }
    }
    
    // INT imm8 (0xCD imm)
    if (mnem_len == 3 && strncmp(mnem, "int", 3) == 0) {
        if (op1->type == OP_IMM && bytes + 2 <= output_max) {
            output[bytes++] = 0xCD;
            output[bytes++] = op1->value & 0xFF;
            return bytes;
        }
    }
    
    // Default: encode as NOP to prevent errors
    output[bytes++] = 0x90;
    return bytes;
}

// Generate minimal ELF32 executable
static void generate_elf_header(uint8_t *buf, uint32_t code_size) {
    ELF32_Header header;
    
    // ELF magic number
    header.e_ident[0] = 0x7F;
    header.e_ident[1] = 'E';
    header.e_ident[2] = 'L';
    header.e_ident[3] = 'F';
    header.e_ident[4] = 1;  // 32-bit
    header.e_ident[5] = 1;  // Little-endian
    header.e_ident[6] = 1;  // Current version
    header.e_ident[7] = 0;  // UNIX System V ABI
    header.e_ident[8] = 0;
    for (int i = 9; i < 16; i++) {
        header.e_ident[i] = 0;
    }
    
    header.e_type = 2;           // ET_EXEC
    header.e_machine = 3;        // EM_386
    header.e_version = 1;
    header.e_entry = 0x08048000; // Entry point
    header.e_phoff = 52;         // Program header offset
    header.e_shoff = 0;          // Section header offset (not used)
    header.e_flags = 0;
    header.e_ehsize = 52;        // ELF header size
    header.e_phentsize = 32;     // Program header size
    header.e_phnum = 1;          // Number of program headers
    header.e_shentsize = 0;
    header.e_shnum = 0;
    header.e_shstrndx = 0;
    
    // Write ELF header
    memcpy(buf, header.e_ident, 16);
    write_u16_le(buf + 16, header.e_type);
    write_u16_le(buf + 18, header.e_machine);
    write_u32_le(buf + 20, header.e_version);
    write_u32_le(buf + 24, header.e_entry);
    write_u32_le(buf + 28, header.e_phoff);
    write_u32_le(buf + 32, header.e_shoff);
    write_u32_le(buf + 36, header.e_flags);
    write_u16_le(buf + 40, header.e_ehsize);
    write_u16_le(buf + 42, header.e_phentsize);
    write_u16_le(buf + 44, header.e_phnum);
    write_u16_le(buf + 46, header.e_shentsize);
    write_u16_le(buf + 48, header.e_shnum);
    write_u16_le(buf + 50, header.e_shstrndx);
    
    // Generate program header at offset 52
    uint8_t *phdr = buf + 52;
    uint32_t p_type = 1;     // PT_LOAD
    uint32_t p_offset = 84;  // After ELF + program header
    uint32_t p_vaddr = 0x08048000;
    uint32_t p_paddr = 0x08048000;
    uint32_t p_filesz = 84 + code_size;
    uint32_t p_memsz = 84 + code_size;
    uint32_t p_flags = 5;    // PF_R | PF_X (read + execute)
    uint32_t p_align = 0x1000;
    
    write_u32_le(phdr + 0, p_type);
    write_u32_le(phdr + 4, p_offset);
    write_u32_le(phdr + 8, p_vaddr);
    write_u32_le(phdr + 12, p_paddr);
    write_u32_le(phdr + 16, p_filesz);
    write_u32_le(phdr + 20, p_memsz);
    write_u32_le(phdr + 24, p_flags);
    write_u32_le(phdr + 28, p_align);
}

// Main assembly function
int assemble_x86(const char *asm_text, size_t asm_len, 
                 char *output, size_t output_max, size_t *output_len) {
    if (!asm_text || !output || output_max < 100) {
        if (output_len) *output_len = 0;
        return 1;
    }
    
    // ELF header is 84 bytes (52 byte ELF header + 32 byte program header)
    const size_t ELF_HEADER_SIZE = 84;
    
    if (output_max < ELF_HEADER_SIZE + 4) {
        if (output_len) *output_len = 0;
        return 1;
    }
    
    // Generate ELF header
    generate_elf_header((uint8_t *)output, 4);
    
    // Assemble the code into the space after headers
    uint8_t *code_start = (uint8_t *)output + ELF_HEADER_SIZE;
    size_t code_space = output_max - ELF_HEADER_SIZE;
    size_t code_size = 0;
    
    // Parse assembly line by line
    const char *line_start = asm_text;
    while (line_start < asm_text + asm_len && code_size < code_space) {
        const char *line_end = line_start;
        while (line_end < asm_text + asm_len && *line_end != '\n') {
            line_end++;
        }
        
        size_t line_len = line_end - line_start;
        const char *p = trim_left(line_start);
        size_t remaining = line_len - (p - line_start);
        
        // Skip empty lines and comments
        if (remaining > 0 && *p != '\0' && *p != ';' && *p != '#') {
            // Find mnemonic
            const char *mnem_start = p;
            while (p < line_end && *p != ' ' && *p != '\t' && *p != '\n') p++;
            size_t mnem_len = p - mnem_start;
            
            // Parse operands
            Operand op1 = {OP_NONE}, op2 = {OP_NONE};
            
            // Find operands separated by comma
            if (p < line_end && *p != '\n') {
                const char *op_start = p;
                int comma_pos = -1;
                for (const char *q = op_start; q < line_end && *q != '\n'; q++) {
                    if (*q == ',') {
                        comma_pos = q - op_start;
                        break;
                    }
                }
                
                if (comma_pos > 0) {
                    // Two operands
                    parse_operand(op_start, comma_pos, &op1);
                    parse_operand(op_start + comma_pos + 1, (line_end - op_start) - comma_pos - 1, &op2);
                } else {
                    // One operand
                    parse_operand(op_start, line_end - op_start, &op1);
                }
            }
            
            // Encode instruction
            size_t bytes = encode_instruction(mnem_start, mnem_len, &op1, &op2, 
                                             code_start + code_size, code_space - code_size);
            code_size += bytes;
        }
        
        line_start = line_end + 1;
    }
    
    // Add final RET instruction if not present
    if (code_size < code_space) {
        code_start[code_size++] = 0xC3; // RET
    }
    
    // Update ELF header with actual code size
    generate_elf_header((uint8_t *)output, code_size);
    
    if (output_len) *output_len = ELF_HEADER_SIZE + code_size;
    
    return 0;
}
