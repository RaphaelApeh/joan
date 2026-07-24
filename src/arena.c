/*
src/arena.c

MIT License

Copyright (c) 2026 Raphael Apeh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "arena.h"

#define ARENA_BSIZE 1024

static Jn_ArenaBlock* arena_block_alloc(size_t size)
{
    Jn_ArenaBlock* block = malloc(sizeof(Jn_ArenaBlock));
    block->used = 0;
    block->size = size;
    block->mem = malloc(size);
    block->next = NULL;
    return block;
}

void arena_init(Jn_Arena* arena)
{
    arena->head = arena_block_alloc(ARENA_BSIZE);
    return;
}

void* arena_alloc(Jn_Arena* arena, size_t size)
{
    assert(arena != NULL);
    size = (size + 7) & ~7;
    Jn_ArenaBlock* block = arena->head;
    if (block->used + size > block->size)
    {
        size_t new_s = size > ARENA_BSIZE ? size : ARENA_BSIZE;
        Jn_ArenaBlock* new_b = arena_block_alloc(new_s);
        new_b->next = block;
        arena->head = new_b;
        block = new_b;
    }
    void* ptr = block->mem + block->used;
    block->used += size;
    arena->last_ptr = ptr;
    arena->last_size = size;
    return ptr;
}

// TODO: arena_realloc fix bug
void* arena_realloc(Jn_Arena* arena, void* ptr, size_t old_size, size_t new_size)
{
    assert(old_size < new_size);
    old_size = (old_size + 7) & -7;
    new_size = (new_size + 7) & -7;
    if (NULL == ptr)
        return arena_alloc(arena, new_size);
    // TODO
    if (ptr == arena->last_ptr)
    {
        Jn_ArenaBlock* block = arena->head;
        size_t start = (char *)ptr - (char *)block->mem;
        if (start + new_size <= block->size)
        {
            block->used = start + new_size;
            arena->last_size = new_size;
            return ptr;
        }
    }
    void* new_ptr = arena_alloc(arena, new_size);
    size_t copy = old_size < new_size ? old_size : new_size;
    memcpy(new_ptr, ptr, copy);
    return ptr;
}
void arena_free(Jn_Arena* arena)
{
    assert(arena != NULL);
    Jn_ArenaBlock* block = arena->head;
    while (block)
    {
        Jn_ArenaBlock* next = block->next;
        free(block->mem);
        free(block);
        block = next;
    }
    arena->head = NULL;
}