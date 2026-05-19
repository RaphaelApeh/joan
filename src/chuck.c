#include <stdint.h>
#include <stdlib.h>
#include "chuck.h"

void chuck_init(Chuck* chuck)
{
    chuck->count = 0;
    chuck->ident_count = 0;
    chuck->ident_capacity = 100;
    chuck->capacity = 100;
    chuck->constants_count = 0;
    chuck->constants_capacity = 100;
    chuck->code = malloc(sizeof(uint8_t) * chuck->capacity);
    chuck->constants = malloc(sizeof(Object *) * chuck->constants_capacity);
    chuck->idents = malloc(sizeof(char *) * chuck->ident_capacity);
}


int add_ident(Chuck* chuck, char* ident)
{
    if (NULL == chuck) return -1;
    if (chuck->ident_count >= chuck->ident_capacity)
    {
        chuck->ident_capacity *= 2;
        chuck->idents = realloc(chuck->idents, sizeof(char *) * chuck->ident_capacity);
    }
    chuck->idents[chuck->ident_count] = ident;
    return chuck->ident_count++;
}

void write_chuck(Chuck* chuck, uint8_t byte)
{
    if (NULL == chuck)
        return;
    if (chuck->count >= chuck->capacity)
    {
        chuck->capacity *= 2;
        chuck->code = realloc(
            chuck->code, chuck->capacity
        );
    }
    chuck->code[chuck->count++] = byte;
}

int add_constant(Chuck* chuck, Object* object)
{
    if (NULL == chuck)
        return -1;
    if (chuck->constants_count >= chuck->constants_capacity)
    {
        chuck->capacity *= 2;
        chuck->constants = realloc(
            chuck->constants,
            sizeof(Object *) * chuck->capacity
        );
    }
    chuck->constants[chuck->constants_count] = object;
    return chuck->constants_count++;
}

int current_offset(Chuck* chuck)
{
    return chuck->count;
}

int emit_jump(Chuck* chuck, uint8_t instrction)
{
    write_chuck(chuck, instrction);
    write_chuck(chuck, 0xff);
    write_chuck(chuck, 0xff);
    return chuck->count - 2;
}

void patch_jump(Chuck* chuck, int offset)
{
    int jump = chuck->count - offset - 2;
    chuck->code[offset] = (jump >> 8) & 0xff;
    chuck->code[offset + 1] = jump & 0xff;
}

void emit_loop(Chuck* chuck, int loop_start)
{
    write_chuck(chuck, OP_LOOP);
    int offset = chuck->count - loop_start + 2;
    write_chuck(chuck, (offset >> 8) & 0xff);
    write_chuck(chuck, offset & 0xff);
}