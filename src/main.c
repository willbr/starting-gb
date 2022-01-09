#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define true  1
#define false 0

#define flag_mask_z  (1 << 7)
#define flag_mask_n  (1 << 6)
#define flag_mask_h  (1 << 5)
#define flag_mask_cy (1 << 4)

#define flag_z(f)  ((f) & flag_mask_z)
#define flag_n(f)  ((f) & flag_mask_n)
#define flag_h(f)  ((f) & flag_mask_h)
#define flag_cy(f) ((f) & flag_mask_cy)

typedef unsigned int uint;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

#define LIST_OF_TYPES \
    X(nil) \
    X(string) \
    X(i32) \
    X(r8)  \
    X(r16) \
    X(u8) \
    X(u16) \
    X(i8) \
    X(u3) \
    X(condition) \
    X(deref_r8) \
    X(deref_r16) \
    X(deref_u16) \
    X(fn) \
    X(end)

typedef enum Type {
#define X(name) type_##name,
    LIST_OF_TYPES
#undef X
} Type;

char *type_names[] = {
#define X(name) #name,
    LIST_OF_TYPES
#undef X
};


#define LIST_OF_KEYWORDS \
    X(nil) \
    X(illegal) \
    X(z) \
    X(nz) \
    X(cy) \
    X(nc) \
    X(deref_c) \
    X(deref_bc) \
    X(deref_de) \
    X(deref_hl) \
    X(deref_u8) \
    X(deref_u16) \
    X(00h) \
    X(08h) \
    X(10h) \
    X(18h) \
    X(20h) \
    X(28h) \
    X(30h) \
    X(38h) \
    X(a) \
    X(a16) \
    X(a8) \
    X(adc) \
    X(add) \
    X(af) \
    X(and) \
    X(b) \
    X(bc) \
    X(c) \
    X(call) \
    X(ccf) \
    X(cp) \
    X(cpl) \
    X(d) \
    X(daa) \
    X(de) \
    X(dec) \
    X(di) \
    X(e) \
    X(ei) \
    X(h) \
    X(halt) \
    X(hl) \
    X(inc) \
    X(jp) \
    X(jr) \
    X(l) \
    X(ld) \
    X(ldh) \
    X(ldi) \
    X(ldd) \
    X(nop) \
    X(or) \
    X(pop) \
    X(prefix) \
    X(push) \
    X(r8) \
    X(ret) \
    X(reti) \
    X(rla) \
    X(rlca) \
    X(rra) \
    X(rrca) \
    X(rst) \
    X(sbc) \
    X(scf) \
    X(sp) \
    X(stop) \
    X(sub) \
    X(u8) \
    X(u16) \
    X(xor) \
    X(end)


typedef enum Keyword {
#define X(name) keyword_##name,
    LIST_OF_KEYWORDS
#undef X
} Keyword;


char *keyword_names[] = {
#define X(name) #name,
    LIST_OF_KEYWORDS
#undef X
};

#include "opcodes.h"

#define TOKEN_LEN 32

#define ESC "\x1b"
#define BLACK_TEXT "30"
#define RED_TEXT "31"
#define YELLOW_TEXT "33"
#define CYAN_TEXT "36"
#define WHITE_TEXT "37"
#define BRIGHT_BLACK_TEXT   "90"
#define BRIGHT_RED_TEXT     "91"
#define BRIGHT_GREEN_TEXT   "92"
#define BRIGHT_YELLOW_TEXT  "93"
#define BRIGHT_BLUE_TEXT    "94"
#define BRIGHT_MAGENTA_TEXT "95"
#define BRIGHT_CYAN_TEXT    "96"
#define BRIGHT_WHITE_TEXT   "97"
#define RESET ESC "[0m"

#define CTEXT(c, s) ESC "[" c "m" s RESET

#define ere \
    do { \
        fprintf(stderr, \
                CTEXT(YELLOW_TEXT, "\n%s : %d : %s\n"), \
                __FILE__, __LINE__, __func__); \
    } while (0);

