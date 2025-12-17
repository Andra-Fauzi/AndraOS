#include "chibicc.h"
#include "memory.h"

#define GP_MAX 6
#define FP_MAX 8

// 32-bit x86 target flag
#define TARGET_I386 1

static MemStream *output_file;
static int depth;
static char *argreg8[] = {"%dil", "%sil", "%dl", "%cl", "%r8db", "%r9db"};
static char *argreg16[] = {"%di", "%si", "%dx", "%cx", "%r8dw", "%r9dw"};
static char *argreg32[] = {"%edi", "%esi", "%edx", "%ecx", "%r8dd", "%r9dd"};
static char *argreg64[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
static Obj *current_fn;

static void gen_expr(Node *node);
static void gen_stmt(Node *node);

// vprintf‑like versi kamu
int chi_vprintf(MemStream *w, const char *fmt, va_list ap) {
    char buf[64];
    const char *p = fmt;
    int count = 0;

    while (*p) {
        if (*p == '%') {
            p++;
            bool is_long = false;
            if (*p == 'l') {
                is_long = true;
                p++;
            }

            switch (*p) {
            case 'd': {
                long val;
                if (is_long) val = va_arg(ap, long);
                else val = va_arg(ap, int);

                int n = 0;
                if (val == 0) {
                    buf[n++] = '0';
                } else if (val < 0) {
                    memstream_write(w, "-", 1);
                    count++;
                    val = -val;
                    char rev[32];
                    int ri = 0;
                    while (val > 0) {
                        rev[ri++] = '0' + (val % 10);
                        val /= 10;
                    }
                    while (ri) buf[n++] = rev[--ri];
                } else {
                    char rev[32];
                    int ri = 0;
                    while (val > 0) {
                        rev[ri++] = '0' + (val % 10);
                        val /= 10;
                    }
                    while (ri) buf[n++] = rev[--ri];
                }
                memstream_write(w, buf, n);
                count += n;
                break;
            }
            case 'u': {
                unsigned long val;
                if (is_long) val = va_arg(ap, unsigned long);
                else val = va_arg(ap, unsigned int);

                int n = 0;
                if (val == 0) {
                    buf[n++] = '0';
                } else {
                    char rev[32];
                    int ri = 0;
                    while (val > 0) {
                        rev[ri++] = '0' + (val % 10);
                        val /= 10;
                    }
                    while (ri) buf[n++] = rev[--ri];
                }
                memstream_write(w, buf, n);
                count += n;
                break;
            }
            case 'x': {
                unsigned long val;
                if (is_long) val = va_arg(ap, unsigned long);
                else val = va_arg(ap, unsigned int);

                int n = 0;
                if (val == 0) {
                    buf[n++] = '0';
                } else {
                    char rev[32];
                    int ri = 0;
                    while (val > 0) {
                        int digit = val % 16;
                        rev[ri++] = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
                        val /= 16;
                    }
                    while (ri) buf[n++] = rev[--ri];
                }
                memstream_write(w, buf, n);
                count += n;
                break;
            }
            case 'c': {
                int val = va_arg(ap, int);
                char c = (char)val;
                memstream_write(w, &c, 1);
                count++;
                break;
            }
            case 's': {
                const char *str = va_arg(ap, const char *);
                if (!str) str = "(null)";
                int i = 0;
                while (str[i]) {
                    memstream_write(w, &str[i], 1);
                    i++;
                }
                count += i;
                break;
            }
            default:
                memstream_write(w, p, 1);
                count++;
                break;
            }
            p++;
        } else {
            memstream_write(w, p, 1);
            count++;
            p++;
        }
    }
    return count;
}

// printf‑like wrapper kamu
int chi_printf(MemStream *w, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = chi_vprintf(w, fmt, ap);
    va_end(ap);
    return ret;
}

static void println(char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  chi_vprintf(output_file, fmt, ap);
  va_end(ap);
  chi_printf(output_file, "\n");
}

static int count(void) {
  static int i = 1;
  return i++;
}

static void push(void) {
  println("  push %%eax");
  depth++;
}

static void pop(char *arg) {
  println("  pop %s", arg);
  depth--;
}

static void pushf(void) {
  println("  sub $8, %%esp");
  println("  movsd %%xmm0, (%%esp)");
  depth++;
}

static void popf(int reg) {
  println("  movsd (%%esp), %%xmm%d", reg);
  println("  add $8, %%esp");
  depth--;
}

// Round up `n` to the nearest multiple of `align`. For instance,
// align_to(5, 8) returns 8 and align_to(11, 8) returns 16.
int align_to(int n, int align) {
  return (n + align - 1) / align * align;
}

static char *reg_dx(int sz) {
  switch (sz) {
  case 1: return "%dl";
  case 2: return "%dx";
  case 4: return "%edx";
  case 8: return "%edx";
  }
  unreachable();
}

static char *reg_ax(int sz) {
  switch (sz) {
  case 1: return "%al";
  case 2: return "%ax";
  case 4: return "%eax";
  case 8: return "%eax";
  }
  unreachable();
}

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node *node) {
  switch (node->kind) {
  case ND_VAR:
    // Variable-length array, which is always local.
    if (node->var->ty->kind == TY_VLA) {
      println("  mov %d(%%ebp), %%eax", node->var->offset);
      return;
    }

    // Local variable
    if (node->var->is_local) {
      println("  lea %d(%%ebp), %%eax", node->var->offset);
      return;
    }

    if (opt_fpic) {
      // Thread-local variable
      if (node->var->is_tls) {
        println("  data16 lea %s@tlsgd(%%rip), %%edi", node->var->name);
        println("  .value 0x6666");
        println("  rex64");
        println("  call __tls_get_addr@PLT");
        return;
      }

      // Function or global variable
      println("  mov %s@GOTPCREL(%%rip), %%eax", node->var->name);
      return;
    }

    // Thread-local variable
    if (node->var->is_tls) {
      println("  mov %%fs:0, %%eax");
      println("  add $%s@tpoff, %%eax", node->var->name);
      return;
    }

    // Here, we generate an absolute address of a function or a global
    // variable. Even though they exist at a certain address at runtime,
    // their addresses are not known at link-time for the following
    // two reasons.
    //
    //  - Address randomization: Executables are loaded to memory as a
    //    whole but it is not known what address they are loaded to.
    //    Therefore, at link-time, relative address in the same
    //    exectuable (i.e. the distance between two functions in the
    //    same executable) is known, but the absolute address is not
    //    known.
    //
    //  - Dynamic linking: Dynamic shared objects (DSOs) or .so files
    //    are loaded to memory alongside an executable at runtime and
    //    linked by the runtime loader in memory. We know nothing
    //    about addresses of global stuff that may be defined by DSOs
    //    until the runtime relocation is complete.
    //
    // In order to deal with the former case, we use RIP-relative
    // addressing, denoted by `(%rip)`. For the latter, we obtain an
    // address of a stuff that may be in a shared object file from the
    // Global Offset Table using `@GOTPCREL(%rip)` notation.

    // Function
    if (node->ty->kind == TY_FUNC) {
      if (node->var->is_definition)
        println("  lea %s(%%rip), %%eax", node->var->name);
      else
        println("  mov %s@GOTPCREL(%%rip), %%eax", node->var->name);
      return;
    }

    // Global variable
    println("  lea %s(%%rip), %%eax", node->var->name);
    return;
  case ND_DEREF:
    gen_expr(node->lhs);
    return;
  case ND_COMMA:
    gen_expr(node->lhs);
    gen_addr(node->rhs);
    return;
  case ND_MEMBER:
    gen_addr(node->lhs);
    println("  add $%d, %%eax", node->member->offset);
    return;
  case ND_FUNCALL:
    if (node->ret_buffer) {
      gen_expr(node);
      return;
    }
    break;
  case ND_ASSIGN:
  case ND_COND:
    if (node->ty->kind == TY_STRUCT || node->ty->kind == TY_UNION) {
      gen_expr(node);
      return;
    }
    break;
  case ND_VLA_PTR:
    println("  lea %d(%%ebp), %%eax", node->var->offset);
    return;
  }

  error_tok(node->tok, "not an lvalue");
}

