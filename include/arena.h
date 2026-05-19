#ifndef ARENA_H
#include <stddef.h>
#include <stdint.h>

typedef struct ArenaBlock ArenaBlock;

typedef struct ArenaBlock
{
    uint8_t* mem;
    size_t used;
    size_t size;
    ArenaBlock* next;
} ArenaBlock;

typedef struct Arena
{
    ArenaBlock* head;
} Arena;

void arena_init(Arena* arena);
void* arena_alloc(Arena* arena, size_t size);
void arena_free(Arena* arena);

#endif