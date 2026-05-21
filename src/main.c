#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "chuck.h"
#include "arena.h"
#include "builtins/function.h"

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


int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("Error expected a file.\n");
        return 1;
    }
    char* filename = argv[1];
    char* filecontent = read_file(filename);
    lexer l;
    Arena arena;
    arena_init(&arena);
    init_lexer(&l, filecontent);
    parser* p = init_parser(&l);
    advance_parser(p);
    struct Chuck chuck;
    chuck.env = p->env;
    p->arena = &arena;
    chuck_init(&chuck);
    VM vm = {0};
    vm.p = p;
    while(p->curr.type != TOKEN_EOF)
    {
        AST* stmt = parse_stmt(p);
        compile(stmt, &chuck);
        //write_chuck(&chuck, OP_POP);
    }
    write_chuck(&chuck, OP_RETURN);
    vm.chuck = &chuck;
    vm.ip = chuck.code;
    vm.sp = vm.stack;
    vm.global = chuck.env;
    vm.env = vm.global;
    set_functions(chuck.env);
    InterpretResult i = vm_run(&vm);
    if (i == INTERPRET_RUNTIME_ERROR)
        goto end;
    free(filename);
    arena_free(&arena);
    free(p);
    return 0;
    end:
        free(filename);
        free(filecontent);
        arena_free(&arena);
        free(p->env);
        free(p);
        free(vm.global);
        free(chuck.constants);
        free(chuck.code);
        free(vm.sp);
        return 1;
}