// Load a value from where %eax is pointing to.
static void load(Type *ty) {
  switch (ty->kind) {
  case TY_ARRAY:
  case TY_STRUCT:
  case TY_UNION:
  case TY_FUNC:
  case TY_VLA:
    // If it is an array, do not attempt to load a value to the
    // register because in general we can't load an entire array to a
    // register. As a result, the result of an evaluation of an array
    // becomes not the array itself but the address of the array.
    // This is where "array is automatically converted to a pointer to
    // the first element of the array in C" occurs.
    return;
  case TY_FLOAT:
    println("  movss (%%eax), %%xmm0");
    return;
  case TY_DOUBLE:
    println("  movsd (%%eax), %%xmm0");
    return;
  case TY_LDOUBLE:
    println("  fldt (%%eax)");
    return;
  }

  char *insn = ty->is_unsigned ? "movz" : "movs";

  // When we load a char or a short value to a register, we always
  // extend them to the size of int, so we can assume the lower half of
  // a register always contains a valid value. The upper half of a
  // register for char, short and int may contain garbage. When we load
  // a long value to a register, it simply occupies the entire register.
  if (ty->size == 1)
    println("  %sbl (%%eax), %%eax", insn);
  else if (ty->size == 2)
    println("  %swl (%%eax), %%eax", insn);
  else if (ty->size == 4)
    println("  movsxd (%%eax), %%eax");
  else
    println("  mov (%%eax), %%eax");
}

// Store %eax to an address that the stack top is pointing to.
static void store(Type *ty) {
  pop("%edi");

  switch (ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
    for (int i = 0; i < ty->size; i++) {
      println("  mov %d(%%eax), %%r8ddb", i);
      println("  mov %%r8ddb, %d(%%edi)", i);
    }
    return;
  case TY_FLOAT:
    println("  movss %%xmm0, (%%edi)");
    return;
  case TY_DOUBLE:
    println("  movsd %%xmm0, (%%edi)");
    return;
  case TY_LDOUBLE:
    println("  fstpt (%%edi)");
    return;
  }

  if (ty->size == 1)
    println("  mov %%al, (%%edi)");
  else if (ty->size == 2)
    println("  mov %%ax, (%%edi)");
  else if (ty->size == 4)
    println("  mov %%eax, (%%edi)");
  else
    println("  mov %%eax, (%%edi)");
}

static void cmp_zero(Type *ty) {
  switch (ty->kind) {
  case TY_FLOAT:
    println("  xorps %%xmm1, %%xmm1");
    println("  ucomiss %%xmm1, %%xmm0");
    return;
  case TY_DOUBLE:
    println("  xorpd %%xmm1, %%xmm1");
    println("  ucomisd %%xmm1, %%xmm0");
    return;
  case TY_LDOUBLE:
    println("  fldz");
    println("  fucomip");
    println("  fstp %%st(0)");
    return;
  }

  if (is_integer(ty) && ty->size <= 4)
    println("  cmp $0, %%eax");
  else
    println("  cmp $0, %%eax");
}

enum { I8, I16, I32, I64, U8, U16, U32, U64, F32, F64, F80 };

static int getTypeId(Type *ty) {
  switch (ty->kind) {
  case TY_CHAR:
    return ty->is_unsigned ? U8 : I8;
  case TY_SHORT:
    return ty->is_unsigned ? U16 : I16;
  case TY_INT:
    return ty->is_unsigned ? U32 : I32;
  case TY_LONG:
    return ty->is_unsigned ? U64 : I64;
  case TY_FLOAT:
    return F32;
  case TY_DOUBLE:
    return F64;
  case TY_LDOUBLE:
    return F80;
  }
  return U64;
}

// The table for type casts
static char i32i8[] = "movsbl %al, %eax";
static char i32u8[] = "movzbl %al, %eax";
static char i32i16[] = "movswl %ax, %eax";
static char i32u16[] = "movzwl %ax, %eax";
static char i32f32[] = "cvtsi2ssl %eax, %xmm0";
static char i32i64[] = "movsxd %eax, %eax";
static char i32f64[] = "cvtsi2sdl %eax, %xmm0";
static char i32f80[] = "mov %eax, -4(%esp); fildl -4(%esp)";

static char u32f32[] = "mov %eax, %eax; cvtsi2ssq %eax, %xmm0";
static char u32i64[] = "mov %eax, %eax";
static char u32f64[] = "mov %eax, %eax; cvtsi2sdq %eax, %xmm0";
static char u32f80[] = "mov %eax, %eax; mov %eax, -8(%esp); fildll -8(%esp)";

static char i64f32[] = "cvtsi2ssq %eax, %xmm0";
static char i64f64[] = "cvtsi2sdq %eax, %xmm0";
static char i64f80[] = "movl %eax, -8(%esp); fildll -8(%esp)";

static char u64f32[] = "cvtsi2ssq %eax, %xmm0";
static char u64f64[] =
  "test %eax,%eax; js 1f; pxor %xmm0,%xmm0; cvtsi2sd %eax,%xmm0; jmp 2f; "
  "1: mov %eax,%edi; and $1,%eax; pxor %xmm0,%xmm0; shr %edi; "
  "or %eax,%edi; cvtsi2sd %edi,%xmm0; addsd %xmm0,%xmm0; 2:";
static char u64f80[] =
  "mov %eax, -8(%esp); fildq -8(%esp); test %eax, %eax; jns 1f;"
  "mov $1602224128, %eax; mov %eax, -4(%esp); fadds -4(%esp); 1:";

static char f32i8[] = "cvttss2sil %xmm0, %eax; movsbl %al, %eax";
static char f32u8[] = "cvttss2sil %xmm0, %eax; movzbl %al, %eax";
static char f32i16[] = "cvttss2sil %xmm0, %eax; movswl %ax, %eax";
static char f32u16[] = "cvttss2sil %xmm0, %eax; movzwl %ax, %eax";
static char f32i32[] = "cvttss2sil %xmm0, %eax";
static char f32u32[] = "cvttss2siq %xmm0, %eax";
static char f32i64[] = "cvttss2siq %xmm0, %eax";
static char f32u64[] = "cvttss2siq %xmm0, %eax";
static char f32f64[] = "cvtss2sd %xmm0, %xmm0";
static char f32f80[] = "movss %xmm0, -4(%esp); flds -4(%esp)";

