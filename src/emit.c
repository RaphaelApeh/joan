#include <assert.h>
#include <stdlib.h>
#include "emit.h"
#include "vm.h"
#include "opcode.h"

#define CHECK_CHUCK() assert(chuck != NULL)

int add_ident(Chuck* chuck, char* ident)
{
    CHECK_CHUCK();
    if (chuck->ident_count >= chuck->ident_capacity)
    {
        chuck->ident_capacity *= 2;
        chuck->idents = realloc(chuck->idents, sizeof(char *) * chuck->ident_capacity);
    }
    chuck->idents[chuck->ident_count] = ident;
    return chuck->ident_count++;
}
static void chuck_grow(Chuck* chuck)
{
    chuck->capacity *= 2;
    chuck->code = realloc(
            chuck->code, chuck->capacity
    );
}

void write_chuck(Chuck* chuck, uint8_t byte)
{
    CHECK_CHUCK();
    if (chuck->count >= chuck->capacity)
    {
        chuck_grow(chuck);
    }
    chuck->code[chuck->count++] = byte;
}

void write_chuck_loc(Chuck* chuck, uint8_t byte, int line, int column)
{
    CHECK_CHUCK();
    if (chuck->count >= chuck->capacity)
    {
        chuck_grow(chuck);
    }
    chuck->code[chuck->count] = byte;
    chuck->lines[chuck->count] = line;
    chuck->columns[chuck->count] = column;
    chuck->count++;
}

int add_constant(Chuck* chuck, JnObject* object)
{
    assert(chuck != NULL && object != NULL);
    if (chuck->constants_count >= chuck->constants_capacity)
    {
        chuck->capacity *= 2;
        chuck->constants = realloc(
            chuck->constants,
            sizeof(JnObject *) * chuck->capacity
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
    CHECK_CHUCK();
    write_chuck(chuck, instrction);
    write_chuck(chuck, 0xff);
    write_chuck(chuck, 0xff);
    return chuck->count - 2;
}

void patch_jump(Chuck* chuck, int offset)
{
    CHECK_CHUCK();
    int jump = chuck->count - offset - 2;
    chuck->code[offset] = (jump >> 8) & 0xff;
    chuck->code[offset + 1] = jump & 0xff;
}

void emit_loop(Chuck* chuck, int loop_start)
{
    CHECK_CHUCK();
    write_chuck(chuck, OP_LOOP);
    int offset = chuck->count - loop_start + 2;
    write_chuck(chuck, (offset >> 8) & 0xff);
    write_chuck(chuck, offset & 0xff);
}
