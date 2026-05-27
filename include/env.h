#ifndef ENV_H
#define ENV_H

#include <stdbool.h>
#include <stdint.h>

typedef struct env_t env_t;
typedef struct Object Object;

typedef struct
{
    char* key;
    Object* value;
    bool is_const;
    bool is_public;
    bool used;
} entry_t;

typedef struct env_t
{
    entry_t* entries;
    size_t capacity;
    size_t count;
    env_t* parent;
} env_t;

env_t* init_env(env_t* parent);
void set_env(env_t* env, char* key, Object* obj, bool is_const, bool is_public);
Object* get_env(env_t* env, char* key);
entry_t* get_envEntry(env_t* env, char* key);
#endif