static char f64i8[] = "cvttsd2sil %xmm0, %eax; movsbl %al, %eax";
static char f64u8[] = "cvttsd2sil %xmm0, %eax; movzbl %al, %eax";
static char f64i16[] = "cvttsd2sil %xmm0, %eax; movswl %ax, %eax";
static char f64u16[] = "cvttsd2sil %xmm0, %eax; movzwl %ax, %eax";
static char f64i32[] = "cvttsd2sil %xmm0, %eax";
static char f64u32[] = "cvttsd2siq %xmm0, %eax";
static char f64i64[] = "cvttsd2siq %xmm0, %eax";
static char f64u64[] = "cvttsd2siq %xmm0, %eax";
static char f64f32[] = "cvtsd2ss %xmm0, %xmm0";
static char f64f80[] = "movsd %xmm0, -8(%esp); fldl -8(%esp)";

#define FROM_F80_1                                           \
  "fnstcw -10(%esp); movzwl -10(%esp), %eax; or $12, %ah; " \
  "mov %ax, -12(%esp); fldcw -12(%esp); "

#define FROM_F80_2 " -24(%esp); fldcw -10(%esp); "

static char f80i8[] = FROM_F80_1 "fistps" FROM_F80_2 "movsbl -24(%esp), %eax";
static char f80u8[] = FROM_F80_1 "fistps" FROM_F80_2 "movzbl -24(%esp), %eax";
static char f80i16[] = FROM_F80_1 "fistps" FROM_F80_2 "movzbl -24(%esp), %eax";
static char f80u16[] = FROM_F80_1 "fistpl" FROM_F80_2 "movswl -24(%esp), %eax";
static char f80i32[] = FROM_F80_1 "fistpl" FROM_F80_2 "mov -24(%esp), %eax";
static char f80u32[] = FROM_F80_1 "fistpl" FROM_F80_2 "mov -24(%esp), %eax";
static char f80i64[] = FROM_F80_1 "fistpq" FROM_F80_2 "mov -24(%esp), %eax";
static char f80u64[] = FROM_F80_1 "fistpq" FROM_F80_2 "mov -24(%esp), %eax";
static char f80f32[] = "fstps -8(%esp); movss -8(%esp), %xmm0";
static char f80f64[] = "fstpl -8(%esp); movsd -8(%esp), %xmm0";

static char *cast_table[][11] = {
  // i8   i16     i32     i64     u8     u16     u32     u64     f32     f64     f80
  {NULL,  NULL,   NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80}, // i8
  {i32i8, NULL,   NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80}, // i16
  {i32i8, i32i16, NULL,   i32i64, i32u8, i32u16, NULL,   i32i64, i32f32, i32f64, i32f80}, // i32
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   i64f32, i64f64, i64f80}, // i64

  {i32i8, NULL,   NULL,   i32i64, NULL,  NULL,   NULL,   i32i64, i32f32, i32f64, i32f80}, // u8
  {i32i8, i32i16, NULL,   i32i64, i32u8, NULL,   NULL,   i32i64, i32f32, i32f64, i32f80}, // u16
  {i32i8, i32i16, NULL,   u32i64, i32u8, i32u16, NULL,   u32i64, u32f32, u32f64, u32f80}, // u32
  {i32i8, i32i16, NULL,   NULL,   i32u8, i32u16, NULL,   NULL,   u64f32, u64f64, u64f80}, // u64

  {f32i8, f32i16, f32i32, f32i64, f32u8, f32u16, f32u32, f32u64, NULL,   f32f64, f32f80}, // f32
  {f64i8, f64i16, f64i32, f64i64, f64u8, f64u16, f64u32, f64u64, f64f32, NULL,   f64f80}, // f64
  {f80i8, f80i16, f80i32, f80i64, f80u8, f80u16, f80u32, f80u64, f80f32, f80f64, NULL},   // f80
};

static void cast(Type *from, Type *to) {
  if (to->kind == TY_VOID)
    return;

  if (to->kind == TY_BOOL) {
    cmp_zero(from);
    println("  setne %%al");
    println("  movzx %%al, %%eax");
    return;
  }

  int t1 = getTypeId(from);
  int t2 = getTypeId(to);
  if (cast_table[t1][t2])
    println("  %s", cast_table[t1][t2]);
}

// Structs or unions equal or smaller than 16 bytes are passed
// using up to two registers.
//
// If the first 8 bytes contains only floating-point type members,
// they are passed in an XMM register. Otherwise, they are passed
// in a general-purpose register.
//
// If a struct/union is larger than 8 bytes, the same rule is
// applied to the the next 8 byte chunk.
//
// This function returns true if `ty` has only floating-point
// members in its byte range [lo, hi).
static bool has_flonum(Type *ty, int lo, int hi, int offset) {
  if (ty->kind == TY_STRUCT || ty->kind == TY_UNION) {
    for (Member *mem = ty->members; mem; mem = mem->next)
      if (!has_flonum(mem->ty, lo, hi, offset + mem->offset))
        return false;
    return true;
  }

  if (ty->kind == TY_ARRAY) {
    for (int i = 0; i < ty->array_len; i++)
      if (!has_flonum(ty->base, lo, hi, offset + ty->base->size * i))
        return false;
    return true;
  }

  return offset < lo || hi <= offset || ty->kind == TY_FLOAT || ty->kind == TY_DOUBLE;
}

static bool has_flonum1(Type *ty) {
  return has_flonum(ty, 0, 8, 0);
}

static bool has_flonum2(Type *ty) {
  return has_flonum(ty, 8, 16, 0);
}

static void push_struct(Type *ty) {
  int sz = align_to(ty->size, 8);
  println("  sub $%d, %%esp", sz);
  depth += sz / 8;

  for (int i = 0; i < ty->size; i++) {
    println("  mov %d(%%eax), %%r10b", i);
    println("  mov %%r10b, %d(%%esp)", i);
  }
}

static void push_args2(Node *args, bool first_pass) {
  if (!args)
    return;
  push_args2(args->next, first_pass);

  if ((first_pass && !args->pass_by_stack) || (!first_pass && args->pass_by_stack))
    return;

  gen_expr(args);

  switch (args->ty->kind) {
  case TY_STRUCT:
  case TY_UNION:
    push_struct(args->ty);
    break;
  case TY_FLOAT:
  case TY_DOUBLE:
    pushf();
    break;
  case TY_LDOUBLE:
    println("  sub $16, %%esp");
    println("  fstpt (%%esp)");
    depth += 2;
    break;
  default:
    push();
  }
}

