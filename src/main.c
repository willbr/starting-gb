#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define true  1
#define false 0


#include "opcodes.h"

typedef unsigned int uint;

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char      i8;
typedef signed short     i16;
typedef signed int       i32;
typedef signed long long i64;

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
                u16 pc;
        } br;

        struct {
                u16 bc;
                u16 de;
                u16 hl;
                u16 af;
                u16 pc;
        } wr;

};


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
    X(fn)

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


typedef struct Stack {
    char data[512];
    char *next;
    char *last;
    int length;
} Stack;


typedef int (*fnptr)(Stack *);

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
    Dict dict;
} global;


typedef struct Object {
    Type type;
    char name[TOKEN_LEN];
    union {
        i32 i;
        fnptr fn;
    };
} Object;


/* ##### */

void DictElem_repr(DictElem *e);
void Dict_init(Dict *d);
void Dict_repr(Dict *d);
int  Dict_alloc_word(Dict *d, const char *n, Type t);
int  Dict_add_fn(Dict *d, const char *n, fnptr fn);

void Stack_init(Stack *s);
int Stack_read_string(u8 *c, u8 *w, u8 *wlen);
void Stack_push_i32(Stack *s, i32 i);
int Stack_push_u8(Stack *s, u8 i);
int Stack_read_i32(u8 *c, i32 *i);
int Stack_pop_i32(Stack *s, i32 *i);
int Stack_push_object(Stack *s, Object *o);
int Stack_push_string(Stack *s, const char *o);
int Stack_push_tag(Stack *s, Type t);
int Stack_pop_object(Stack *s, Object *o);
void Stack_repr(Stack *s);
int Stack_add(Stack *s);

int u8_from_object(u8 *i, Object *o);
void Object_repr(Object *o);

void chomp(char **in, char c);
int read_token(char *dst, const char *src, size_t n);

u8 peek8(u16 addr);
void init(void);
void print_header(void);
void print_line_prefix(void);
int parse_number(i32 *n, const char *arg);
int parse_addr(u16 *addr, const char *arg);
int parse_u8(u8 *n, const char *arg);

int lookup_word(Object *o, char *w);

void eval_rpn(Stack *s, const char *x);
void eval_string(char *x, int echo);

void assemble(u8 *code, const char *cmd, const char *args);
void eval(u8 *code);

/* ##### */


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
    fprintf(stderr, "(Object ");
    switch (o->type) {
    case type_i32:
        fprintf(stderr, "i32 0x%08x)\n", o->i);
        break;

    case type_fn:
        fprintf(stderr, "fn 0x%p)\n", o->fn);
        break;

    case type_r8:
        fprintf(stderr, "r8 %s)\n", o->name);
        break;

    case type_string:
        fprintf(stderr, "string \"%s\")\n", o->name);
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

    *s->next++ = i;
    s->length += 1;

    return 0;
}


void
Stack_push_i32(Stack *s, i32 i)
{
    if ((s->next + 5) > s->last)
        die("stack overflow");

    *s->next++ = (i >> 24) & 0xff;
    *s->next++ = (i >> 16) & 0xff;
    *s->next++ = (i >>  8) & 0xff;
    *s->next++ = (i >>  0) & 0xff;
    *s->next++ = type_i32;

    s->length += 1;
}


int
Stack_read_string(u8 *c, u8 *w, u8 *wlen)
{
    /*ere;*/
    assert(*c == type_string);
    /*debug_var("p", w);*/
    *wlen = *(--c);
    /*debug_var("d", *wlen);*/
    c -= *wlen + 1;
    /*debug_var("s", c);*/
    /*debug_var("c", *c);*/
    /*ere;*/
    strcpy(w, c);
    /*ere;*/
    /*debug_var("s", w);*/
    /*die("ere");*/
    /*ere;*/
    return 0;
}


int
Stack_read_i32(u8 *c, i32 *i)
{
    assert(*c == type_i32);
    /*debug_var("02x", *(c - 0));*/
    /*debug_var("02x", *(c - 1));*/
    /*debug_var("02x", *(c - 2));*/
    /*debug_var("02x", *(c - 3));*/
    /*debug_var("02x", *(c - 4));*/
    *i = 0;
    *i += *(--c) << 0;
    *i += *(--c) << 8;
    *i += *(--c) << 16;
    *i += *(--c) << 24;
    /*debug_var("04x", *i);*/
    return 0;
}


int
Stack_pop_i32(Stack *s, i32 *i)
{
    if (s->next <= s->data)
        die("stack underflow");

    if (Stack_read_i32(s->next - 1, i))
        die("read failed");

    /*debug_var("x", *i);*/

    s->next -= 5;
    s->length -= 1;

    return 0;
}


int
Stack_push_object(Stack *s, Object *o)
{
    /*ere;*/
    /*Object_repr(o);*/
    switch (o->type) {
    case type_r8:
        /*ere;*/
        /*Stack_repr(s);*/
        Stack_push_string(s, o->name);
        /*ere;*/
        /*Stack_repr(s);*/
        Stack_push_tag(s, o->type);
        /*ere;*/
        /*Stack_repr(s);*/
        break;

    default:
        fprintf(stderr, "%s\n", type_names[o->type]);
        die("ere");
        break;
    }
}


