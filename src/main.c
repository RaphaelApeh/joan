#include "Joan.h"
#include <stdlib.h>
#include <stdio.h>
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
    argc -= 1;
    char* filename = NULL;
    char* source = NULL;
    struct Command c = parse_args(new_argv, argc);

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
            filename = new_argv[1];
            if (!filename) return -1;
            source = read_file(filename);
            Jn_program_init();
            Jn_exec_program(source);
            Jn_repl();
            Jn_program_close();
            return 0;
        case C_RUN:
            filename = new_argv[0];
            source = read_file(filename);
            Jn_program_init();
            Jn_exec_program(source);
            Jn_program_close();
            return 0;
    }
    return -1;
}
