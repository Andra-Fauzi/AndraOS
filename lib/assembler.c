#include "assembler.h"
#include "util.h"
#include <stdint.h>
#include <stdbool.h>

extern multiboot_info_t *multiboot_info;

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------

#define ELF_BASE 0X4048000
#define ELF_HDRSZ 84
#define CODE_BASE (ELF_BASE + ELF_HDRSZ)

#define MAX_LABELS 512

typedef struct {
    char name[64];
    uint32_t address;
    bool defined;
} AsmLabel;

static AsmLabel labels[MAX_LABELS];
static int label_count = 0;


static void reset_labels() {
    label_count = 0;
}

static AsmLabel *find_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        #ifdef DEBUG
        kprint("\n", multiboot_info);
        kprint("ini label yang di cari:", multiboot_info);
        kprint(name, multiboot_info);
        kprint("\n", multiboot_info);
        kprint("ini label yang ditemukan:", multiboot_info);
        kprint(labels[i].name, multiboot_info);
        kprint("\n", multiboot_info);
        #endif
        if (strcmp(labels[i].name, name) == 0) {
            #ifdef DEBUG
            kprint("ketemu\n", multiboot_info);
            #endif
            return &labels[i];
        }
    }
    #ifdef DEBUG
    kprint("tidak ketemu\n", multiboot_info);
    #endif
    return NULL;
}

static AsmLabel *add_label(const char *name) {
    AsmLabel *l = find_label(name);
    if (l) return l;
    if (label_count >= MAX_LABELS) return NULL;
    l = &labels[label_count++];

    strncpy(l->name, name, 63);
    l->name[63] = '\0';
    l->defined = false;
    l->address = 0;
    return l;
}

// ELF Structures
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

// x86 Registers
typedef struct {
    const char *name;
    uint8_t code;
    uint8_t size; // 1=8bit, 2=16bit, 4=32bit
} Register;

static Register registers[] = {
    {"al", 0, 1}, {"cl", 1, 1}, {"dl", 2, 1}, {"bl", 3, 1},
    {"ah", 4, 1}, {"ch", 5, 1}, {"dh", 6, 1}, {"bh", 7, 1},
    {"ax", 0, 2}, {"cx", 1, 2}, {"dx", 2, 2}, {"bx", 3, 2},
    {"sp", 4, 2}, {"bp", 5, 2}, {"si", 6, 2}, {"di", 7, 2},
    {"eax", 0, 4}, {"ecx", 1, 4}, {"edx", 2, 4}, {"ebx", 3, 4},
    {"esp", 4, 4}, {"ebp", 5, 4}, {"esi", 6, 4}, {"edi", 7, 4},
    {"dil", 7, 1}, {"sil", 6, 1}, {"bpl", 5, 1}, {"spl", 4, 1},
    {"xmm0", 0, 16}, {"xmm1", 1, 16}, {"xmm2", 2, 16}, {"xmm3", 3, 16},
    {"xmm4", 4, 16}, {"xmm5", 5, 16}, {"xmm6", 6, 16}, {"xmm7", 7, 16},
    {"st(0)", 0, 10},
    {NULL, 0, 0}
};

static int get_register(const char *name, int *code, int *size) {
    for (int i = 0; registers[i].name; i++) {
        if (strcasecmp(registers[i].name, name) == 0) {
            if (code) *code = registers[i].code;
            if (size) *size = registers[i].size;
            return 1;
        }
    }
    return 0;
}

// Operand Parsing
typedef enum {
    OP_NONE,
    OP_REG,
    OP_IMM,
    OP_MEM,
    OP_LABEL
} OperandType;

typedef struct {
    OperandType type;
    int reg;
    int reg_size;
    int32_t offset;
    int base_reg;
    char label_name[64];
    bool indirect;
} Operand;

// -----------------------------------------------------------------------------
// Parsing Helpers
// -----------------------------------------------------------------------------

