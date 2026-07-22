#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <Joan.h>
#include "repl.h"

#define OPTPARSE_IMPLEMENTATION
#include "vendor/optparse.h"

enum { INCOMPLETE, OK};

static struct { int braces, parens, brackets;} repl_s = {0};

#define MAX_REPL_BUFF 1 << 10
static char buffer[MAX_REPL_BUFF] = {0};
static int buffer_count = 0;

struct Command parse_args(char** args, int argc)
{
    struct Command c = {0};
    int opt;
    if (argc == 1) // No Arguments
    {
        c.type = C_REPL;
        return c;
    }
    if (argc == 2 && *(args[1]) != '-' )
    {
        c.filename = args[1];
        c.type = C_RUN;
        return c;
    }
    struct optparse_long longopts[] = {
        {"help", 'h', OPTPARSE_NONE},
        {"version", 'v', OPTPARSE_NONE},
        {"repl", 'r', OPTPARSE_OPTIONAL},
        {"file", 'f', OPTPARSE_REQUIRED},
        {"debug", 'd', OPTPARSE_NONE},
        {"command", 'c', OPTPARSE_REQUIRED},
        {"interative", 'i', OPTPARSE_REQUIRED},
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
            case 'd':
                c.debug = true; break;
            case 'c':
                // TODO
                printf("Command: %s\n", opts.optarg);
                break;
            case 'i':
                c.type = C_ITERATIVE;
                c.filename = opts.optarg;
                break;
            case '?':
                fprintf(stderr, "%s: %s\n\n", args[0], opts.errmsg);
                c.type = C_ERROR;
                break;
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

#if __WIN32
#define _CFMT "CTL-Z"
#else
#define _CFMT "CTL-D"
#endif

static void print_help(void)
{
    // TODO: add a better help msg
    fprintf(stdout, 
    _CFMT " Exit shell.\n"
    ".help Show this help message.\n"
    );
}

JN_API void Jn_run_iterative(J_State* state, const char* filename)
{
    int exit_code = Jn_execute_main(state, filename);
    if (exit_code != 0)
    {
        // TODO
    }
    Jn_repl(state);
}

JN_API void Jn_repl(J_State* state)
{
    char line[256];
    fprintf(stderr, 
        "Welcome to Joan v" JOAN_VERSION "\n"
        "\"" _CFMT "\" to exit shell.\n"
        "\"!help\" for help info.\n"
    );
    for (;;)
    {
        fprintf(stderr, buffer_count == 0 ? ">>> " : "... ");
        if (!fgets(line, sizeof(line), stdin))
        {
            printf("Exiting....\n");
            break;
        }
        strcat(buffer, line);
        buffer_count += strlen(line);
        if (strncmp(line, "!help", 5) == 0) 
        {
            print_help();
            // clean buffer
            buffer[0] = 0;
            buffer_count = 0;
            continue;
        }
        int type =  parse_buffer(buffer);
        if (type == INCOMPLETE)
            continue;
        
        int exit_code = Jn_exec_REPL(state, buffer);
        buffer[0] = 0;
        buffer_count = 0;
        if (exit_code < 0 || exit_code > 0)
        {
            #ifdef JOAN_DEBUG
                printf("Eixt code: %d\n", exit_code);
            #endif
            //  exit() function
            // break;
        }
    }
}