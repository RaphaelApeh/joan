#include "Joan.h"
#include <stdlib.h>
#include <stdio.h>

// #include "lexer.h"
// #include "parser.h"
// #include "vm.h"
// #include "arena.h"
// #include "builtins/function.h"
// #include "opcode.h"
// #include "emit.h"

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
    "-h --help output help information\n\n"
    "Examples: \n"
    "\t$ joan ./main.jt\n"    
    );
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        usage(); return 1;
    }
    char* filename = argv[1];
    char* source = read_file(filename);
    Jn_program_init();
    Jn_exec_program(source);
    Jn_program_close();
    return 0;
}
