#include <Joan.h>
#include "repl.h"
#include "lexer.h"
#include "token.h"
#include "env.h"


static void usage(void)
{
    fprintf(stderr, 
    "Usage: joan [file] [..options]\n"
    "Options: \n"
    "-v --version:      output joan version.\n"
    "-f --file:         execute script file.\n"
    "-r --repl:         REPL.\n"
    "-c --command:      execute a program string.\n"
    "-i --iterative:    run program into repl.\n"
    "-t --token         print the tokens from a file(use only for debugging.).\n"
    "-h --help:         output help information.\n\n"
    "Examples: \n"
    "\t$ joan\n"
    "\t$ joan ./main.jt\n"
    "\t$ joan -c \"printf(\"Hello World\");\"\n"
    "\t$ joan -i ./main.jt\n"
    "\t$ joan --file main.jt.\n"
    );
}

static void print_token(Jn_State* state, char* filename)
{
    if (!filename) return;
    Jn_Lexer l = {0}; Jn_Buffer b; Jn_Token t;
    Jn_buff_init(&b);
    if (Jn_read_file(&b, filename) != 0)
    {
        fprintf(stderr, "File not found \"%s\".\n", filename);
        exit(EXIT_FAILURE);
    }
    jn_lexer_init(&l, b.data, filename);
    while (Jn_get_next_token(&l, &t))
    {
        // TODO: token_string char array
        printf("[TOKEN=%d]: ", t.type);
        switch (t.type)
        {
            case TOK_CHAR:
                printf("['%s']\n", t.lexeme); break;
            case TOK_STRING:
                printf("[\"%s\"]\n", t.lexeme); break;
            case TOK_NEWLINE:
                printf("[\\n]\n"); break;
            default:
                printf("[%s]\n", t.lexeme);
        }
    }
    switch (t.type)
    {
        case TOK_ERROR:
            fprintf(stderr, "[Error:\"%s\":%lld:%lld]: %s\n", l.filename, l.line, l.column, t.lexeme);
            break;
        case TOK_EOF:
            fprintf(stderr, "[EOF]: program ended.\n");
        default:
            break;
    }
    Jn_buff_clear(&b);
}

static void version(void)
{
    fprintf(stdout, 
    "Joan v" JOAN_VERSION " at " JOAN_BRANCH
    );
}

int main(int argc, char** argv)
{
    char* source = NULL;
    char** nw_argv = argc > 0 ? argv + 1 : argv;
    int nw_argc = argc > 0 ? argc - 1 : 0;
    int exit_code;
    struct Command c = parse_args(argv, argc);
    Jn_State state = {0};
    switch (c.type)
    {
        case C_ERROR:
            usage(); return -1;
        case C_HELP:
            usage(); return 0;
        case C_VERSION:
            version(); return 0;
        case C_SCMD:
        {
            goto scmd;
        }
        case C_REPL:
        {
            goto repl;
        }
        case C_ITERATIVE:
        {
            goto interative;
        }
        case C_RUN:
        {
            goto execute;
        }
        case C_TOKEN:
            goto token;
        default:
            usage(); return 0;
    }
    return 0;
    repl:
        Jn_program_init(&state, nw_argv, nw_argc);
        Jn_repl(&state);
        Jn_program_close(&state);
        return 0;
    token:
        Jn_program_init(&state, nw_argv, nw_argc);
        print_token(&state, c.filename);
        Jn_program_close(&state);
        return 0;
    scmd:
        Jn_program_init(&state, nw_argv, nw_argc);
        exit_code = Jn_exec_string(&state, c.cmd_string);
        Jn_program_close(&state);
        return exit_code;
    execute:
        Jn_program_init(&state, nw_argv, nw_argc);
        if (!c.filename)   return -1;
        exit_code = Jn_execute_main(&state, c.filename);
        Jn_program_close(&state);
        return exit_code;
    interative:
        Jn_program_init(&state, nw_argv, nw_argc);
        if (!c.filename) return -1;
        Jn_run_iterative(&state, c.filename);
        Jn_program_close(&state);
        return 0;
}
