#ifndef JOAN_ENV_H
#define JOAN_ENV_H

#include <stdbool.h>
#include <stdint.h>

typedef struct env_t env_t;
typedef struct JnObject JnObject;

typedef struct
{
    char* key;
    JnObject* value;
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
void set_env(env_t* env, char* key, JnObject* obj, bool is_const, bool is_public);
JnObject* get_env(env_t* env, char* key);
entry_t* get_envEntry(env_t* env, char* key);
#endif