int
Stack_push_string(Stack *s, const char *w)
{
    int wlen = strlen(w);
    u8 *c = s->next;

    /*ere;*/
    /*debug_var("d", wlen);*/
    /*debug_var("d", s->length);*/

    if (wlen > 255)
        die("string is longer than 255");

    if ((c + wlen + 2) > s->last)
        die("overflow");

    strcpy(c, w);
    /*debug_var("s", s->next);*/

    s->next += wlen + 1;

    if (Stack_push_u8(s, wlen))
        die("error");
    s->length -= 1;

    if (Stack_push_tag(s, type_string))
        die("error");

    if (false) {
        for (int i = 0; i < (wlen + 3); i += 1)
            debug_var("x", *(c + i));
    }

    s->length += 1;
    /*ere;*/
    /*debug_var("d", s->length);*/
    /*debug_var("d", wlen);*/
    /*debug_var("s", w);*/
    return 0;
}



int
Stack_push_tag(Stack *s, Type t)
{
    if (Stack_push_u8(s, t))
        die("error");

    s->length -= 1;
    return 0;
}


int
Stack_pop_object(Stack *s, Object *o)
{
    char *c = s->next - 1;
    i32 i = 0;
    char w[TOKEN_LEN] = "";
    u8 wlen = 0;

    if (s->next <= s->data)
        die("stack underflow");

    o->type = *c;

    switch (*c) {
    case type_i32:
        if (Stack_read_i32(c, &i)) {
            die("read failed");
        }
        o->i = i;
        c -= sizeof i32;
        break;

    case type_r8:
        c -= 1; /* skip double tag */
        if (Stack_read_string(c, w, &wlen))
            die("read failed");
        strcpy(o->name, w);
        break;

    case type_string:
        if (Stack_read_string(c, w, &wlen))
            die("read failed");
        strcpy(o->name, w);
        break;

    default:
        fprintf(stderr, "missing case: %s\n", type_names[*c]);
        die("todo");
    }

    s->next = c;
    return 0;
}


void
Stack_repr(Stack *s)
{
    char *c = s->next - 1;
    i32 i = 0;
    char w[TOKEN_LEN] = "";
    u8 wlen = 0;

    fprintf(stderr, "\nStack (%d):\n", s->length);
    while(c >= s->data) {
        switch (*c) {
        case type_i32:
            /*ere;*/
            if (Stack_read_i32(c, &i)) {
                die("read failed");
            }
            fprintf(stderr, "- 0x%04x\n", i);
            c -= 5;
            break;

        case type_r8:
            c -= 1; /* skip double tag */
            if (Stack_read_string(c, w, &wlen))
                die("read failed");
            fprintf(stderr, "- r8(%s)\n", w);
            c -= (wlen + 3);
            break;

        case type_string:
            if (Stack_read_string(c, w, &wlen))
                die("read failed");

            fprintf(stderr, "- \"%s\"\n", w);
            /*debug_var("s", w);*/
            c -= (wlen + 3);
            /*debug_var("p", c);*/
            /*ere;*/
            break;

        default:
            ere;
            fprintf(stderr, "type: %s\n", type_names[*c]);
            die("todo");
        }
    }
    fprintf(stderr, "\n");
    /*ere;*/
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

    /*debug_var("x", a);*/
    /*debug_var("x", b);*/
    r = a + b;
    /*debug_var("x", r);*/

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
    return memory[addr];
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

    Dict_init(&global.dict);
    Dict_add_fn(&global.dict, "+", Stack_add);

    Dict_alloc_word(&global.dict, "a", type_r8);
    Dict_alloc_word(&global.dict, "b", type_r8);
    Dict_alloc_word(&global.dict, "c", type_r8);
    Dict_alloc_word(&global.dict, "d", type_r8);
    Dict_alloc_word(&global.dict, "e", type_r8);
    Dict_alloc_word(&global.dict, "h", type_r8);
    Dict_alloc_word(&global.dict, "l", type_r8);
    /*Dict_repr(&global.dict);*/
}


