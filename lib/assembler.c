#include "assembler.h"
#include "util.h"

// Simple x86 instruction encoder
typedef struct {
    char *mnemonic;
    uint8_t opcode;
    int args;  // Number of arguments
} Instruction;

static Instruction instructions[] = {
    {"ret", 0xC3, 0},
    {"nop", 0x90, 0},
    {"push", 0x50, 1},
    {"pop", 0x58, 1},
    {"mov", 0x89, 2},
    {"add", 0x01, 2},
    {"sub", 0x29, 2},
    {"xor", 0x31, 2},
    {"and", 0x21, 2},
    {"or", 0x09, 2},
    {"cmp", 0x39, 2},
    {"je", 0x74, 1},
    {"jne", 0x75, 1},
    {"jmp", 0xEB, 1},
    {"call", 0xE8, 1},
    {"lea", 0x8D, 2},
    {NULL, 0, 0}
};

// x86 register encoding
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
    {"rax", 0},
    {"rcx", 1},
    {"rdx", 2},
    {"rbx", 3},
    {"rsp", 4},
    {"rbp", 5},
    {"rsi", 6},
    {"rdi", 7},
    {NULL, 0}
};

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

// Trim whitespace from left
static const char *trim_left(const char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

// Parse a single line of assembly
static size_t assemble_line(const char *line, size_t line_len, uint8_t *output, size_t output_max) {
    const char *p = trim_left(line);
    size_t remaining = line_len - (p - line);
    
    // Skip empty lines and comments
    if (remaining == 0 || *p == '\0' || *p == ';' || *p == '#') {
        return 0;
    }
    
    // Find mnemonic
    const char *mnem_start = p;
    while (p < line + line_len && *p != ' ' && *p != '\t' && *p != '\n') p++;
    size_t mnem_len = p - mnem_start;
    
    // Find instruction
    Instruction *instr = NULL;
    for (int i = 0; instructions[i].mnemonic; i++) {
        const char *m = instructions[i].mnemonic;
        size_t mlen = 0;
        while (m[mlen]) mlen++;
        if (mnem_len == mlen && strncmp(mnem_start, m, mlen) == 0) {
            instr = &instructions[i];
            break;
        }
    }
    
    if (!instr) {
        // Unknown instruction, skip it (or output NOP)
        if (output_max > 0) {
            output[0] = 0x90; // NOP
            return 1;
        }
        return 0;
    }
    
    // For now, simple encoding: just output the opcode + NOPs for padding
    if (output_max > 0) {
        output[0] = instr->opcode;
        // Add ModR/M byte for 2-operand instructions
        if (instr->args == 2 && output_max > 1) {
            output[1] = 0xC0; // ModR/M: register to register
            return 2;
        } else if (instr->args == 1 && output_max > 1) {
            output[1] = 0x00; // Simple operand byte
            return 2;
        }
        return 1;
    }
    
    return 0;
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
        size_t bytes = assemble_line(line_start, line_len, code_start + code_size, code_space - code_size);
        code_size += bytes;
        
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