#define die(msg, ...) \
    do { \
        fprintf(stderr, \
                CTEXT(RED_TEXT, "\nerror: " msg "\n"), \
                ## __VA_ARGS__); \
        ere; \
        exit(1); \
    } while (0);

#define debug_var(s,v) \
    fprintf(stderr, #v ": %" s "\n", v)


union registers {
        struct {
#if SYSTEM_IS_BIG_ENDIAN
                u8 b; u8 c;
                u8 d; u8 e;
                u8 h; u8 l;
                u8 a; u8 f;
#else
                u8 c; u8 b;
                u8 e; u8 d;
                u8 l; u8 h;
                u8 f; u8 a;
#endif
                u16 sp;
                u16 pc;

                u8 deref_hl;
        } br;

        struct {
                u16 bc;
                u16 de;
                u16 hl;
                u16 af;

                u16 sp;
                u16 pc;

                u8 deref_hl;
        } wr;

};


typedef int (*fnptr)(struct Stack *);

typedef struct Object {
    Type type;
    char name[TOKEN_LEN];
    int name_length;
    union {
        i32 i;
        fnptr fn;
    };
} Object;


typedef struct Stack {
    Object data[64];
    Object *next;
    Object *last;
    int length;
} Stack;


typedef struct DictElem {
    char name[TOKEN_LEN];
    Type type;
    fnptr fn;
} DictElem;


typedef struct Dict {
    DictElem words[TOKEN_LEN];
    DictElem *next;
    DictElem *last;
    int length;
} Dict;


union registers reg;
union registers prev_reg;

u8 memory[0x10000];

struct settings {
    int echo_bytes;
    int num_words;
    int reading_rom;
    Dict dict;
} global;


/* ##### */

void Code_repr(u8 *code);

Keyword Keyword_from_string(const char *);
void Keyword_repr(Keyword k);
void Opcode_repr(Opcode *o);

void DictElem_repr(DictElem *e);
void Dict_init(Dict *d);
void Dict_repr(Dict *d);
int  Dict_alloc_word(Dict *d, const char *n, Type t);
int  Dict_add_fn(Dict *d, const char *n, fnptr fn);

void Stack_init(Stack *s);
void Stack_push_i32(Stack *s, i32 i);
int Stack_push_u8(Stack *s, u8 i);
int Stack_pop_i32(Stack *s, i32 *i);
int Stack_push_object(Stack *s, Object *o);
int Stack_pop_object(Stack *s, Object *o);
void Stack_repr(Stack *s);
int Stack_add(Stack *s);

int u8_from_object(u8 *i, Object *o);
void Object_repr(Object *o);

void chomp(char **in, char c);
int read_token(char *dst, const char *src, size_t n);

u8 peek8(u16 addr);
u8* peek8ptr(u16 addr);
void init(void);
void print_header(void);
void print_line_prefix(void);
int parse_number(i32 *n, const char *arg);
int parse_addr(u16 *addr, const char *arg);
int parse_u8(u8 *n, const char *arg);

int lookup_word(Object *o, char *w);
int lookup_opcode(Keyword k, Stack *s, Opcode **o);
int invalid_argument(Object *o, Keyword w);

void eval_rpn(Stack *s, const char *x);
void eval_string(char *x, int echo);

void assemble(u8 *code, const char *cmd, const char *args);
void eval(u8 *code, int echo);

/* ##### */


Keyword
Keyword_from_string(const char *s)
{
    int num_keywords = sizeof keyword_names / sizeof keyword_names[0];
    for (int i = 0; i < num_keywords; i += 1) {
        if (!strcmp(s, keyword_names[i]))
                return i;
    }
    return -1;
}


void
Keyword_repr(Keyword k)
{
    if (k == -1) {
        ere;
        debug_var("d", k);
        die("invalid k");
    }
    fprintf(stderr, "Keyword(%d, %s)\n", k, keyword_names[k]);
}


void
Opcode_repr(Opcode *o)
{
/*typedef struct Opcode {*/
    /*int code;*/
    /*char mnemonic[16];*/
    /*int bytes;*/
    /*int total_cycles;*/
    /*int cycles[2];*/
    /*int num_operands;*/
    /*Operand operands[2];*/
    /*int immediate;*/
    /*Opcode_Flags flag;*/
/*} Opcode;*/
    fprintf(stderr,
            "Opcode($%02x, %s, %d bytes, ",
            o->code,
            o->mnemonic,
            o->bytes);
    fprintf(stderr, "\"%s", keyword_names[o->words[0]]);
    char *sep = " ";
    char *prefix = "";
    for (int i = 0; i < o->num_operands; i += 1) {
        char *name = keyword_names[o->words[i+1]];
        if (o->immediate) {
            prefix = "";
        } else if (o->operands[i].immediate) {
            prefix = "";
        } else {
            name += 6;
            prefix = "*";
        }
        fprintf(stderr, "%s%s%s", sep, prefix, name);
        sep = ", ";
    }
    fprintf(stderr, "\")\n");
}


int
u8_from_object(u8 *i, Object *o)
{
    i32 src = 0;
    assert(o->type == type_i32);
    src = o->i;

    if (src > 0xff)
        die("too big");
    if (src < 0x00)
        die("too small");

    *i = src;
    /*debug_var("d", *i);*/

    /*die("ere");*/
    return 0;
}


void
Object_repr(Object *o)
{
    /*ere;*/
    /*debug_var("p", o);*/
    /*debug_var("d", o->type);*/
    fprintf(stderr, "(Object ");

    if (o == NULL) {
        fprintf(stderr, "NULL pointer)\n");
        return;
    }

    if (o->type >= type_end) {
        fprintf(stderr, "invalid type)\n");
        return;
    }

    switch (o->type) {
    case type_nil:
        fprintf(stderr, "nil)\n", o->i);
        break;

    case type_i32:
        fprintf(stderr, "i32 0x%08x)\n", o->i);
        break;

    case type_r8:
        fprintf(stderr, "r8 %s)\n", o->name);
        break;

    case type_r16:
        fprintf(stderr, "r16 %s)\n", o->name);
        break;

    case type_deref_r16:
        fprintf(stderr, "*r16 %s)\n", o->name);
        break;

    case type_deref_u16:
        fprintf(stderr, "*u16 %04x)\n", o->i);
        break;

    case type_condition:
        fprintf(stderr, "condition %s)\n", o->name);
        break;

    default:
        fprintf(stderr, "missing repr for: %s\n", type_names[o->type]);
        die("todo");
    }
}


void
Dict_init(Dict *d)
{
    int num_elems = sizeof d->words / sizeof d->words[0];
    d->next = d->words;
    d->last = &d->words[num_elems - 1];
    d->length = 0;
}

void
Dict_repr(Dict *d)
{
    int i = 0;
    DictElem *e = d->words;
    debug_var("d", d->length);

    for (i = 0; i < d->length; i += 1, e += 1) {
        fprintf(stderr, "word:%d\n", i);
        DictElem_repr(e);
        fprintf(stderr, "\n");
    }
}


void
DictElem_repr(DictElem *e)
{
    debug_var("p", e);
    debug_var("s", e->name);
    debug_var("x", e->type);
    debug_var("p", e->fn);
}


int
Dict_add_fn(Dict *d, const char *n, fnptr fn)
{
    Dict_alloc_word(d, n, type_fn);
    (d->next - 1)->fn = fn;
    return 0;
}


int
Dict_alloc_word(Dict *d, const char *n, Type t)
{
    DictElem *e = d->next;

    if (e > d->last)
        die("dict overflow");

    strncpy(e->name, n, 31);
    e->name[31] = '\0';
    e->type = t;
    e->fn = NULL;

    d->next += 1;
    d->length += 1;
    return 0;
}


void
Stack_init(Stack *s)
{
    int num_elems = sizeof s->data / sizeof s->data[0];
    s->next = s->data;
    s->last = &s->data[num_elems - 1];
    s->length = 0;
}


int
Stack_push_u8(Stack *s, u8 i)
{
    if (s->next > s->last)
        die("stack overflow");

    s->next->type = type_i32;
    s->next->i = i;

    s->next   += 1;
    s->length += 1;

    return 0;
}


void
Stack_push_i32(Stack *s, i32 i)
{
    if (s->next > s->last)
        die("stack overflow");

    s->next->type = type_i32;
    s->next->i = i;

    s->next   += 1;
    s->length += 1;
}


int
Stack_pop_i32(Stack *s, i32 *i)
{
    s->next   -= 1;
    s->length -= 1;

    if (s->next < s->data)
        die("stack underflow");

    assert(s->next->type == type_i32);
    *i = s->next->i;


    return 0;
}


int
Stack_push_object(Stack *s, Object *o)
{
    memcpy(s->next, o, sizeof(*s->next));
    s->next += 1;
    s->length += 1;
    return 0;
}


int
Stack_pop_object(Stack *s, Object *o)
{
    s->next -= 1;
    s->length -= 1;
    memcpy(o, s->next, sizeof(*s->next));
    return 0;
}


void
Stack_repr(Stack *s)
{
    Object *o = s->next - 1;

    fprintf(stderr, "\nStack (%d):\n", s->length);
    while(o >= s->data) {
        fprintf(stderr, "- ");
        Object_repr(o);
        o -= 1;
    }
    fprintf(stderr, "\n");
}


int
Stack_add(Stack *s)
{
    i32 a = 0;
    i32 b = 0;
    i32 r = 0;

    if (Stack_pop_i32(s, &b))
        die("fail");

    if (Stack_pop_i32(s, &a))
        die("fail");

    r = a + b;

    Stack_push_i32(s, r);
}


void
chomp(char **in, char c)
{
    while (**in == c)
        *in += 1;
}


int
read_token(char *dst, const char *src, size_t n)
{
    int i = 0;

    if (dst == NULL)
        die("dst is NULL");

    if (src == NULL)
        die("src is NULL");

    n -= 1;
    /*debug_var("d", n);*/
    while ((*src != '\0') && !isspace(*src) && n--) {
        *dst++ = *src++;
        i += 1;
        /*debug_var("d", n);*/
    }
    *dst = '\0';
    return i;
}


u8
peek8(u16 addr) {
    return *peek8ptr(addr);
}

u8*
peek8ptr(u16 addr) {
    return &memory[addr];
}


void
init(void)
{
    assert(sizeof u8  == 1);
    assert(sizeof u16 == 2);
    assert(sizeof u32 == 4);
    assert(sizeof u64 == 8);

    assert(sizeof i8  == 1);
    assert(sizeof i16 == 2);
    assert(sizeof i32 == 4);
    assert(sizeof i64 == 8);

    reg.wr.af = 0;
    reg.wr.bc = 0;
    reg.wr.de = 0;
    reg.wr.hl = 0;

    reg.wr.pc = 0x100;

    memcpy(&prev_reg, &reg, sizeof(reg));

    Dict_init(&global.dict);
    Dict_add_fn(&global.dict, "+", Stack_add);

    Dict_alloc_word(&global.dict, "a", type_r8);
    Dict_alloc_word(&global.dict, "b", type_r8);
    Dict_alloc_word(&global.dict, "c", type_r8);
    Dict_alloc_word(&global.dict, "d", type_r8);
    Dict_alloc_word(&global.dict, "e", type_r8);
    Dict_alloc_word(&global.dict, "h", type_r8);
    Dict_alloc_word(&global.dict, "l", type_r8);

    Dict_alloc_word(&global.dict, "af", type_r16);
    Dict_alloc_word(&global.dict, "bc", type_r16);
    Dict_alloc_word(&global.dict, "de", type_r16);
    Dict_alloc_word(&global.dict, "hl", type_r16);
    Dict_alloc_word(&global.dict, "sp", type_r16);
    Dict_alloc_word(&global.dict, "pc", type_r16);

    Dict_alloc_word(&global.dict, "z",  type_condition);
    Dict_alloc_word(&global.dict, "nz", type_condition);
    Dict_alloc_word(&global.dict, "cy", type_condition);
    Dict_alloc_word(&global.dict, "nc", type_condition);
    /*Dict_repr(&global.dict);*/
}


void
print_header(void)
{
    puts(" a  znhc bc   de   hl   *hl  bank:offset instruction");
    puts(" ================================================");
}


void
print_line_prefix(void)
{
#define highlight_diff(new, old) \
    do { \
        if (new != old) { \
            printf(ESC "[" WHITE_TEXT "m"); \
        } else { \
            printf(ESC "[" BRIGHT_BLACK_TEXT "m"); \
        } \
    } while(0)

    highlight_diff(reg.br.a, prev_reg.br.a);
    printf(" %02x ", reg.br.a);

    highlight_diff(flag_z(reg.br.f), flag_z(prev_reg.br.f));
    printf("%c", flag_z(reg.br.f) ? 'z' : '-');
    highlight_diff(flag_n(reg.br.f), flag_n(prev_reg.br.f));
    printf("%c", flag_n(reg.br.f) ? 'z' : '-');
    highlight_diff(flag_h(reg.br.f), flag_h(prev_reg.br.f));
    printf("%c", flag_h(reg.br.f) ? 'z' : '-');
    highlight_diff(flag_cy(reg.br.f), flag_cy(prev_reg.br.f));
    printf("%c", flag_cy(reg.br.f) ? 'z' : '-');

    highlight_diff(reg.br.b, prev_reg.br.b);
    printf(" %02x", reg.br.b);
    highlight_diff(reg.br.c, prev_reg.br.c);
    printf("%02x", reg.br.c);

    highlight_diff(reg.br.d, prev_reg.br.d);
    printf(" %02x", reg.br.d);
    highlight_diff(reg.br.e, prev_reg.br.e);
    printf("%02x", reg.br.e);

    highlight_diff(reg.br.h, prev_reg.br.h);
    printf(" %02x", reg.br.h);
    highlight_diff(reg.br.l, prev_reg.br.l);
    printf("%02x", reg.br.l);

    reg.wr.deref_hl = peek8(reg.wr.hl);
    highlight_diff(reg.wr.deref_hl, prev_reg.wr.deref_hl);
    printf("  %02x", reg.wr.deref_hl);

    /* todo highlight bank */
    printf(ESC "[" BRIGHT_BLACK_TEXT "m");
    printf("  %4s", "rom0");

    printf(":");
    highlight_diff(reg.wr.pc, prev_reg.wr.pc);
    printf("%04x", reg.wr.pc);

    printf("   " RESET);

    prev_reg.wr.deref_hl = reg.wr.deref_hl;
}


int
parse_number(i32 *n, const char *arg)
{
    long int v = 0;
    int base = 10;
    char *endptr = NULL;
    if (*arg == '$') {
        arg += 1;
        base = 16;
    }
    v = strtol(arg, &endptr, base);
    if (*endptr) {
        return 1;
    }
    *n = v;
    return 0;
}


int
parse_addr(u16 *addr, const char *arg)
{
    u16 v = 0;
    int base = 10;
    char *endptr = NULL;
    if (*arg == '$') {
        arg += 1;
        base = 16;
    }
    v = strtol(arg, &endptr, base);
    if (*endptr) {
        die("failed to parse");
        return 1;
    }
    /*debug_var("d", *endptr);*/
    /*debug_var("s", arg);*/
    /*debug_var("d", v);*/
    *addr = v;
    return 0;
}


int
parse_u8(u8 *n, const char *arg)
{
    u8 v = 0;
    int base = 10;
    char *endptr = NULL;

    /*debug_var("s", arg);*/

    if (*arg == '$') {
        arg += 1;
        base = 16;
    }

    *n = strtol(arg, &endptr, base);
    if (*endptr) {
        die("failed to parse");
        return 1;
    }
    /*debug_var("d", *endptr);*/
    /*debug_var("d", v);*/
    return 0;
}


void
eval_string(char *x, int echo)
{
    char word[64] = "";
    char *in = x;
    u8 code[3] = {0};
    int i = 0;
    char *spacer = NULL;
    Opcode *op = NULL;

    chomp(&in, ' ');
    if (*in == '\n' || *in == '\0')
        return;

    if (echo)
        printf("%s", x);

    in += read_token(word, in, sizeof(word));
    chomp(&in, ' ');
    /*ere;*/
    /*debug_var("s", x);*/

    assemble(code, word, in);
    eval(code, false);

    if (global.echo_bytes) {
        printf("%38s", "");
        op = &opcode_table[code[0]];

        printf(ESC "[" BRIGHT_BLACK_TEXT "m");
        for (i = 0; i < op->bytes; i += 1) {
            spacer = i < (op->bytes - 1) ? " " : "";
            printf("%02x%s", code[i], spacer);
        }
        printf(RESET "\n");
    }
}


int
lookup_word(Object *o, char *w)
{
    DictElem *e = global.dict.words;
    int i = 0;

    for (i = 0; i < global.dict.length; i += 1, e += 1) {
        if (!strcmp(e->name, w)) {
            o->type = e->type;
            switch (o->type) {
            case type_i32:
                DictElem_repr(e);
                /*o->i = e->i;*/
                die("ere");
                break;

            case type_fn:
                o->fn = e->fn;
                strcpy(o->name, w);
                break;

            case type_condition:
            case type_r8:
            case type_r16:
                strcpy(o->name, w);
                break;

            default:
                debug_var("s", type_names[o->type]);
                die("ere");
                break;
            }
            return 0;
        }
        /*DictElem_repr(e);*/
    }
    return 1;
}


int
Object_fits_u16(Object *o)
{
    if (o->type == type_i32 || o->type == type_deref_u16) {
        return ((0 <= o->i) && (o->i <= 0xffff));
    } else {
        return 0;
    }
}


int
Object_fits_u8(Object *o)
{
    if (o->type == type_i32) {
        return ((0 <= o->i) && (o->i <= 0xff));
    } else {
        return 0;
    }
}


int
invalid_argument(Object *o, Keyword k)
{
    /*ere;*/
    /*Object_repr(o);*/
    /*Keyword_repr(k);*/

    switch (k) {
    case keyword_a16:
    case keyword_u16:
        return !Object_fits_u16(o);

    case keyword_u8:
        return !Object_fits_u8(o);

    case keyword_r8:
        return (o->type != type_r8);

    case keyword_a:
    case keyword_b:
    case keyword_c:
    case keyword_d:
    case keyword_e:
    case keyword_h:
    case keyword_l:
        if (o->type != type_r8)
            return true;
        return strcmp(keyword_names[k], o->name);

    case keyword_bc:
    case keyword_de:
    case keyword_hl:
    case keyword_sp:
        if (o->type != type_r16)
            return true;
        return strcmp(keyword_names[k], o->name);

    case keyword_z:
    case keyword_nz:
    case keyword_cy:
    case keyword_nc:
        if (o->type != type_condition)
            return true;
        return strcmp(keyword_names[k], o->name);

    case keyword_deref_u8:
        if (o->type != type_i32)
            return true;
        return !Object_fits_u8(o);

    case keyword_deref_u16:
        if (o->type != type_deref_u16)
            return true;
        return !Object_fits_u16(o);

    case keyword_deref_c:
        if (o->type != type_deref_r8)
            return true;
        ere;
        Object_repr(o);
        Keyword_repr(k);
        die("*c");

    case keyword_deref_bc:
    case keyword_deref_de:
    case keyword_deref_hl:
        if (o->type != type_deref_r16)
            return true;
        /* +6 to skip 'deref_' */
        return strcmp(keyword_names[k]+6, o->name);

    default:
        ere;
        Object_repr(o);
        Keyword_repr(k);
        die("todo");
        return true;
    }
}


int
lookup_opcode(Keyword k, Stack *s, Opcode **o)
{
    /*ere;*/
    /*Stack_repr(s);*/

    int num_opcodes = sizeof opcode_table / sizeof opcode_table[0];
    Opcode *op = opcode_table;
    int match = false;

    *o = NULL;

    /*ere;*/
    /*debug_var("d", s->length - 1);*/

    for (int i = num_opcodes; i; i -= 1, op += 1) {
        int num_args = op->num_words > 0 ? op->num_words - 1 : 0;
        /*ere;*/
        /*debug_var("d", op->num_words);*/
        if ((s->length == num_args) && (k == op->words[0])) {
            /*ere;*/
            /*debug_var("d", num_args);*/
            /*Keyword_repr(k);*/
            /*Opcode_repr(op);*/
            Object *obj = s->next - num_args;

            /*ere;*/
            /*Stack_repr(s);*/
            match = true;
            for (int j = 1; j < op->num_words; j += 1, obj += 1) {
                /*ere;*/
                /*Stack_repr(s);*/
                if (invalid_argument(obj, op->words[j])) {
                    match = false;
                    break;
                }
                /*ere;*/
                /*Stack_repr(s);*/
                /*Object_repr(obj);*/

            }
            if (match) {
                /*ere;*/
                /*Stack_repr(s);*/
                /*Opcode_repr(op);*/
                /*debug_var("d", num_args);*/
                *o = op;
                return 0;
            }
        }
    }

    return 1;
}


void
eval_rpn(Stack *s, const char *x)
{
    char tok[64] = "";
    char *w;
    const char *in = x;
    i32 l = 0;

    while (*in != '\n' && *in != '\0') {
        Object o;
        o.type = type_nil;
        in += read_token(tok, in, sizeof(tok));
        w = tok;
        /*debug_var("s", tok);*/
        chomp(&in, ' ');
        switch (*w) {
            case '*':
                w += 1;
                /*debug_var("s", w);*/
                if (parse_number(&l, w)) {
                    if(lookup_word(&o, w)) {
                        debug_var("s", w);
                        die("error");
                    }
                    /*Object_repr(&o);*/
                    if (o.type == type_fn) {
                        die("can't deref a function");
                    } else {
                        /*ere;*/
                        if (o.type != type_r16) {
                            Object_repr(&o);
                            die("expected a deref of a r16");
                        }
                        o.type = type_deref_r16;
                        Stack_push_object(s, &o);
                        /*ere;*/
                    }
                } else {
                    /*debug_var("x", l);*/
                    o.type = type_deref_u16;
                    o.i = l;
                    Stack_push_object(s, &o);
                    /*Stack_push_i32(s, l);*/
                }

                break;

            default:
                if (parse_number(&l, tok)) {
                    if(lookup_word(&o, tok)) {
                        debug_var("s", tok);
                        die("error");
                    }
                    /*Object_repr(&o);*/
                    if (o.type == type_fn) {
                        /*Stack_repr(s);*/
                        o.fn(s);
                        /*Stack_repr(s);*/
                    } else {
                        /*ere;*/
                        Stack_push_object(s, &o);
                        /*ere;*/
                    }
                } else {
                    /*debug_var("x", l);*/
                    Stack_push_i32(s, l);
                }
                break;
        }

        /*Stack_repr(s);*/
    }
}


void
assemble(u8 *code, const char *cmd, const char *args)
{
    u16 addr = 0;
    u8 v8 = 0;

    Object arg1 = {type_nil};
    Object arg2 = {type_nil};

    Stack s;
    Stack_init(&s);
    eval_rpn(&s, args);
    /*ere;*/
    /*Stack_repr(&s);*/

    Keyword k = Keyword_from_string(cmd);
    Opcode *op = NULL;

    if (lookup_opcode(k, &s, &op)) {
        ere;
        Stack_repr(&s);
        debug_var("s", cmd);
        Keyword_repr(k);
        ere;
        /*Opcode *op2 = &opcode_table[0x7e];*/
        /*Opcode_repr(op2);*/
        /*Object_repr(s.next - 1);*/
        /*Object_repr(s.next - 2);*/
        /*debug_var("d", invalid_argument(s.next - 1, keyword_u16));*/
        /*debug_var("d", invalid_argument(s.next - 2, keyword_hl));*/
        /*debug_var("d", invalid_argument(s.next - 2, keyword_a));*/
        /*debug_var("d", invalid_argument(s.next - 2, keyword_z));*/
        /*debug_var("d", invalid_argument(s.next - 2, keyword_nz));*/
        /*debug_var("d", invalid_argument(s.next - 2, keyword_cy));*/
        /*debug_var("d", invalid_argument(s.next - 2, keyword_nc));*/
        die("lookup failed");
    }

    /*ere;*/
    /*Stack_repr(&s);*/

    /*ere;*/
    /*Keyword_repr(k);*/
    /*Stack_repr(&s);*/
    /*Opcode_repr(op);*/

    /*debug_var("d", op->num_operands);*/

    *(code+0) = op->code;

    switch (op->num_operands) {
    case 0:
        break;

    case 1:
        if (Stack_pop_object(&s, &arg1))
            die("pop failed");

        /*Object_repr(&arg1);*/

        /*ere;*/
        /*Keyword_repr(op->words[1]);*/
        switch (op->words[1]) {
        case keyword_a:
        case keyword_b:
        case keyword_c:
        case keyword_d:
        case keyword_e:
        case keyword_h:
        case keyword_l:
        case keyword_af:
        case keyword_bc:
        case keyword_de:
        case keyword_hl:
        case keyword_sp:
        case keyword_deref_hl:
            break;

        case keyword_a16:
            if (!Object_fits_u16(&arg1))
                die("wrong size");
            addr = arg1.i;
            *(code+1) = (u8)(addr >> 8);
            *(code+2) = (u8)(addr >> 0);
            break;

        default:
            die("other");
        }
        break;

    case 2:
        if (Stack_pop_object(&s, &arg2))
            die("pop failed");

        if (Stack_pop_object(&s, &arg1))
            die("pop failed");

        /*Keyword_repr(op->words[1]);*/
        /*Keyword_repr(op->words[2]);*/

        /*ere;*/
        /*Opcode_repr(op);*/
        /*Object_repr(&arg1);*/
        /*Object_repr(&arg2);*/

        switch (op->words[1]) {
        case keyword_a:
        case keyword_b:
        case keyword_c:
        case keyword_d:
        case keyword_e:
        case keyword_h:
        case keyword_l:
        case keyword_z:
        case keyword_bc:
        case keyword_de:
        case keyword_hl:
        case keyword_nz:
        case keyword_cy:
        case keyword_nc:
        case keyword_deref_hl:
            break;

        case keyword_deref_u8:
            if (!Object_fits_u8(&arg1))
                die("wrong size");
            /*ere;*/
            *(code+1) = (u8)(arg2.i >> 0);
            break;

        case keyword_deref_u16:
            if (!Object_fits_u16(&arg1))
                die("wrong size");
            /*ere;*/
            *(code+1) = (u8)(arg2.i >> 8);
            *(code+2) = (u8)(arg2.i >> 0);
            break;


        default:
            ere;
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("other");
        }

        switch (op->words[2]) {
        case keyword_a:
        case keyword_bc:
        case keyword_de:
        case keyword_hl:
        case keyword_deref_hl:
            break;

        case keyword_u8:
            if (!Object_fits_u8(&arg2))
                die("wrong size");
            *(code+1) = (u8)(arg2.i);
            break;

        case keyword_u16:
        case keyword_a16:
            if (!Object_fits_u16(&arg2))
                die("wrong size");
            /*ere;*/
            *(code+1) = (u8)(arg2.i >> 8);
            *(code+2) = (u8)(arg2.i >> 0);
            break;

        default:
            ere;
            Opcode_repr(op);
            /*Opcode_repr(&opcode_table[2]);*/
            Keyword_repr(k);
            Keyword_repr(op->words[1]);
            Keyword_repr(op->words[2]);
            die("other");
        }

        break;

    default:
        die("todo");
    }


    if (false) {
        ere;
        fprintf(stderr, "code: %02x %02x %02x\n", *(code+0), *(code+1), *(code+2));
        /*debug_var("x", *(code+0));*/
        /*debug_var("x", *(code+1));*/
        /*debug_var("x", *(code+2));*/
    }
}


void
Code_repr(u8 *code)
{
    Opcode *o = &opcode_table[*code];
    int i = 0;
    u8 *c = code;
    u8  d8  = 0;
    u16 d16 = 0;
    i8  r8 = 0;
    char *sep = "";

    for (i = 0; i < 3; i += 1) {
        if (i < o->bytes) {
            fprintf(stderr, "%s%02x", sep, *c);
            c += 1;
        } else {
            fprintf(stderr, "%s  ", sep);
        }
        sep = " ";
    }
    fprintf(stderr, " ");

    char *prefix = "";
    fprintf(stderr, "%s", keyword_names[o->words[0]]);
    sep = " ";
    for (int i = 0; i < o->num_operands; i += 1) {
        Keyword k = o->words[i+1];
        char *name = keyword_names[k];
        char arg_buffer[TOKEN_LEN] = "";

        switch (k) {
        case keyword_a:
        case keyword_b:
        case keyword_c:
        case keyword_d:
        case keyword_e:
        case keyword_h:
        case keyword_l:

        case keyword_hl:

        case keyword_deref_hl:

        case keyword_z:
        case keyword_nz:
        case keyword_cy:
        case keyword_nc:

        case keyword_nop:
            break;

        case keyword_r8:
            r8  = *(code + 1) << 0;
            d16 = reg.wr.pc + r8;
            snprintf(arg_buffer, TOKEN_LEN - 1, "$%04x", d16);
            name = arg_buffer;
            break;

        case keyword_u8:
            d8  = *(code + 1) << 0;
            snprintf(arg_buffer, TOKEN_LEN - 1, "$%02x", d8);
            name = arg_buffer;
            break;

        case keyword_u16:
            d16  = *(code + 1) << 0;
            d16 += *(code + 2) << 8;
            snprintf(arg_buffer, TOKEN_LEN - 1, "$%04x", d16);
            name = arg_buffer;
            break;

        default:
            fprintf(stderr, "\n");
            Keyword_repr(k);
            die("unknown keyword");
        }

        if (o->immediate) {
            prefix = "";
        } else if (o->operands[i].immediate) {
            prefix = "";
        } else {
            name += 6;
            prefix = "*";
        }
        fprintf(stderr, "%s%s%s", sep, prefix, name);
        sep = ", ";
    }
    fprintf(stderr, "\n");
}


void
eval(u8 *code, int echo)
{
    u16 addr = 0;
    i8   r8 = 0;
    u8   d8 = 0;
    u16  d16 = 0;
    u8  *dst8 = NULL;
    u16 *dst16 = NULL;
    /*debug_var("x", *(code+0));*/
    /*debug_var("x", *(code+1));*/
    /*debug_var("x", *(code+2));*/
    /*debug_var("x", *(code+3));*/
    memcpy(&prev_reg, &reg, sizeof(reg));
    Opcode *op = &opcode_table[*code];

    if (echo)
        Code_repr(code);

    if (*code == 0) {
        /* nop */

    } else if (op->words[0] == keyword_ld) {
        /*Opcode_repr(op);*/
        /*debug_var("d", op->immediate);*/
        switch (op->words[1]) {
        case keyword_a:
            dst8 = &reg.br.a;
            break;

        case keyword_b:
            dst8 = &reg.br.b;
            break;

        case keyword_c:
            dst8 = &reg.br.c;
            break;

        case keyword_d:
            dst8 = &reg.br.d;
            break;

        case keyword_e:
            dst8 = &reg.br.e;
            break;

        case keyword_h:
            dst8 = &reg.br.h;
            break;

        case keyword_l:
            dst8 = &reg.br.l;
            break;

        case keyword_bc:
            dst16 = &reg.wr.bc;
            break;

        case keyword_de:
            dst16 = &reg.wr.de;
            break;

        case keyword_hl:
            dst16 = &reg.wr.hl;
            break;
        
        case keyword_deref_u16:
            addr  = *(code+1) << 0;
            addr += *(code+2) << 8;
            dst8 = peek8ptr(addr);
            break;

        case keyword_deref_hl:
            addr = reg.wr.hl;
            dst8 = peek8ptr(addr);
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("unknown keyword");
        }

        switch (op->words[2]) {
        case keyword_a:
            *dst8 = reg.br.a;
            break;

        case keyword_u8:
            *dst8 = *(code+1);
            break;

        case keyword_u16:
            d16  = *(code+1) << 0;
            d16 += *(code+2) << 8;
            *dst16 = d16;
            break;

        case keyword_deref_hl:
            /*debug_var("x", reg.wr.hl);*/
            d8 = peek8(reg.wr.hl);
            /*debug_var("x", d8);*/
            *dst8 = d8;
            break;

        default:
            Keyword_repr(op->words[2]);
            die("other");
        }

    } else if (op->words[0] == keyword_ldh) {
        switch (op->words[1]) {
        case keyword_deref_u8:
            d8 = *(code+1);
            addr = 0xff00 + d8;
            dst8 = peek8ptr(addr);
            break;

        default:
            Keyword_repr(op->words[1]);
            die("other");
        }

        switch (op->words[2]) {
            case keyword_a:
                *dst8 = reg.br.a;
                break;

        default:
            Keyword_repr(op->words[2]);
            die("other");
        }

    } else if ((op->words[0] == keyword_inc) ||  (op->words[0] == keyword_dec )) {
        int step = (op->words[0] == keyword_inc) ? 1 : -1;
        switch (op->words[1]) {
        case keyword_a:
            reg.br.a += step;
            break;

        case keyword_b:
            reg.br.b += step;
            break;

        case keyword_c:
            reg.br.c += step;
            break;

        case keyword_d:
            reg.br.d += step;
            break;

        case keyword_e:
            reg.br.e += step;
            break;

        case keyword_h:
            reg.br.h += step;
            break;

        case keyword_l:
            reg.br.l += step;
            break;

        case keyword_hl:
            reg.wr.hl += step;
            break;

        case keyword_sp:
            reg.wr.sp += step;
            break;

        default:
            Opcode_repr(op);
            die("default");
        }

    } else if (op->words[0] == keyword_jp || op->words[0] == keyword_jr) {
        if (op->words[0] == keyword_jp) {
            addr  = *(code+1) << 0;
            addr += *(code+2) << 8;
        } else {
            r8  = *(code+1) << 0;
            addr = reg.wr.pc + r8;
        }
        /*debug_var("x", addr);*/
        /*debug_var("d", op->num_operands);*/
        if (op->num_operands == 2) {
            int z = flag_z(reg.br.f);
            int nz = !z;
            /*debug_var("x", reg.br.f);*/
            /*debug_var("x", flag_mask_z);*/
            switch (op->words[1]) {
            case keyword_z:
                if (z)
                    reg.wr.pc = addr;
                break;

            case keyword_nz:
                if (nz)
                    reg.wr.pc = addr;
                break;

            default:
                Keyword_repr(op->words[1]);
                die("other");
            }
        } else {
            reg.wr.pc = addr;
        }

    } else if (op->words[0] == keyword_add) {
        switch (op->words[1]) {
        case keyword_hl:
            dst16 = &reg.wr.hl;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("other");
        }

        switch (op->words[2]) {
        case keyword_bc:
            *dst16 = reg.wr.bc;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[2]);
            die("other");
        }

    } else if (op->words[0] == keyword_sub) {
        /*ere;*/
        switch (op->words[1]) {
        case keyword_b:
            if (reg.br.a < reg.br.b)
                reg.br.f = 1;
            else
                reg.br.f = 0;
            reg.br.a -= reg.br.b;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("other");
        }

    } else if (op->words[0] == keyword_xor) {
        /*ere;*/
        /*Opcode_repr(op);*/
        switch (op->words[1]) {
        case keyword_a:
            reg.br.a ^= reg.br.a;
            break;

        case keyword_deref_hl:
            d8 = peek8(reg.wr.hl);
            reg.br.a ^= d8;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("other");
        }

    } else if (op->words[0] == keyword_ldi || op->words[0] == keyword_ldd) {
        int step = op->words[0] == keyword_ldi ? 1 : -1;

        switch (op->words[1]) {
        case keyword_deref_hl:
            addr = reg.wr.hl;
            dst8 = peek8ptr(addr);
            reg.wr.hl += step;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("unknown keyword");
        }

        switch (op->words[2]) {
        case keyword_a:
            *dst8 = reg.br.a;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[2]);
            die("unknown keyword");
        }

    } else {
        Opcode_repr(op);
        die("unknown opcode");
    }

    if (op->flags.z == 'z') {
        int z;
        switch (op->words[1]) {
        case keyword_a:
            z = !reg.br.a;
            break;

        case keyword_b:
            z = !reg.br.b;
            break;

        case keyword_c:
            z = !reg.br.c;
            break;

        case keyword_d:
            z = !reg.br.d;
            break;

        case keyword_e:
            z = !reg.br.e;
            break;

        case keyword_h:
            z = !reg.br.h;
            break;

        case keyword_l:
            z = !reg.br.l;
            break;

        case keyword_deref_hl:
            z = !reg.br.a;
            break;

        default:
            Opcode_repr(op);
            Keyword_repr(op->words[1]);
            die("other");
        }

        if (z)
            reg.br.f |= flag_mask_z;
        else
            reg.br.f &= ~flag_mask_z;
    }


    switch (op->words[0]) {
    case keyword_jp:
    case keyword_jr:
    case keyword_call:
    case keyword_ret:
    case keyword_reti:
    case keyword_rst:
        break;
    default:
        reg.wr.pc += op->bytes;
    }
}


