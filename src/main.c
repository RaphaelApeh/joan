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
    "\t$ joan ./main.jx\n"    
    );
}

int main(int argc, char** argv)
{
    char* filename = argv[1];
    char* source = read_file(filename);
    Jn_program_init();
    Jn_exec_program(source);
    Jn_program_close();
    return 0;
}
// int main(int argc, char** argv)
// {
//     if (argc != 2)
//     {
//         usage();
//         return 1;
//     }
//     char* filename = argv[1];
//     char* filecontent = read_file(filename);
//     joan_lexer_t l;
//     Arena arena;
//     arena_init(&arena);
//     J_init_lexer(&l, filecontent);
//     joan_parser_t* p = jn_init_parser(&l);
//     advance_parser(p);
//     struct Chuck chuck;
//     chuck.env = init_env(NULL);
//     p->arena = &arena;
//     chuck_init(&chuck);
//     JnVM vm = {0};
//     vm.p = p;
//     vm.frame_count = 0;
//     while(p->curr.type != TOKEN_EOF)
//     {
//         AST* stmt = parse_stmt(p);
//         compile(stmt, &chuck);
//         //write_chuck(&chuck, OP_POP);
//     }
//     write_chuck(&chuck, OP_END);
//     vm.chuck = &chuck;
//     vm.ip = chuck.code;
//     vm.sp = vm.stack;
//     vm.global = chuck.env;
//     vm.env = vm.global;
//     set_functions(chuck.env);
//     InterpretResult i = vm_run(&vm);
//     if (i == INTERPRET_RUNTIME_ERROR)
//         goto end;
//     free(filename);
//     arena_free(&arena);
//     free(p);
//     return 0;
//     end:
//         free(filename);
//         free(filecontent);
//         arena_free(&arena);
//         free(p);
//         free(vm.global);
//         free(chuck.constants);
//         free(chuck.code);
//         free(vm.sp);
//         return 1;
// }