// Load function call arguments. Arguments are already evaluated and
// stored to the stack as local variables. What we need to do in this
// function is to load them to registers or push them to the stack as
// specified by the x86-64 psABI. Here is what the spec says:
//
// - Up to 6 arguments of integral type are passed using RDI, RSI,
//   RDX, RCX, R8 and R9.
//
// - Up to 8 arguments of floating-point type are passed using XMM0 to
//   XMM7.
//
// - If all registers of an appropriate type are already used, push an
//   argument to the stack in the right-to-left order.
//
// - Each argument passed on the stack takes 8 bytes, and the end of
//   the argument area must be aligned to a 16 byte boundary.
//
// - If a function is variadic, set the number of floating-point type
//   arguments to RAX.
static int push_args(Node *node) {
  int stack = 0, gp = 0, fp = 0;

  // If the return type is a large struct/union, the caller passes
  // a pointer to a buffer as if it were the first argument.
  if (node->ret_buffer && node->ty->size > 16)
    gp++;

  // Load as many arguments to the registers as possible.
  for (Node *arg = node->args; arg; arg = arg->next) {
    Type *ty = arg->ty;

    switch (ty->kind) {
    case TY_STRUCT:
    case TY_UNION:
      if (ty->size > 16) {
        arg->pass_by_stack = true;
        stack += align_to(ty->size, 8) / 8;
      } else {
        bool fp1 = has_flonum1(ty);
        bool fp2 = has_flonum2(ty);

        if (fp + fp1 + fp2 < FP_MAX && gp + !fp1 + !fp2 < GP_MAX) {
          fp = fp + fp1 + fp2;
          gp = gp + !fp1 + !fp2;
        } else {
          arg->pass_by_stack = true;
          stack += align_to(ty->size, 8) / 8;
        }
      }
      break;
    case TY_FLOAT:
    case TY_DOUBLE:
      if (fp++ >= FP_MAX) {
        arg->pass_by_stack = true;
        stack++;
      }
      break;
    case TY_LDOUBLE:
      arg->pass_by_stack = true;
      stack += 2;
      break;
    default:
      if (gp++ >= GP_MAX) {
        arg->pass_by_stack = true;
        stack++;
      }
    }
  }

  if ((depth + stack) % 2 == 1) {
    println("  sub $8, %%esp");
    depth++;
    stack++;
  }

  push_args2(node->args, true);
  push_args2(node->args, false);

  // If the return type is a large struct/union, the caller passes
  // a pointer to a buffer as if it were the first argument.
  if (node->ret_buffer && node->ty->size > 16) {
    println("  lea %d(%%ebp), %%eax", node->ret_buffer->offset);
    push();
  }

  return stack;
}

static void copy_ret_buffer(Obj *var) {
  Type *ty = var->ty;
  int gp = 0, fp = 0;

  if (has_flonum1(ty)) {
    assert(ty->size == 4 || 8 <= ty->size);
    if (ty->size == 4)
      println("  movss %%xmm0, %d(%%ebp)", var->offset);
    else
      println("  movsd %%xmm0, %d(%%ebp)", var->offset);
    fp++;
  } else {
    for (int i = 0; i < MIN(8, ty->size); i++) {
      println("  mov %%al, %d(%%ebp)", var->offset + i);
      println("  shr $8, %%eax");
    }
    gp++;
  }

  if (ty->size > 8) {
    if (has_flonum2(ty)) {
      assert(ty->size == 12 || ty->size == 16);
      if (ty->size == 12)
        println("  movss %%xmm%d, %d(%%ebp)", fp, var->offset + 8);
      else
        println("  movsd %%xmm%d, %d(%%ebp)", fp, var->offset + 8);
    } else {
      char *reg1 = (gp == 0) ? "%al" : "%dl";
      char *reg2 = (gp == 0) ? "%eax" : "%edx";
      for (int i = 8; i < MIN(16, ty->size); i++) {
        println("  mov %s, %d(%%ebp)", reg1, var->offset + i);
        println("  shr $8, %s", reg2);
      }
    }
  }
}

static void copy_struct_reg(void) {
  Type *ty = current_fn->ty->return_ty;
  int gp = 0, fp = 0;

  println("  mov %%eax, %%edi");

  if (has_flonum(ty, 0, 8, 0)) {
    assert(ty->size == 4 || 8 <= ty->size);
    if (ty->size == 4)
      println("  movss (%%edi), %%xmm0");
    else
      println("  movsd (%%edi), %%xmm0");
    fp++;
  } else {
    println("  mov $0, %%eax");
    for (int i = MIN(8, ty->size) - 1; i >= 0; i--) {
      println("  shl $8, %%eax");
      println("  mov %d(%%edi), %%al", i);
    }
    gp++;
  }

  if (ty->size > 8) {
    if (has_flonum(ty, 8, 16, 0)) {
      assert(ty->size == 12 || ty->size == 16);
      if (ty->size == 4)
        println("  movss 8(%%edi), %%xmm%d", fp);
      else
        println("  movsd 8(%%edi), %%xmm%d", fp);
    } else {
      char *reg1 = (gp == 0) ? "%al" : "%dl";
      char *reg2 = (gp == 0) ? "%eax" : "%edx";
      println("  mov $0, %s", reg2);
      for (int i = MIN(16, ty->size) - 1; i >= 8; i--) {
        println("  shl $8, %s", reg2);
        println("  mov %d(%%edi), %s", i, reg1);
      }
    }
  }
}

static void copy_struct_mem(void) {
  Type *ty = current_fn->ty->return_ty;
  Obj *var = current_fn->params;

  println("  mov %d(%%ebp), %%edi", var->offset);

  for (int i = 0; i < ty->size; i++) {
    println("  mov %d(%%eax), %%dl", i);
    println("  mov %%dl, %d(%%edi)", i);
  }
}

static void builtin_alloca(void) {
  // Align size to 16 bytes.
  println("  add $15, %%edi");
  println("  and $0xfffffff0, %%edi");

  // Shift the temporary area by %edi.
  println("  mov %d(%%ebp), %%ecx", current_fn->alloca_bottom->offset);
  println("  sub %%esp, %%ecx");
  println("  mov %%esp, %%eax");
  println("  sub %%edi, %%esp");
  println("  mov %%esp, %%edx");
  println("1:");
  println("  cmp $0, %%ecx");
  println("  je 2f");
  println("  mov (%%eax), %%r8ddb");
  println("  mov %%r8ddb, (%%edx)");
  println("  inc %%edx");
  println("  inc %%eax");
  println("  dec %%ecx");
  println("  jmp 1b");
  println("2:");

  // Move alloca_bottom pointer.
  println("  mov %d(%%ebp), %%eax", current_fn->alloca_bottom->offset);
  println("  sub %%edi, %%eax");
  println("  mov %%eax, %d(%%ebp)", current_fn->alloca_bottom->offset);
}

