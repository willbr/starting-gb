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


struct settings {
    int echo_bytes;
} global;

union registers reg;
union registers prev_reg;

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
    return 0xff;
}


void
init(void)
{
    assert(sizeof u8  == 1);
    assert(sizeof u16 == 2);
    assert(sizeof u32 == 4);
    assert(sizeof u64 == 8);

    reg.wr.af = 0;
    reg.wr.bc = 0;
    reg.wr.de = 0;
    reg.wr.hl = 0;

    reg.wr.pc = 0x100;
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
assemble(u8 *code, const char *cmd, const char *args)
{
    char arg1[64] = "";
    char arg2[64] = "";
    const char *in = args;
    u16 addr = 0;
    u8 v8 = 0;

    in += read_token(arg1, in, sizeof(arg1));
    chomp(&in, ' ');

    in += read_token(arg2, in, sizeof(arg2));
    chomp(&in, ' ');

    /*debug_var("s", arg1);*/
    /*debug_var("s", arg2);*/

    if (*in != '\0' && *in != '\n') {
        debug_var("d", *in);
        debug_var("s", arg1);
        debug_var("s", arg2);
        die("expected end of line");
    }

    if (*cmd == '\0') {
        die("Opps");
    } else if (!strcmp("jp", cmd)) {
        if (parse_addr(&addr, arg1))
            die("failed");
        /*debug_var("x", addr);*/
        *(code+0) = 0xc3;
        *(code+1) = (u8)(addr >> 8);
        *(code+2) = (u8)(addr >> 0);
    } else if (!strcmp("ld", cmd)) {
        if (!strcmp("a", arg1)) {
            if (parse_u8(&v8, arg2))
                die("failed");
            *(code+0) = 0x3e;
            *(code+1) = v8;
        } else {
            debug_var("s", arg1);
            debug_var("s", arg2);
            puts("LOAD");
            die("todo");
        }
    } else if (!strcmp("inc", cmd)) {
        if (!strcmp("a", arg1)) {
            *(code+0) = 0x3c;
        } else {
            die("INC");
        }
    } else if (!strcmp("nop", cmd)) {
        if (arg1[0] != '\0')
            die("too many args");

        if (arg2[0] != '\0')
            die("too many args");

        *(code+0) = 0;
    } else {
        debug_var("s", cmd);
        die("Opps");
    }

    /*debug_var("x", *(code+0));*/
    /*debug_var("x", *(code+1));*/
    /*debug_var("x", *(code+2));*/
    /*debug_var("x", *(code+3));*/
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


void
eval_string(char *x)
{
    char word[64] = "";
    char *in = x;
    u8 code[3] = {0};
    int i = 0;
    char *spacer = NULL;
    Opcode *op = NULL;

    printf("%s\n", x);
    /*debug_var("s", x);*/

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
main(int argc, char **argv)
{
    char line_buf[512] = "";

    puts("");
    init();

    global.echo_bytes = false;

    print_header();

    print_line_prefix();
    eval_string("jp $150");

    for (;;) {
        print_line_prefix();
        if(fgets(line_buf, sizeof line_buf, stdin) == NULL)
            die("EOF");
        eval_string(line_buf);
    }

    puts("");
    return 0;
}

