#include "Joan.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "repl.h"
#include "env.h"


char* read_file(const char* filename)
{
    FILE* p_file;
    p_file = fopen(filename, "rb");
    if (NULL == p_file)
    {
        perror("Filename does not exists.");
        exit(1);
    }
    fseek(p_file, 0, SEEK_END);
    size_t size = ftell(p_file);
    rewind(p_file);
    char* buf = malloc(sizeof(char) * (size + 1));
    if (NULL == buf)
    {
        perror("memory failed.");
        exit(1);
    }
    fread(buf, 1, size, p_file);
    buf[size] = '\0';
    fclose(p_file);
    return buf;
}

void usage(void)
{
    fprintf(stderr, 
    "Usage: joan [file] [..options]\n"
    "Options: \n"
    "-v --version:      output joan version.\n"
    "-f --file:         execute script file.\n"
    "-r --repl:         REPL.\n"
    "-c --command:      execute a program string.\n"
    "-i --iterative:    run program into repl.\n"
    "-h --help:         output help information.\n\n"
    "Examples: \n"
    "\t$ joan\n"
    "\t$ joan ./main.jt\n"
    "\t$ joan -c \"printf(\"Hello World\");\"\n"
    "\t$ joan -i ./main.jt\n"
    "\t$ joan --file main.jt.\n"
    );
}

void version(void)
{
    fprintf(stdout, 
    "Joan v" JOAN_VERSION
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
        default:
            usage(); return -1;
    }
    return 0;
    repl:
        Jn_program_init(&state);
        Jn_repl(&state);
        Jn_program_close(&state);
        return 0;
    execute:
        Jn_program_init(&state);
        if (!c.filename)   return -1;
        exit_code = Jn_execute_main(&state, c.filename, nw_argv, nw_argc);
        Jn_program_close(&state);
        return exit_code;
    interative:
        Jn_program_init(&state);
        if (!c.filename) return -1;
        Jn_run_iterative(&state, c.filename);
        Jn_program_close(&state);
        return 0;
}