static const char *skip_whitespace(const char *p) {
    while (*p && (*p == ' ' || *p == '\t')) p++;
    return p;
}

static const char *parse_token(const char *p, char *buf, int max_len) {
    p = skip_whitespace(p);
    if (!*p) return NULL;
    
    int i = 0;
    while (*p && !isspace(*p) && *p != ',' && *p != '\n' && *p != ':') {
        if (i < max_len - 1) buf[i++] = *p;
        p++;
    }
    buf[i] = '\0';
    return p;
}

static int parse_operand_str(const char *text, Operand *op) {
    char buf[128];
    const char *p = text;
    memset(op, 0, sizeof(Operand));
    op->type = OP_NONE;
    op->base_reg = -1;
    
    p = skip_whitespace(p);
    if (!*p) return 0;
    
    if (*p == '$') {
        op->type = OP_IMM;
        op->offset = (int32_t)strtoul(p + 1, NULL, 0);
        return 1;
    }

    if (*p == '*') {
        op->indirect = true;
        p++;
    }

    // Character literal: 'a'
    if (*p == '\'') {
        op->type = OP_IMM;
        if (*(p+1) && *(p+2) == '\'') {
            op->offset = (int32_t)*(p+1);
            return 1;
        }
    }
    
    // Check for register
    if (*p == '%') {
        const char *end = p + 1;
        while (isalnum(*end) || *end == '(' || *end == ')') end++; // xmm0, st(0)
        int len = end - (p + 1);
        strncpy(buf, p + 1, len);
        buf[len] = '\0';
        if (get_register(buf, &op->reg, &op->reg_size)) {
            op->type = OP_REG;
            return 1;
        }
        return 0;
    }
    
    
    // Memory: disp(%reg), (%reg), disp, label
    const char *lparen = strchr(p, '(');
    if (*p == '(' || (strchr(p, '(') && strchr(p, '(') < strpbrk(p, " \t\n,"))) {
        op->type = OP_MEM;
        if (lparen > p) {
            strncpy(buf, p, lparen - p);
            buf[lparen-p] = '\0';
            if (isdigit(buf[0]) || buf[0] == '-') {
                op->offset = (int32_t)strtol(buf, NULL, 0);
            } else {
                strncpy(op->label_name, buf, 63);
            }
        }
        const char *rstart = lparen + 1;
        if (*rstart == '%') {
            const char *rend = rstart + 1;
            while (isalnum(*rend)) rend++;
            char regname[16];
            int rlen = rend - (rstart + 1);
            strncpy(regname, rstart + 1, rlen);
            regname[rlen] = '\0';
            int dummy;
            if (!get_register(regname, &op->base_reg, &dummy)) return 0;
        }
        return 1;
    }
    
    if (isalpha(*p) || *p == '_' || *p == '.') {
        const char *end = p;
        while (isalnum(*end) || *end == '_' || *end == '.') end++;
        int len = end - p;
        strncpy(op->label_name, p, len);
        op->label_name[len] = '\0';
        op->type = OP_LABEL;
        return 1;
    }
    
    // Bare number - treat as immediate for convenience (e.g. mov 1, %eax)
    if (isdigit(*p) || (*p == '-' && isdigit(*(p+1)))) {
        op->type = OP_IMM;
        op->offset = (int32_t)strtoul(p, NULL, 0);
        return 1;
    }
    
    return 0;
}

// -----------------------------------------------------------------------------
// Encoding Helpers
// -----------------------------------------------------------------------------

static void emit_u8(uint8_t **buf, uint8_t val) {
    if (*buf) *(*buf)++ = val;
}

static void emit_u32(uint8_t **buf, uint32_t val) {
    emit_u8(buf, val & 0xFF);
    emit_u8(buf, (val >> 8) & 0xFF);
    emit_u8(buf, (val >> 16) & 0xFF);
    emit_u8(buf, (val >> 24) & 0xFF);
}

