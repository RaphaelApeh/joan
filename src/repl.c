#include <string.h>
#include <stdlib.h>
#include "Joan.h"
#include "repl.h"
#define OPTPARSE_IMPLEMENTATION
#include "vendor/optparse.h"

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
    int opt;
    if (argc == 0) // No Arguments
    {
        c.type = C_REPL;
        return c;
    }
    struct optparse_long longopts[] = {
        {"help", 'h', OPTPARSE_NONE},
        {"version", 'v', OPTPARSE_NONE},
        {"repl", 'r', OPTPARSE_OPTIONAL},
        {"file", 'f', OPTPARSE_REQUIRED},
        {"debug", 'd', OPTPARSE_NONE},
        {"command", 'c', OPTPARSE_NONE},
        {"inter", 'i', OPTPARSE_REQUIRED},
        {0}
    };
    struct optparse opts;
    optparse_init(&opts, args);
    while ((opt = optparse_long(&opts, longopts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'h':
                c.type = C_HELP;
                break;
            case 'v':
                c.type = C_VERSION;
                break;
            case 'r':
                c.type = C_REPL;
                break;
            case 'f':
                c.type = C_RUN;
                c.filename = opts.optarg;
                break;
            case 'i':
                c.type = C_RUN_REPL;
                c.filename = opts.optarg;
                break;
            case '?':
                fprintf(stderr, "Invalid input.");
                exit(EXIT_FAILURE);
            default:
                c.type = C_ERROR;
        }
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