// Generate code for a given node.
static void gen_expr(Node *node) {
  println("  .loc %d %d", node->tok->file->file_no, node->tok->line_no);

  switch (node->kind) {
  case ND_NULL_EXPR:
    return;
  case ND_NUM: {
    switch (node->ty->kind) {
    case TY_FLOAT: {
      union { float f32; uint32_t u32; } u = { node->fval };
      println("  mov $%u, %%eax  # float %Lf", u.u32, node->fval);
      println("  movl %%eax, %%xmm0");
      return;
    }
    case TY_DOUBLE: {
      union { double f64; uint32_t u64; } u = { node->fval };
      println("  mov $%lu, %%eax  # double %Lf", u.u64, node->fval);
      println("  movl %%eax, %%xmm0");
      return;
    }
    case TY_LDOUBLE: {
      union { long double f80; uint32_t u64[2]; } u;
      memset(&u, 0, sizeof(u));
      u.f80 = node->fval;
      println("  mov $%lu, %%eax  # long double %Lf", u.u64[0], node->fval);
      println("  mov %%eax, -16(%%esp)");
      println("  mov $%lu, %%eax", u.u64[1]);
      println("  mov %%eax, -8(%%esp)");
      println("  fldt -16(%%esp)");
      return;
    }
    }

    println("  mov $%ld, %%eax", node->val);
    return;
  }
  case ND_NEG:
    gen_expr(node->lhs);

    switch (node->ty->kind) {
    case TY_FLOAT:
      println("  mov $1, %%eax");
      println("  shl $31, %%eax");
      println("  movl %%eax, %%xmm1");
      println("  xorps %%xmm1, %%xmm0");
      return;
    case TY_DOUBLE:
      println("  mov $1, %%eax");
      println("  shl $63, %%eax");
      println("  movl %%eax, %%xmm1");
      println("  xorpd %%xmm1, %%xmm0");
      return;
    case TY_LDOUBLE:
      println("  fchs");
      return;
    }

    println("  neg %%eax");
    return;
  case ND_VAR:
    gen_addr(node);
    load(node->ty);
    return;
  case ND_MEMBER: {
    gen_addr(node);
    load(node->ty);

    Member *mem = node->member;
    if (mem->is_bitfield) {
      println("  shl $%d, %%eax", 64 - mem->bit_width - mem->bit_offset);
      if (mem->ty->is_unsigned)
        println("  shr $%d, %%eax", 64 - mem->bit_width);
      else
        println("  sar $%d, %%eax", 64 - mem->bit_width);
    }
    return;
  }
  case ND_DEREF:
    gen_expr(node->lhs);
    load(node->ty);
    return;
  case ND_ADDR:
    gen_addr(node->lhs);
    return;
  case ND_ASSIGN:
    gen_addr(node->lhs);
    push();
    gen_expr(node->rhs);

    if (node->lhs->kind == ND_MEMBER && node->lhs->member->is_bitfield) {
      println("  mov %%eax, %%r8dd");

      // If the lhs is a bitfield, we need to read the current value
      // from memory and merge it with a new value.
      Member *mem = node->lhs->member;
      println("  mov %%eax, %%edi");
      println("  and $%ld, %%edi", (1L << mem->bit_width) - 1);
      println("  shl $%d, %%edi", mem->bit_offset);

      println("  mov (%%esp), %%eax");
      load(mem->ty);

      long mask = ((1L << mem->bit_width) - 1) << mem->bit_offset;
      println("  mov $%ld, %%r9dd", ~mask);
      println("  and %%r9dd, %%eax");
      println("  or %%edi, %%eax");
      store(node->ty);
      println("  mov %%r8dd, %%eax");
      return;
    }

    store(node->ty);
    return;
  case ND_STMT_EXPR:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  case ND_COMMA:
    gen_expr(node->lhs);
    gen_expr(node->rhs);
    return;
  case ND_CAST:
    gen_expr(node->lhs);
    cast(node->lhs->ty, node->ty);
    return;
  case ND_MEMZERO:
    // `rep stosb` is equivalent to `memset(%edi, %al, %ecx)`.
    println("  mov $%d, %%ecx", node->var->ty->size);
    println("  lea %d(%%ebp), %%edi", node->var->offset);
    println("  mov $0, %%al");
    println("  rep stosb");
    return;
  case ND_COND: {
    int c = count();
    gen_expr(node->cond);
    cmp_zero(node->cond->ty);
    println("  je .L.else.%d", c);
    gen_expr(node->then);
    println("  jmp .L.end.%d", c);
    println(".L.else.%d:", c);
    gen_expr(node->els);
    println(".L.end.%d:", c);
    return;
  }
  case ND_NOT:
    gen_expr(node->lhs);
    cmp_zero(node->lhs->ty);
    println("  sete %%al");
    println("  movzx %%al, %%eax");
    return;
  case ND_BITNOT:
    gen_expr(node->lhs);
    println("  not %%eax");
    return;
  case ND_LOGAND: {
    int c = count();
    gen_expr(node->lhs);
    cmp_zero(node->lhs->ty);
    println("  je .L.false.%d", c);
    gen_expr(node->rhs);
    cmp_zero(node->rhs->ty);
    println("  je .L.false.%d", c);
    println("  mov $1, %%eax");
    println("  jmp .L.end.%d", c);
    println(".L.false.%d:", c);
    println("  mov $0, %%eax");
    println(".L.end.%d:", c);
    return;
  }
  case ND_LOGOR: {
    int c = count();
    gen_expr(node->lhs);
    cmp_zero(node->lhs->ty);
    println("  jne .L.true.%d", c);
    gen_expr(node->rhs);
    cmp_zero(node->rhs->ty);
    println("  jne .L.true.%d", c);
    println("  mov $0, %%eax");
    println("  jmp .L.end.%d", c);
    println(".L.true.%d:", c);
    println("  mov $1, %%eax");
    println(".L.end.%d:", c);
    return;
  }
  case ND_FUNCALL: {
    if (node->lhs->kind == ND_VAR && !strcmp(node->lhs->var->name, "alloca")) {
      gen_expr(node->args);
      println("  mov %%eax, %%edi");
      builtin_alloca();
      return;
    }

    int stack_args = push_args(node);
    gen_expr(node->lhs);

    int gp = 0, fp = 0;

    // If the return type is a large struct/union, the caller passes
    // a pointer to a buffer as if it were the first argument.
    if (node->ret_buffer && node->ty->size > 16)
      pop(argreg64[gp++]);

    for (Node *arg = node->args; arg; arg = arg->next) {
      Type *ty = arg->ty;

      switch (ty->kind) {
      case TY_STRUCT:
      case TY_UNION:
        if (ty->size > 16)
          continue;

        bool fp1 = has_flonum1(ty);
        bool fp2 = has_flonum2(ty);

        if (fp + fp1 + fp2 < FP_MAX && gp + !fp1 + !fp2 < GP_MAX) {
          if (fp1)
            popf(fp++);
          else
            pop(argreg64[gp++]);

          if (ty->size > 8) {
            if (fp2)
              popf(fp++);
            else
              pop(argreg64[gp++]);
          }
        }
        break;
      case TY_FLOAT:
      case TY_DOUBLE:
        if (fp < FP_MAX)
          popf(fp++);
        break;
      case TY_LDOUBLE:
        break;
      default:
        if (gp < GP_MAX)
          pop(argreg64[gp++]);
      }
    }

    println("  mov %%eax, %%r10");
    println("  mov $%d, %%eax", fp);
    println("  call *%%r10");
    println("  add $%d, %%esp", stack_args * 8);

    depth -= stack_args;

    // It looks like the most significant 48 or 56 bits in RAX may
    // contain garbage if a function return type is short or bool/char,
    // respectively. We clear the upper bits here.
    switch (node->ty->kind) {
    case TY_BOOL:
      println("  movzx %%al, %%eax");
      return;
    case TY_CHAR:
      if (node->ty->is_unsigned)
        println("  movzbl %%al, %%eax");
      else
        println("  movsbl %%al, %%eax");
      return;
    case TY_SHORT:
      if (node->ty->is_unsigned)
        println("  movzwl %%ax, %%eax");
      else
        println("  movswl %%ax, %%eax");
      return;
    }

    // If the return type is a small struct, a value is returned
    // using up to two registers.
    if (node->ret_buffer && node->ty->size <= 16) {
      copy_ret_buffer(node->ret_buffer);
      println("  lea %d(%%ebp), %%eax", node->ret_buffer->offset);
    }

    return;
  }
  case ND_LABEL_VAL:
    println("  lea %s(%%rip), %%eax", node->unique_label);
    return;
  case ND_CAS: {
    gen_expr(node->cas_addr);
    push();
    gen_expr(node->cas_new);
    push();
    gen_expr(node->cas_old);
    println("  mov %%eax, %%r8dd");
    load(node->cas_old->ty->base);
    pop("%edx"); // new
    pop("%edi"); // addr

    int sz = node->cas_addr->ty->base->size;
    println("  lock cmpxchg %s, (%%edi)", reg_dx(sz));
    println("  sete %%cl");
    println("  je 1f");
    println("  mov %s, (%%r8dd)", reg_ax(sz));
    println("1:");
    println("  movzbl %%cl, %%eax");
    return;
  }
  case ND_EXCH: {
    gen_expr(node->lhs);
    push();
    gen_expr(node->rhs);
    pop("%edi");

    int sz = node->lhs->ty->base->size;
    println("  xchg %s, (%%edi)", reg_ax(sz));
    return;
  }
  }

  switch (node->lhs->ty->kind) {
  case TY_FLOAT:
  case TY_DOUBLE: {
    gen_expr(node->rhs);
    pushf();
    gen_expr(node->lhs);
    popf(1);

    char *sz = (node->lhs->ty->kind == TY_FLOAT) ? "ss" : "sd";

    switch (node->kind) {
    case ND_ADD:
      println("  add%s %%xmm1, %%xmm0", sz);
      return;
    case ND_SUB:
      println("  sub%s %%xmm1, %%xmm0", sz);
      return;
    case ND_MUL:
      println("  mul%s %%xmm1, %%xmm0", sz);
      return;
    case ND_DIV:
      println("  div%s %%xmm1, %%xmm0", sz);
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  ucomi%s %%xmm0, %%xmm1", sz);

      if (node->kind == ND_EQ) {
        println("  sete %%al");
        println("  setnp %%dl");
        println("  and %%dl, %%al");
      } else if (node->kind == ND_NE) {
        println("  setne %%al");
        println("  setp %%dl");
        println("  or %%dl, %%al");
      } else if (node->kind == ND_LT) {
        println("  seta %%al");
      } else {
        println("  setae %%al");
      }

      println("  and $1, %%al");
      println("  movzb %%al, %%eax");
      return;
    }

    error_tok(node->tok, "invalid expression");
  }
  case TY_LDOUBLE: {
    gen_expr(node->lhs);
    gen_expr(node->rhs);

    switch (node->kind) {
    case ND_ADD:
      println("  faddp");
      return;
    case ND_SUB:
      println("  fsubrp");
      return;
    case ND_MUL:
      println("  fmulp");
      return;
    case ND_DIV:
      println("  fdivrp");
      return;
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
      println("  fcomip");
      println("  fstp %%st(0)");

      if (node->kind == ND_EQ)
        println("  sete %%al");
      else if (node->kind == ND_NE)
        println("  setne %%al");
      else if (node->kind == ND_LT)
        println("  seta %%al");
      else
        println("  setae %%al");

      println("  movzb %%al, %%eax");
      return;
    }

    error_tok(node->tok, "invalid expression");
  }
  }

  gen_expr(node->rhs);
  push();
  gen_expr(node->lhs);
  pop("%edi");

  char *ax, *di, *dx;

  if (node->lhs->ty->kind == TY_LONG || node->lhs->ty->base) {
    ax = "%eax";
    di = "%edi";
    dx = "%edx";
  } else {
    ax = "%eax";
    di = "%edi";
    dx = "%edx";
  }

  switch (node->kind) {
  case ND_ADD:
    println("  add %s, %s", di, ax);
    return;
  case ND_SUB:
    println("  sub %s, %s", di, ax);
    return;
  case ND_MUL:
    println("  imul %s, %s", di, ax);
    return;
  case ND_DIV:
  case ND_MOD:
    if (node->ty->is_unsigned) {
      println("  mov $0, %s", dx);
      println("  div %s", di);
    } else {
      if (node->lhs->ty->size == 8)
        println("  cqo");
      else
        println("  cdq");
      println("  idiv %s", di);
    }

    if (node->kind == ND_MOD)
      println("  mov %%edx, %%eax");
    return;
  case ND_BITAND:
    println("  and %s, %s", di, ax);
    return;
  case ND_BITOR:
    println("  or %s, %s", di, ax);
    return;
  case ND_BITXOR:
    println("  xor %s, %s", di, ax);
    return;
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
    println("  cmp %s, %s", di, ax);

    if (node->kind == ND_EQ) {
      println("  sete %%al");
    } else if (node->kind == ND_NE) {
      println("  setne %%al");
    } else if (node->kind == ND_LT) {
      if (node->lhs->ty->is_unsigned)
        println("  setb %%al");
      else
        println("  setl %%al");
    } else if (node->kind == ND_LE) {
      if (node->lhs->ty->is_unsigned)
        println("  setbe %%al");
      else
        println("  setle %%al");
    }

    println("  movzb %%al, %%eax");
    return;
  case ND_SHL:
    println("  mov %%edi, %%ecx");
    println("  shl %%cl, %s", ax);
    return;
  case ND_SHR:
    println("  mov %%edi, %%ecx");
    if (node->lhs->ty->is_unsigned)
      println("  shr %%cl, %s", ax);
    else
      println("  sar %%cl, %s", ax);
    return;
  }

  error_tok(node->tok, "invalid expression");
}

