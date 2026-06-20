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
    "Usage: joan [options] [file]\n"
    "Options: \n"
    "-v --version output joan version\n"
    "-f --file run a script\n"
    "-r  execute a program and run on repl\n"
    "-h --help output help information\n\n"
    "Examples: \n"
    "\t$ joan ./main.jt\n"
    "\t$ joan -r ./main.jt\n"
    );
}

void version(void)
{
    fprintf(stderr, 
    "Joan " JOAN_VERSION
    );
}

int main(int argc, char** argv)
{
    char** new_argv = argv + 1;
    int new_argc = argc - 1;
    char* source = NULL;
    struct Command c = parse_args(argv, argc);

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
            _JN_INIT_PROGRAM  = true;
            Jn_repl(); 
            return 0;
        }
        case C_RUN_REPL:
            if (!c.filename) return -1;
            Jn_program_init();
            Jn_execute_main(c.filename);
            Jn_repl();
            Jn_program_close();
            return 0;
        case C_RUN:
            Jn_program_init();
            assert(c.filename);
            Jn_execute_main(c.filename);
            Jn_program_close();
            return 0;
    }
    return -1;
}
