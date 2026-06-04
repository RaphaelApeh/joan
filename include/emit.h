#ifndef JOAN_EMIT_H
#define JOAN_EMIT_H
#include <stdint.h>
#include "object.h"
typedef struct Chuck Chuck;

int add_ident(Chuck* chuck, char* ident);
void write_chuck(Chuck* chuck, uint8_t byte);
int add_constant(Chuck* chuck, JnObject* object);
int current_offset(Chuck* chuck);
int emit_jump(Chuck* chuck, uint8_t instrction);
void patch_jump(Chuck* chuck, int offset);
void emit_loop(Chuck* chuck, int loop_start);
#endif