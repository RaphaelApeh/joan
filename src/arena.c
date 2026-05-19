#include <stdlib.h>
#include "arena.h"

#define ARENA_BSIZE 1024

static ArenaBlock* arena_block_alloc(size_t size)
{
    ArenaBlock* block = malloc(sizeof(ArenaBlock));
    block->used = 0;
    block->size = size;
    block->mem = malloc(size);
    block->next = NULL;
    return block;
}

void arena_init(Arena* arena)
{
    arena->head = arena_block_alloc(ARENA_BSIZE);
    return;
}

void* arena_alloc(Arena* arena, size_t size)
{
    size = (size + 7) & ~7;
    ArenaBlock* block = arena->head;
    if (block->used + size > block->size)
    {
        size_t new_s = size > ARENA_BSIZE ? size : ARENA_BSIZE;
        ArenaBlock* new_b = arena_block_alloc(new_s);
        new_b->next = block;
        arena->head = new_b;
        block = new_b;
    }
    void* ptr = block->mem + block->used;
    block->used += size;
    return ptr;
}
void arena_free(Arena* arena)
{
    ArenaBlock* block = arena->head;
    while (block)
    {
        ArenaBlock* next = block->next;
        free(block->mem);
        free(block);
        block = next;
    }
    arena->head = NULL;
}