void
print_header(void)
{
    puts(" af   bc   de   hl   [hl] bank:offset instruction");
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

    highlight_diff(reg.wr.af, prev_reg.wr.af);
    printf(" %04x", reg.wr.af);
    highlight_diff(reg.wr.bc, prev_reg.wr.bc);
    printf(" %04x", reg.wr.bc);
    highlight_diff(reg.wr.de, prev_reg.wr.de);
    printf(" %04x", reg.wr.de);
    highlight_diff(reg.wr.hl, prev_reg.wr.hl);
    printf(" %04x", reg.wr.hl);

    /* todo highlight *hl */
    printf("  %02x", peek8(reg.wr.hl));

    /* todo highlight bank */
    printf("  %4s", "rom0");

    printf(":");
    highlight_diff(reg.wr.pc, prev_reg.wr.pc);
    printf("%04x", reg.wr.pc);

    printf("   " RESET);
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

    if (echo)
        printf("%s\n", x);

    in += read_token(word, in, sizeof(word));
    chomp(&in, ' ');

    assemble(code, word, in);
    eval(code);

    if (global.echo_bytes) {
        printf("%38s", "");
        op = &unprefixed[code[0]];

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

            case type_r8:
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


void
eval_rpn(Stack *s, const char *x)
{
    char tok[64] = "";
    const char *in = x;
    i32 l = 0;

    while (*in != '\n' && *in != '\0') {
        in += read_token(tok, in, sizeof(tok));
        /*debug_var("s", tok);*/
        chomp(&in, ' ');
        if (parse_number(&l, tok)) {
            Object o;
            if(lookup_word(&o, tok)) {
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
    /*ere;*/
    /*debug_var("s", args);*/

    Stack s;
    Stack_init(&s);
    /*Stack_repr(&s);*/
    eval_rpn(&s, args);
    /*Stack_repr(&s);*/


    if (*cmd == '\0') {
        die("Opps");
    } else if (!strcmp("jp", cmd)) {
        /*debug_var("d", s.length);*/
        if (s.length == 1) {
            if (Stack_pop_object(&s, &arg1))
                die("pop failed");
            /*Object_repr(&arg1);*/
            assert(arg1.type == type_i32);
            if (arg1.i > 0xffff)
                die("addr too big");
            if (arg1.i < 0x0)
                die("addr too small");
            addr = arg1.i;
            /*debug_var("x", addr);*/
            *(code+0) = 0xc3;
            *(code+1) = (u8)(addr >> 8);
            *(code+2) = (u8)(addr >> 0);
        } else if (s.length == 2) {
            die("ere");
        } else {
            die("ere");
        }
        /*if (parse_addr(&addr, arg1))*/
            /*die("failed");*/
    } else if (!strcmp("ld", cmd)) {
        /*ere;*/
        /*Stack_repr(&s);*/
        assert(s.length == 2);
        /*Stack_repr(&s);*/

        if (Stack_pop_object(&s, &arg2))
            die("pop failed");

        /*ere;*/
        /*Stack_repr(&s);*/
        if (Stack_pop_object(&s, &arg1))
            die("pop failed");
        /*Stack_repr(&s);*/
        /*Object_repr(&arg2);*/
        /*Object_repr(&arg1);*/

        switch (arg1.type) {
        case type_r8:
            switch (arg1.name[0]) {
            case 'a':
                if (u8_from_object(&v8, &arg2))
                    die("cast failed");
                *(code+0) = 0x3e;
                *(code+1) = v8;
                break;
            default:
                die("r8?");
                break;
            }
            break;
        default:
            die("load other");
            break;
        }
    } else if (!strcmp("inc", cmd)) {
        assert(s.length == 1);
        if (Stack_pop_object(&s, &arg1))
            die("pop failed");
        assert(arg1.type == type_r8);
        switch (arg1.name[0]) {
        case 'a':
            *(code+0) = 0x3c;
            break;
        default:
            die("inc");
            break;
        }
    } else if (!strcmp("nop", cmd)) {
        assert(s.length == 0);
        *(code+0) = 0;
    } else {
        die("ere");
        debug_var("s", cmd);
        die("Opps");
    }

    if (true) {
        ere;
        debug_var("x", *(code+0));
        debug_var("x", *(code+1));
        debug_var("x", *(code+2));
        /*debug_var("x", *(code+3));*/
    }
}


void
eval(u8 *code)
{
    u16 addr = 0;
    u8  n = 0;
    /*debug_var("x", *(code+0));*/
    /*debug_var("x", *(code+1));*/
    /*debug_var("x", *(code+2));*/
    /*debug_var("x", *(code+3));*/
    memcpy(&prev_reg, &reg, sizeof(reg));

    if (*code == 0) {
        /* skip */
    } else if (*code == 0xc3) {
        addr = *(code+1) << 8;
        addr += *(code+2);
        /*debug_var("x", addr);*/
        reg.wr.pc = addr;
    } else if (*code == 0x3e) {
        n = *(code+1);
        /*debug_var("x", n);*/
        reg.br.a = n;
    } else if (*code == 0x3c) {
        reg.br.a += 1;
    } else {
        debug_var("x", *code);
        die("todo");
    }
}


int
main(int argc, char **argv)
{
    char line_buf[512] = "";

    puts("");
    init();

    global.echo_bytes = false;

    print_header();

    print_line_prefix();
    eval_string("jp $be00 $ef +", true);

    print_line_prefix();
    eval_string("nop", true);

    print_line_prefix();
    eval_string("ld a $ff", true);

    print_line_prefix();
    eval_string("inc a", true);

    for (;;) {
        print_line_prefix();
        if(fgets(line_buf, sizeof line_buf, stdin) == NULL)
            die("EOF");
        eval_string(line_buf, false);
    }

    puts("");
    return 0;
}