static void gen_asm(Node *node) {
  if (!node->asm_inputs && !node->asm_outputs) {
    println("  %s", node->asm_str);
    return;
  }

  Node *args[30];
  int nargs = 0;

  for (Node *n = node->asm_outputs; n; n = n->next)
    args[nargs++] = n;

  int num_outputs = nargs;

  for (Node *n = node->asm_inputs; n; n = n->next)
    args[nargs++] = n;

  if (nargs > 30)
    error_tok(node->tok, "too many asm operands");

  char *regs[30];
  const char *r32[] = {"%edi", "%esi", "%edx", "%ecx", "%ebx", "%eax"};
  const char *r16[] = {"%di", "%si", "%dx", "%cx", "%bx", "%ax"};
  const char *r8[] =  {"%dil", "%sil", "%dl", "%cl", "%bl", "%al"};

  for (int i = 0; i < nargs; i++) {
    int sz = args[i]->lhs->ty->size;
    char *constraint = args[i]->asm_str;
    regs[i] = NULL;
    
    if (constraint) {
       if (strchr(constraint, 'a')) regs[i] = reg_ax(sz);
       else if (strchr(constraint, 'b')) regs[i] = (sz==4?"%ebx":(sz==2?"%bx":"%bl"));
       else if (strchr(constraint, 'c')) regs[i] = (sz==4?"%ecx":(sz==2?"%cx":"%cl"));
       else if (strchr(constraint, 'd')) regs[i] = (sz==4?"%edx":(sz==2?"%dx":"%dl"));
       else if (strchr(constraint, 'S')) {
            if (sz == 1) error_tok(node->tok, "cannot use SI/ESI for 8-bit operand");
            regs[i] = (sz==4?"%esi":"%si");
       }
       else if (strchr(constraint, 'D')) {
            if (sz == 1) error_tok(node->tok, "cannot use DI/EDI for 8-bit operand");
            regs[i] = (sz==4?"%edi":"%di");
       }
    }

    if (regs[i]) continue;

    if (sz == 8) error_tok(node->tok, "asm does not support 64-bit operands in 32-bit mode");
    else if (sz == 4) regs[i] = (char *)r32[i];
    else if (sz == 2) regs[i] = (char *)r16[i];
    else if (sz == 1) regs[i] = (char *)r8[i];
    else error_tok(node->tok, "asm only supports integer operands");
  }

  for (int i = num_outputs; i < nargs; i++) {
    gen_expr(args[i]->lhs);
    push();
  }

  for (int i = nargs - 1; i >= num_outputs; i--) {
    pop(regs[i]);
  }

  char buf[4096];
  char *d = buf;
  char *p = node->asm_str;

  while (*p && d < buf + 4096 - 100) {
    if (*p == '%') {
      if (p[1] == '%') {
        *d++ = '%';
        p += 2;
        continue;
      }
      
      char *end;
      long idx = strtoul(p + 1, &end, 10);
      if (end != p + 1) {
        if (idx < 0 || idx >= nargs)
          error_tok(node->tok, "invalid operand index");
        char *r = regs[idx];
        while (*r) *d++ = *r++;
        p = end;
        continue;
      }
    }
    *d++ = *p++;
  }
  *d = 0;
  println("  %s", buf);

  for (int i = 0; i < num_outputs; i++) {
    gen_addr(args[i]->lhs);
    
    // The previous instruction (gen_addr) put the address in %rax.
    // The value to store is in regs[i].
    // Since %rax is used for address, we can't use it for the value.
    // Fortunaltey regs[i] is likely NOT %rax (we used rdi etc).
    
    int sz = args[i]->lhs->ty->size;
    if (sz == 8) println("  mov %s, (%%rax)", regs[i]);
    else if (sz == 4) println("  mov %s, (%%rax)", regs[i]);
    else if (sz == 2) println("  mov %s, (%%rax)", regs[i]);
    else if (sz == 1) println("  mov %s, (%%rax)", regs[i]);
  }
}