void
example_program(void)
{
    print_line_prefix();
    eval_string("jp $be00 $ef +", true);

    print_line_prefix();
    eval_string("nop", true);

    print_line_prefix();
    eval_string("ld a $ff", true);

    print_line_prefix();
    eval_string("inc b", true);

    print_line_prefix();
    eval_string("sub b", true);

    print_line_prefix();
    eval_string("dec b", true);

    print_line_prefix();
    eval_string("jp nz $100", true);

    print_line_prefix();
    eval_string("dec a", true);
}


int
str_eq(const char *s1, const char *s2)
{
    return !strcmp(s1, s2);
}


int
str_ends_with(const char *s, const char *suffix)
{
    return true;
}


int
main(int argc, char **argv)
{
    char line_buf[512] = "";
    FILE *f;

    puts("");
    init();

    global.echo_bytes = false;

    /*example_program();*/

    if (argc != 2) {
        die("invalid arguments");
    }

    argv += 1;
    while (*argv) {
        if (str_eq("-", *argv)) {
            f = stdin;
        } else {
            if (!(f = fopen(*argv, "r")))
                die("open failed");

            if (str_ends_with(*argv, ".gb")) {
                long int filesize = 0;

                fseek(f, 0, SEEK_END);
                filesize = ftell(f);
                /*debug_var("d", filesize);*/
                assert(filesize == 0x8000);

                fseek(f, 0, SEEK_SET);
                /*debug_var("zu", ftell(f));*/
                fread(memory, 0x8000, 1, f);
                /*debug_var("zu", ftell(f));*/
                assert(filesize == ftell(f));

                if (fclose(f) == EOF)
                    die("close rom failed");

                global.reading_rom = true;
            }
        }
        argv += 1;
    }


    print_header();

    if (global.reading_rom) {
        int i = 0;
        int limit = 10;
        for (;;) {
            u8 code[3] = {0};
            print_line_prefix();
            code[0] = peek8(reg.wr.pc + 0);
            code[1] = peek8(reg.wr.pc + 1);
            code[2] = peek8(reg.wr.pc + 2);

            eval(code, true);

            if (i++ == limit)
                die("step");
        }
    } else {
        for (;;) {
            print_line_prefix();
            if(fgets(line_buf, sizeof line_buf, f) == NULL) {
                printf("\n\nEOF\n\n");
                return 0;
            }
            if (line_buf[0] == '\n')
                printf("\n");
            eval_string(line_buf, f != stdin);
        }
    }

    puts("");
    return 0;
}