static void emit_modrm(uint8_t **buf, int mod, int reg, int rm) {
    emit_u8(buf, (mod << 6) | ((reg & 7) << 3) | (rm & 7));
}

// Emits Insn + ModRM.
// Opcode can be 1 or 2 bytes. If opcode > 0xFF, assumes 2 bytes (big endian format in int? e.g. 0x0F85).
static void emit_insn_modrm(uint8_t **buf, int opcode, Operand *op_reg, Operand *op_mem) {
    int mod = 0, rm = 0;
    int32_t disp = op_mem->offset;
    
    if (opcode > 0xFF) {
        emit_u8(buf, (opcode >> 8) & 0xFF);
        emit_u8(buf, opcode & 0xFF);
    } else {
        emit_u8(buf, opcode);
    }
    
    if (op_mem->type == OP_REG) {
        mod = 3;
        rm = op_mem->reg;
    } else if (op_mem->type == OP_MEM) {
        if (op_mem->base_reg == -1) {
            mod = 0; rm = 5; 
        } else {
            rm = op_mem->base_reg;
             if (disp == 0 && rm != 5) {
                mod = 0;
            } else if (disp >= -128 && disp <= 127) {
                mod = 1;
            } else {
                mod = 2;
            }
        }
    }
    
    int reg_field = (op_reg) ? op_reg->reg : 0; // If op_reg is NULL, reg field is 0 (or use specific extended opcode logic)
    
    emit_modrm(buf, mod, reg_field, rm);
    
    if (mod == 1) emit_u8(buf, (int8_t)disp);
    else if (mod == 2 || (mod == 0 && rm == 5)) emit_u32(buf, disp);
}

// -----------------------------------------------------------------------------
// Assembler Logic
// -----------------------------------------------------------------------------