static void gen_stmt(Node *node) {
  println("  .loc %d %d", node->tok->file->file_no, node->tok->line_no);

  switch (node->kind) {
  case ND_IF: {
    int c = count();
    gen_expr(node->cond);
    cmp_zero(node->cond->ty);
    println("  je  .L.else.%d", c);
    gen_stmt(node->then);
    println("  jmp .L.end.%d", c);
    println(".L.else.%d:", c);
    if (node->els)
      gen_stmt(node->els);
    println(".L.end.%d:", c);
    return;
  }
  case ND_FOR: {
    int c = count();
    if (node->init)
      gen_stmt(node->init);
    println(".L.begin.%d:", c);
    if (node->cond) {
      gen_expr(node->cond);
      cmp_zero(node->cond->ty);
      println("  je %s", node->brk_label);
    }
    gen_stmt(node->then);
    println("%s:", node->cont_label);
    if (node->inc)
      gen_expr(node->inc);
    println("  jmp .L.begin.%d", c);
    println("%s:", node->brk_label);
    return;
  }
  case ND_DO: {
    int c = count();
    println(".L.begin.%d:", c);
    gen_stmt(node->then);
    println("%s:", node->cont_label);
    gen_expr(node->cond);
    cmp_zero(node->cond->ty);
    println("  jne .L.begin.%d", c);
    println("%s:", node->brk_label);
    return;
  }
  case ND_SWITCH:
    gen_expr(node->cond);

    for (Node *n = node->case_next; n; n = n->case_next) {
      char *ax = (node->cond->ty->size == 8) ? "%eax" : "%eax";
      char *di = (node->cond->ty->size == 8) ? "%edi" : "%edi";

      if (n->begin == n->end) {
        println("  cmp $%ld, %s", n->begin, ax);
        println("  je %s", n->label);
        continue;
      }

      // [GNU] Case ranges
      println("  mov %s, %s", ax, di);
      println("  sub $%ld, %s", n->begin, di);
      println("  cmp $%ld, %s", n->end - n->begin, di);
      println("  jbe %s", n->label);
    }

    if (node->default_case)
      println("  jmp %s", node->default_case->label);

    println("  jmp %s", node->brk_label);
    gen_stmt(node->then);
    println("%s:", node->brk_label);
    return;
  case ND_CASE:
    println("%s:", node->label);
    gen_stmt(node->lhs);
    return;
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next)
      gen_stmt(n);
    return;
  case ND_GOTO:
    println("  jmp %s", node->unique_label);
    return;
  case ND_GOTO_EXPR:
    gen_expr(node->lhs);
    println("  jmp *%%eax");
    return;
  case ND_LABEL:
    println("%s:", node->unique_label);
    gen_stmt(node->lhs);
    return;
  case ND_RETURN:
    if (node->lhs) {
      gen_expr(node->lhs);
      Type *ty = node->lhs->ty;

      switch (ty->kind) {
      case TY_STRUCT:
      case TY_UNION:
        if (ty->size <= 16)
          copy_struct_reg();
        else
          copy_struct_mem();
        break;
      }
    }

    println("  jmp .L.return.%s", current_fn->name);
    return;
  case ND_EXPR_STMT:
    gen_expr(node->lhs);
    return;
  case ND_ASM:
    gen_asm(node);
    return;
  }

  error_tok(node->tok, "invalid statement");
}

// Assign offsets to local variables.
static void assign_lvar_offsets(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function)
      continue;

    // If a function has many parameters, some parameters are
    // inevitably passed by stack rather than by register.
    // The first passed-by-stack parameter resides at RBP+16.
    int top = 16;
    int bottom = 0;

    int gp = 0, fp = 0;

    // Assign offsets to pass-by-stack parameters.
    for (Obj *var = fn->params; var; var = var->next) {
      Type *ty = var->ty;

      switch (ty->kind) {
      case TY_STRUCT:
      case TY_UNION:
        if (ty->size <= 16) {
          bool fp1 = has_flonum(ty, 0, 8, 0);
          bool fp2 = has_flonum(ty, 8, 16, 8);
          if (fp + fp1 + fp2 < FP_MAX && gp + !fp1 + !fp2 < GP_MAX) {
            fp = fp + fp1 + fp2;
            gp = gp + !fp1 + !fp2;
            continue;
          }
        }
        break;
      case TY_FLOAT:
      case TY_DOUBLE:
        if (fp++ < FP_MAX)
          continue;
        break;
      case TY_LDOUBLE:
        break;
      default:
        if (gp++ < GP_MAX)
          continue;
      }

      top = align_to(top, 8);
      var->offset = top;
      top += var->ty->size;
    }

    // Assign offsets to pass-by-register parameters and local variables.
    for (Obj *var = fn->locals; var; var = var->next) {
      if (var->offset)
        continue;

      // AMD64 System V ABI has a special alignment rule for an array of
      // length at least 16 bytes. We need to align such array to at least
      // 16-byte boundaries. See p.14 of
      // https://github.com/hjl-tools/x86-psABI/wiki/x86-64-psABI-draft.pdf.
      int align = (var->ty->kind == TY_ARRAY && var->ty->size >= 16)
        ? MAX(16, var->align) : var->align;

      bottom += var->ty->size;
      bottom = align_to(bottom, align);
      var->offset = -bottom;
    }

    fn->stack_size = align_to(bottom, 16);
  }
}

