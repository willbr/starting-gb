#include <stdio.h>
#include <ctype.h>
#include <assert.h>

typedef unsigned int uint;

#define ESC "\x1b"
#define RED_TEXT "31"
#define YELLOW_TEXT "33"
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
    while ((*src != '\0') && !isspace(*src) && n--) {
        *dst++ = *src++;
        i += 1;
    }
    i += 1;
    *dst = '\0';
    return i;
}

int
main(int argc, char **argv)
{
    char line[256] = "ld a 4";
    char word[64] = "";
    char *word_end = &word[64];
    char *in = line;

    puts("hi");

    debug_var("s", line);

    while (*in != '\0') {
        in += read_token(word, in, 63);
        chomp(&in, ' ');
        debug_var("s", word);
    }

    if (*in != 0)
        die("expected end of line");


    puts("\n");
    puts(" a  f    b  c  d  e  h  l  [hl] b:offset instruction");
    puts(" -- ---- -- -- -- -- -- --  --  00:0100  jp $150");
    puts(" -- ---- -- -- -- -- -- --  --  00:0150  ld a 4");
    puts(" 04 ---- -- -- -- -- -- --  --  00:0151  inc a");
    puts(" 05 ---- -- -- -- -- -- --  --  00:0152  nop");
    puts("\n");

    puts("bye");
    return 0;
}

