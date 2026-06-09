#include <string.h>
#include <stdlib.h>
#include "Joan.h"
#include "env.h"
#include "object.h"
#include "helper.h"

env_t* init_env(env_t* parent)
{
    env_t* e = malloc(sizeof(env_t));
    e->capacity = 256;
    e->count = 0;
    e->parent = parent;
    e->entries = malloc(sizeof(entry_t) * 256);
    return e;
}

void set_env(env_t* env, char* key, JnObject* obj, bool is_const, bool is_public)
{
    if (NULL == env || NULL == obj) return;
    if (env->count >= env->capacity)
    {
        env->capacity *= 2;
        env->entries = realloc(env->entries, sizeof(entry_t) * env->capacity);
    }
    env->entries[env->count].used = true;
    env->entries[env->count].key = strdup(key);
    env->entries[env->count].value = obj;
    env->entries[env->count].is_public = is_public;
    env->entries[env->count].is_const = is_const;
    env->count++;
}

entry_t* get_envEntry(env_t* env, char* key)
{
    if (NULL == env) return NULL;
    while(env)
    {
        for (size_t i = 0; i < env->count; i++)
            if (strcmp(env->entries[i].key, key) == 0)
                return &env->entries[i];
        env = env->parent;
    }
    return NULL;
}

JnObject* get_env(env_t* env, char* key)
{
    if (NULL == env) return NULL;
    while(env)
    {
        for (size_t i = 0; i < env->count; i++)
            if (strcmp(env->entries[i].key, key) == 0)
                return env->entries[i].value;
        env = env->parent;
    }
    return NULL;
}