static void emit_data(Obj *prog) {
  for (Obj *var = prog; var; var = var->next) {
    if (var->is_function || !var->is_definition)
      continue;

    if (var->is_static)
      println("  .local %s", var->name);
    else
      println("  .globl %s", var->name);

    int align = (var->ty->kind == TY_ARRAY && var->ty->size >= 16)
      ? MAX(16, var->align) : var->align;

    // Common symbol
    if (opt_fcommon && var->is_tentative) {
      println("  .comm %s, %d, %d", var->name, var->ty->size, align);
      continue;
    }

    // .data or .tdata
    if (var->init_data) {
      if (var->is_tls)
        println("  .section .tdata,\"awT\",@progbits");
      else
        println("  .data");

      println("  .type %s, @object", var->name);
      println("  .size %s, %d", var->name, var->ty->size);
      println("  .align %d", align);
      println("%s:", var->name);

      Relocation *rel = var->rel;
      int pos = 0;
      while (pos < var->ty->size) {
        if (rel && rel->offset == pos) {
          println("  .quad %s%+ld", *rel->label, rel->addend);
          rel = rel->next;
          pos += 8;
        } else {
          println("  .byte %d", var->init_data[pos++]);
        }
      }
      continue;
    }

    // .bss or .tbss
    if (var->is_tls)
      println("  .section .tbss,\"awT\",@nobits");
    else
      println("  .bss");

    println("  .align %d", align);
    println("%s:", var->name);
    println("  .zero %d", var->ty->size);
  }
}

static void store_fp(int r, int offset, int sz) {
  switch (sz) {
  case 4:
    println("  movss %%xmm%d, %d(%%ebp)", r, offset);
    return;
  case 8:
    println("  movsd %%xmm%d, %d(%%ebp)", r, offset);
    return;
  }
  unreachable();
}

static void store_gp(int r, int offset, int sz) {
  switch (sz) {
  case 1:
    println("  mov %s, %d(%%ebp)", argreg8[r], offset);
    return;
  case 2:
    println("  mov %s, %d(%%ebp)", argreg16[r], offset);
    return;
  case 4:
    println("  mov %s, %d(%%ebp)", argreg32[r], offset);
    return;
  case 8:
    println("  mov %s, %d(%%ebp)", argreg64[r], offset);
    return;
  default:
    for (int i = 0; i < sz; i++) {
      println("  mov %s, %d(%%ebp)", argreg8[r], offset + i);
      println("  shr $8, %s", argreg64[r]);
    }
    return;
  }
}

static void emit_text(Obj *prog) {
  for (Obj *fn = prog; fn; fn = fn->next) {
    if (!fn->is_function || !fn->is_definition)
      continue;

    // No code is emitted for "static inline" functions
    // if no one is referencing them.
    if (!fn->is_live)
      continue;

    if (fn->is_static)
      println("  .local %s", fn->name);
    else
      println("  .globl %s", fn->name);

    println("  .text");
    println("  .type %s, @function", fn->name);
    println("%s:", fn->name);
    current_fn = fn;

    // Prologue
#ifdef TARGET_I386
    println("  push %%ebp");
    println("  mov %%esp, %%ebp");
    println("  sub $%d, %%esp", fn->stack_size);
#else
    println("  push %%ebp");
    println("  mov %%esp, %%ebp");
    println("  sub $%d, %%esp", fn->stack_size);
#endif
    println("  mov %%esp, %d(%%ebp)", fn->alloca_bottom->offset);

    // Save arg registers if function is variadic
    if (fn->va_area) {
      int gp = 0, fp = 0;
      for (Obj *var = fn->params; var; var = var->next) {
        if (is_flonum(var->ty))
          fp++;
        else
          gp++;
      }

      int off = fn->va_area->offset;

      // va_elem
      println("  movl $%d, %d(%%ebp)", gp * 8, off);          // gp_offset
      println("  movl $%d, %d(%%ebp)", fp * 8 + 48, off + 4); // fp_offset
      println("  movl %%ebp, %d(%%ebp)", off + 8);            // overflow_arg_area
      println("  addl $16, %d(%%ebp)", off + 8);
      println("  movl %%ebp, %d(%%ebp)", off + 16);           // reg_save_area
      println("  addl $%d, %d(%%ebp)", off + 24, off + 16);

      // __reg_save_area__
      println("  movl %%edi, %d(%%ebp)", off + 24);
      println("  movl %%esi, %d(%%ebp)", off + 32);
      println("  movl %%edx, %d(%%ebp)", off + 40);
      println("  movl %%ecx, %d(%%ebp)", off + 48);
      println("  movl %%r8dd, %d(%%ebp)", off + 56);
      println("  movl %%r9dd, %d(%%ebp)", off + 64);
      println("  movsd %%xmm0, %d(%%ebp)", off + 72);
      println("  movsd %%xmm1, %d(%%ebp)", off + 80);
      println("  movsd %%xmm2, %d(%%ebp)", off + 88);
      println("  movsd %%xmm3, %d(%%ebp)", off + 96);
      println("  movsd %%xmm4, %d(%%ebp)", off + 104);
      println("  movsd %%xmm5, %d(%%ebp)", off + 112);
      println("  movsd %%xmm6, %d(%%ebp)", off + 120);
      println("  movsd %%xmm7, %d(%%ebp)", off + 128);
    }

    // Save passed-by-register arguments to the stack
    int gp = 0, fp = 0;
    for (Obj *var = fn->params; var; var = var->next) {
      if (var->offset > 0)
        continue;

      Type *ty = var->ty;

      switch (ty->kind) {
      case TY_STRUCT:
      case TY_UNION:
        assert(ty->size <= 16);
        if (has_flonum(ty, 0, 8, 0))
          store_fp(fp++, var->offset, MIN(8, ty->size));
        else
          store_gp(gp++, var->offset, MIN(8, ty->size));

        if (ty->size > 8) {
          if (has_flonum(ty, 8, 16, 0))
            store_fp(fp++, var->offset + 8, ty->size - 8);
          else
            store_gp(gp++, var->offset + 8, ty->size - 8);
        }
        break;
      case TY_FLOAT:
      case TY_DOUBLE:
        store_fp(fp++, var->offset, ty->size);
        break;
      default:
        store_gp(gp++, var->offset, ty->size);
      }
    }

    // Emit code
    gen_stmt(fn->body);
    assert(depth == 0);

    // [https://www.sigbus.info/n1570#5.1.2.2.3p1] The C spec defines
    // a special rule for the main function. Reaching the end of the
    // main function is equivalent to returning 0, even though the
    // behavior is undefined for the other functions.
    if (strcmp(fn->name, "main") == 0)
      println("  mov $0, %%eax");

    // Epilogue
    println(".L.return.%s:", fn->name);
#ifdef TARGET_I386
    println("  mov %%ebp, %%esp");
    println("  pop %%ebp");
#else
    println("  mov %%ebp, %%esp");
    println("  pop %%ebp");
#endif
    println("  ret");
  }
}

void codegen(Obj *prog, MemStream *out) {
  output_file = out;

  File **files = get_input_files();
  for (int i = 0; files[i]; i++)
    println("  .file %d \"%s\"", files[i]->file_no, files[i]->name);

  assign_lvar_offsets(prog);
  emit_data(prog);
  emit_text(prog);
}