int process_instruction(const char *line, uint8_t **buf, uint32_t current_addr, int pass) {
    int size = 0;
    #define EMIT_U8(x)   do { if (buf) emit_u8(buf, x); size++; } while(0)
    #define EMIT_U32(x) do { if (buf) emit_u32(buf, x); size += 4; } while(0)
    #define EMIT_MODRM(mod, reg, rm) \
    do { if (buf) emit_modrm(buf, mod, reg, rm); size++; } while(0)
    #define EMIT_INSN_MODRM(opcode, op_reg, op_rm) do { \
    emit_insn_modrm(buf, opcode, op_reg, op_rm); \
    if (pass == 1) { \
        size += 1; \
        if ((op_rm)->type == OP_MEM) { \
            if ((op_rm)->offset != 0 || (op_rm)->base_reg == 5) size += 4; \
        } \
    } \
} while(0)
    


    char mnemonic[32];
    const char *p = parse_token(line, mnemonic, 32);
    if (!p) return 0;
    
    if (*p == ':') {
        if (pass == 1) {
            AsmLabel *l = add_label(mnemonic);
            if (l) {
                l->address = current_addr;
                l->defined = true;
                char buf1[128];
                to_string(current_addr, buf1);
                kprint("Defined label: ", multiboot_info);
                kprint(mnemonic, multiboot_info);
                kprint(" at ", multiboot_info);
                kprint(buf1, multiboot_info);
                kprint(" (hex: ", multiboot_info);
                kprint_hex(current_addr, multiboot_info);
                kprint(")\n", multiboot_info);
            }
        }
        return 0;
    }
    
    Operand op1 = {0}, op2 = {0};
    int op_count = 0;
    
    p = skip_whitespace(p);
    if (*p) {
        const char *comma = strchr(p, ',');
        if (comma) {
            char s1[64];
            int len1 = comma - p;
            strncpy(s1, p, len1); s1[len1] = '\0';
            parse_operand_str(s1, &op1);
            parse_operand_str(comma + 1, &op2);
            op_count = 2;
        } else {
            parse_operand_str(p, &op1);
            op_count = 1;
        }
    }
    
    uint8_t *start_buf = buf ? *buf : NULL;
    
    if (strcasecmp(mnemonic, "nop") == 0) EMIT_U8(0x90);
    else if (strcasecmp(mnemonic, "ret") == 0) EMIT_U8(0xC3);
    else if (strcasecmp(mnemonic, "leave") == 0) EMIT_U8(0xC9);
    else if (strcasecmp(mnemonic, "int") == 0) { // e.g., int 0x80
        EMIT_U8(0xCD); 
        EMIT_U8((uint8_t)op1.offset);
    }
    else if (strcasecmp(mnemonic, "push") == 0) {
        if (op1.type == OP_REG) EMIT_U8(0x50 + op1.reg);
        else if (op1.type == OP_IMM) { EMIT_U8(0x68); EMIT_U32(op1.offset); }
        else { 
            // push r/m
            Operand reg_op = {0}; reg_op.reg = 6; // /6
            EMIT_INSN_MODRM( 0xFF, &reg_op, &op1);
        }
    } else if (strcasecmp(mnemonic, "pop") == 0) {
        if (op1.type == OP_REG) EMIT_U8(0x58 + op1.reg);
    } else if (strcasecmp(mnemonic, "dec") == 0) {
        if (op1.type == OP_REG) EMIT_U8(0x48 + op1.reg);
        // dec r/m? opcode FF /1
    } else if (strcasecmp(mnemonic, "inc") == 0) {
        if (op1.type == OP_REG) EMIT_U8(0x40 + op1.reg);
        // inc r/m? opcode FF /0
    } else if (strcasecmp(mnemonic, "neg") == 0) {
        // F7 /3
        Operand ext = {0}; ext.reg = 3; EMIT_INSN_MODRM( 0xF7, &ext, &op1);
    } else if (strcasecmp(mnemonic, "not") == 0) {
        // F7 /2
        Operand ext = {0}; ext.reg = 2; EMIT_INSN_MODRM( 0xF7, &ext, &op1);
    } else if (strcasecmp(mnemonic, "mul") == 0) { // F7 /4 unsigned
        Operand ext = {0}; ext.reg = 4; EMIT_INSN_MODRM( 0xF7, &ext, &op1);
    } else if (strcasecmp(mnemonic, "div") == 0) { // F7 /6 unsigned
        Operand ext = {0}; ext.reg = 6; EMIT_INSN_MODRM( 0xF7, &ext, &op1);
    } else if (strcasecmp(mnemonic, "idiv") == 0) { // F7 /7 signed
        Operand ext = {0}; ext.reg = 7; EMIT_INSN_MODRM( 0xF7, &ext, &op1);
    } else if (strcasecmp(mnemonic, "imul") == 0) {
        if (op_count == 2) EMIT_INSN_MODRM( 0x0FAF, &op1, &op2);
        else { /* F7 /5 loop */ Operand ext = {0}; ext.reg = 5; EMIT_INSN_MODRM( 0xF7, &ext, &op1); }
    } else if (strcasecmp(mnemonic, "add") == 0) {
        if (op1.type == OP_IMM && op2.type == OP_REG) {
             // 81 /0 imm32 (or 83 /0 imm8)
             int opcode = (op1.offset >= -128 && op1.offset <= 127) ? 0x83 : 0x81;
             Operand ext = {0}; ext.reg = 0;
             EMIT_INSN_MODRM( opcode, &ext, &op2); // op2 is destination
             if (opcode == 0x83) EMIT_U8((int8_t)op1.offset); else EMIT_U32(op1.offset);
        } else EMIT_INSN_MODRM( 0x01, &op1, &op2); // add reg, rm? No add src, dst. chibicc: add %eax, %ebx (dst=%ebx). Op 01: ADD r/m, r. src=r(op1), dst=rm(op2).
    } else if (strcasecmp(mnemonic, "sub") == 0) {
        if (op1.type == OP_IMM && op2.type == OP_REG) {
             int opcode = (op1.offset >= -128 && op1.offset <= 127) ? 0x83 : 0x81;
             Operand ext = {0}; ext.reg = 5;
             EMIT_INSN_MODRM( opcode, &ext, &op2);
             if (opcode == 0x83) EMIT_U8((int8_t)op1.offset); else EMIT_U32(op1.offset);
        } else EMIT_INSN_MODRM( 0x29, &op1, &op2); // sub reg, rm
    } else if (strcasecmp(mnemonic, "and") == 0) {
        if (op1.type == OP_IMM) {
             Operand ext = {0}; ext.reg = 4;
             EMIT_INSN_MODRM( 0x81, &ext, &op2); EMIT_U32(op1.offset);
        } else EMIT_INSN_MODRM( 0x21, &op1, &op2);
    } else if (strcasecmp(mnemonic, "or") == 0) {
        if (op1.type == OP_IMM) {
             Operand ext = {0}; ext.reg = 1;
             EMIT_INSN_MODRM( 0x81, &ext, &op2); EMIT_U32(op1.offset);
        } else EMIT_INSN_MODRM( 0x09, &op1, &op2);
    } else if (strcasecmp(mnemonic, "xor") == 0) {
        if (op1.type == OP_IMM) {
             Operand ext = {0}; ext.reg = 6;
             EMIT_INSN_MODRM( 0x81, &ext, &op2); EMIT_U32(op1.offset);
        } else EMIT_INSN_MODRM( 0x31, &op1, &op2);
    } else if (strcasecmp(mnemonic, "shl") == 0) {
        Operand ext = {0}; ext.reg = 4;
        if (op1.type == OP_IMM) { EMIT_INSN_MODRM( 0xC1, &ext, &op2); EMIT_U8(op1.offset); }
        else if (op1.type == OP_REG && op1.reg == 1) { EMIT_INSN_MODRM( 0xD3, &ext, &op2); } // cl
    } else if (strcasecmp(mnemonic, "sar") == 0) {
        Operand ext = {0}; ext.reg = 7;
        if (op1.type == OP_IMM) { EMIT_INSN_MODRM( 0xC1, &ext, &op2); EMIT_U8(op1.offset); }
        else if (op1.type == OP_REG && op1.reg == 1) { EMIT_INSN_MODRM( 0xD3, &ext, &op2); }
    } else if (strcasecmp(mnemonic, "cmp") == 0) {
        if (op1.type == OP_IMM && op2.type == OP_REG) {
             int opcode = (op1.offset >= -128 && op1.offset <= 127) ? 0x83 : 0x81;
             Operand ext = {0}; ext.reg = 7;
             EMIT_INSN_MODRM( opcode, &ext, &op2);
             if (opcode == 0x83) EMIT_U8((int8_t)op1.offset); else EMIT_U32(op1.offset);
        } else EMIT_INSN_MODRM( 0x39, &op1, &op2);
    } else if (strcasecmp(mnemonic, "mov") == 0) {
        if (op1.type == OP_REG && op2.type == OP_REG) EMIT_INSN_MODRM( 0x89, &op1, &op2);
        else if (op1.type == OP_IMM && op2.type == OP_REG) { EMIT_U8(0xB8 + op2.reg); EMIT_U32(op1.offset); }
        else if (op1.type == OP_MEM && op2.type == OP_REG) EMIT_INSN_MODRM( 0x8B, &op2, &op1);
        else if (op1.type == OP_REG && op2.type == OP_MEM) EMIT_INSN_MODRM( 0x89, &op1, &op2);
    } else if (strcasecmp(mnemonic, "lea") == 0) {
        EMIT_INSN_MODRM( 0x8D, &op2, &op1);
    } else if (strcasecmp(mnemonic, "jmp") == 0) {
        EMIT_U8(0xE9);
        int32_t diff = 0;
        AsmLabel *l = find_label(op1.label_name);
        if (pass == 2 && l) diff = l->address - (current_addr + 5);
        EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "call") == 0) {
        if (op1.indirect && (op1.type == OP_REG || op1.type == OP_MEM)) { 
            Operand ext = {0}; ext.reg = 2;
            EMIT_INSN_MODRM(0xFF, &ext, &op1); 
        } // call r/m
        else {
            EMIT_U8(0xE8);
            int32_t diff = 0;
            AsmLabel *l = find_label(op1.label_name);
            kprint("nama label : ", multiboot_info);
            kprint(op1.label_name, multiboot_info);
            kprint("\n", multiboot_info);
            if (pass == 2) {
                if (!l || !l->defined) {
                    kprint("undefined label :", multiboot_info);
                    kprint(op1.label_name, multiboot_info);
                    kprint("\n", multiboot_info);
                    exit(1);
                }

                char buffer[512];
                to_string(l->address, buffer);
                kprint("ini address untuk ke label atau function :", multiboot_info);
                kprint_hex((uintptr_t)l->address, multiboot_info);
                kprint("\n", multiboot_info);
                to_string(current_addr, buffer);
                kprint("ini address saat ini:", multiboot_info);
                kprint_hex((uintptr_t)current_addr, multiboot_info);
                kprint("\n", multiboot_info);
                diff = l->address - (current_addr + 5);
                kprint("\n", multiboot_info);
                kprint("ini address yang dituju:", multiboot_info);
                kprint_hex((uintptr_t)diff, multiboot_info);
                kprint("\n", multiboot_info);
            }
            EMIT_U32(diff);
        }
    } else if (strcasecmp(mnemonic, "je") == 0 || strcasecmp(mnemonic, "jz") == 0) {
        EMIT_U8(0x0F); EMIT_U8(0x84);
        int32_t diff = 0; AsmLabel *l = find_label(op1.label_name);
        if (pass == 2 && l) diff = l->address - (current_addr + 6);
        EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "jne") == 0 || strcasecmp(mnemonic, "jnz") == 0) {
        EMIT_U8(0x0F); EMIT_U8(0x85);
        uint32_t diff = 0; AsmLabel *l = find_label(op1.label_name);
        if (pass == 2 && l) diff = l->address - (current_addr + 6);
        EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "jl") == 0) {
        EMIT_U8(0x0F); EMIT_U8(0x8C);
        uint32_t diff = 0; AsmLabel *l = find_label(op1.label_name); if (pass==2 && l) diff = l->address - (current_addr + 6); EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "jle") == 0) {
        EMIT_U8(0x0F); EMIT_U8(0x8E);
        uint32_t diff = 0; AsmLabel *l = find_label(op1.label_name); if (pass==2 && l) diff = l->address - (current_addr + 6); EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "jg") == 0) {
        EMIT_U8(0x0F); EMIT_U8(0x8F);
        uint32_t diff = 0; AsmLabel *l = find_label(op1.label_name); if (pass==2 && l) diff = l->address - (current_addr + 6); EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "jge") == 0) {
        EMIT_U8(0x0F); EMIT_U8(0x8D);
        uint32_t diff = 0; AsmLabel *l = find_label(op1.label_name); if (pass==2 && l) diff = l->address - (current_addr + 6); EMIT_U32(diff);
    } else if (strcasecmp(mnemonic, "sete") == 0) {
        Operand ext = {0}; ext.reg=0; EMIT_INSN_MODRM( 0x0F94, &ext, &op1);
    } else if (strcasecmp(mnemonic, "setne") == 0) {
        Operand ext = {0}; ext.reg=0; EMIT_INSN_MODRM( 0x0F95, &ext, &op1);
    } else if (strcasecmp(mnemonic, "setl") == 0) {
        Operand ext = {0}; ext.reg=0; EMIT_INSN_MODRM( 0x0F9C, &ext, &op1);
    } else if (strcasecmp(mnemonic, "setle") == 0) {
        Operand ext = {0}; ext.reg=0; EMIT_INSN_MODRM( 0x0F9E, &ext, &op1);
    } else if (strcasecmp(mnemonic, "movzx") == 0) { // movzx rm, reg -> 0F B6 /r
        EMIT_INSN_MODRM( 0x0FB6, &op2, &op1);
    } else if (strcasecmp(mnemonic, "movsx") == 0) { // movsx rm, reg -> 0F BE /r
        EMIT_INSN_MODRM( 0x0FBE, &op2, &op1);
    } else if (strcasecmp(mnemonic, "movsbl") == 0) {
        EMIT_INSN_MODRM( 0x0FBE, &op2, &op1);
    } else if (strcasecmp(mnemonic, "movswl") == 0) {
        EMIT_INSN_MODRM( 0x0FBF, &op2, &op1);
    } else if (strcasecmp(mnemonic, "movzbl") == 0) {
        EMIT_INSN_MODRM( 0x0FB6, &op2, &op1);
    } else if (strcasecmp(mnemonic, "movzwl") == 0) {
        EMIT_INSN_MODRM( 0x0FB7, &op2, &op1);
    } else if (strcasecmp(mnemonic, "cdq") == 0) {
        EMIT_U8(0x99);
    } 
    
    if(pass == 1) {
        return size;
    }
    if (buf && *buf) return *buf - start_buf;
    return 0;
}

