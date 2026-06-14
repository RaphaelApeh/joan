#include <string.h>
#include "Joan.h"
#include "repl.h"

int _JN_INIT_PROGRAM = false;

enum { INCOMPLETE, OK};

typedef struct {
    int braces, parens, brackets;
} repl_state;

static repl_state repl_s = {0};

#define MAX_REPL_BUFF 1 << 10
static char buffer[MAX_REPL_BUFF] = {0};
static int buffer_count = 0;

struct Command parse_args(char** args, int argc)
{
    struct Command c = {0};
    if (argc == 0) // No Arguments
    {
        c.type = C_REPL;
        return c;
    }
    char* arg = args[0];
    if (strcmp(arg, "--help") == 0 || strcmp(arg, "-h") == 0)
    {
        c.type = C_HELP;
    } else if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0)
        c.type = C_VERSION;
    else if (strcmp(arg, "-r") == 0)
    {
        if (argc < 1)
        {
            c.type = C_ERROR;
            return c;
        }
        c.type = C_RUN_REPL;
    }
    else {
        c.type = C_RUN;
    }
    return c;
}

static int parse_buffer(char* str)
{
    repl_s.braces = 0;
    repl_s.brackets = 0;
    repl_s.parens = 0;
    int in_string = 0;
    for (char* p = str; *p; p++)
    {
        char c = *p;

        if (c == '"' && (p == str || p[-1] != '\\'))
        {
            in_string = !in_string;
            continue;
        }
        if (in_string) continue;

        switch (c)
        {
            case '{': repl_s.braces++; break;
            case '}': repl_s.braces--; break;
            case '(': repl_s.parens++; break;
            case ')': repl_s.parens--; break;
            case '[': repl_s.brackets++; break;
            case ']': repl_s.brackets--; break;
        }
    }
    if (repl_s.braces > 0 || repl_s.parens > 0 || repl_s.brackets > 0)
        return INCOMPLETE;
    return OK;
}

JN_API void Jn_repl(void)
{
    char line[256];
    if  (_JN_INIT_PROGRAM)
    {
        fprintf(stderr, 
            "Joan  " JOAN_VERSION "\n"
            "!exit to exit for shell.\n"
            "!help for help info.\n"
        );
        fprintf(stderr, "Entering ....\n");
        Jn_program_init();
    } else {
        fprintf(stderr, 
        "!exit to exit shell.\n"
        );
    }
    for (;;)
    {
        fprintf(stderr, buffer_count == 0 ? ">>> " : "... ");
        if (!fgets(line, sizeof(line), stdin))
            break;
        strcat(buffer, line);
        buffer_count += strlen(line);
        if (strncmp(line, "!exit", 5) == 0) break;
        int type =  parse_buffer(buffer);
        if (type == INCOMPLETE)
            continue;
        
        int exit_code = Jn_exec_REPL(buffer);
        buffer[0] = 0;
        buffer_count = 0;
        if (exit_code < 0)
        {
            // DO something
        }
    }
    if (_JN_INIT_PROGRAM)
        Jn_program_close();
    fprintf(stderr, "Exit ....\n");
}