int assemble_x86(const char *asm_text, size_t asm_len, char *output, size_t output_max, size_t *output_len) {
    reset_labels();
    
    uint32_t current_addr_base = CODE_BASE;
    uint32_t current_addr = current_addr_base;
    char line[256];
    const char *p = asm_text;
    
    // Pass 1
    while (p < asm_text + asm_len) {
        const char *eol = strchr(p, '\n');
        if (!eol) eol = asm_text + asm_len;
        int len = eol - p;
        if (len >= 256) len = 255;
        strncpy(line, p, len);
        line[len] = '\0';
        
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        if (*start && *start != ';' && *start != '#') {
             uint8_t tmp[128];
             uint8_t *ptr = tmp;
             int sz = process_instruction(start, &ptr, current_addr, 1);
             current_addr += sz;
        }
        p = eol + 1;
    }
    
    uint32_t code_size = current_addr - current_addr_base;
    if (output_max < 84 + code_size) return 1;
    
    // ELF Header
    memset(output, 0, 84);
    ELF32_Header *eh = (ELF32_Header*)output;
    eh->e_ident[0] = 0x7F; eh->e_ident[1] = 'E'; eh->e_ident[2] = 'L'; eh->e_ident[3] = 'F';
    eh->e_ident[4] = 1; eh->e_ident[5] = 1; eh->e_ident[6] = 1;
    eh->e_type = 2; eh->e_machine = 3; eh->e_version = 1;
    eh->e_entry = CODE_BASE;
    eh->e_phoff = 52; eh->e_ehsize = 52; eh->e_phentsize = 32; eh->e_phnum = 1;
    ELF32_ProgramHeader *ph = (ELF32_ProgramHeader*)(output + 52);
    ph->p_type = 1; ph->p_vaddr = ELF_BASE; ph->p_paddr = ELF_BASE;
    ph->p_filesz = ELF_HDRSZ + code_size; ph->p_memsz = ELF_HDRSZ + code_size;
    ph->p_flags = 7; ph->p_align = 0x1000;
    
    // Pass 2
    current_addr = current_addr_base;
    uint8_t *out_ptr = (uint8_t*)output + 84;
    p = asm_text;
    
    while (p < asm_text + asm_len) {
        const char *eol = strchr(p, '\n');
        if (!eol) eol = asm_text + asm_len;
        int len = eol - p;
        if (len >= 256) len = 255;
        strncpy(line, p, len);
        line[len] = '\0';
        
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        if (*start && *start != ';' && *start != '#') {
             uint8_t *prev = out_ptr;
             process_instruction(start, &out_ptr, current_addr, 2);
             current_addr += (out_ptr - prev);
        }
        p = eol + 1;
    }
    
    *output_len = 84 + code_size;
    return 0;
}