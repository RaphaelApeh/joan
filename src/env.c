#include <string.h>
#include <stdlib.h>
#include "env.h"
#include "object.h"

env_t* init_env(env_t* parent)
{
    env_t* e = malloc(sizeof(env_t));
    e->capacity = 256;
    e->count = 0;
    e->parent = parent;
    e->entries = malloc(sizeof(entry_t) * 256);
    return e;
}

static uint32_t hash(const char* str)
{
    uint32_t h = 2166136261u;
    while (*str)
    {
        h ^= (unsigned char)*str++;
        h *= 16777619;
    }
    return h;
}
void set_env(env_t* env, char* key, Object* obj, bool is_const, bool is_public)
{
    if (NULL == env || NULL == obj) return;
    // Object* e;
    // if (e = get_env(env, key) != NULL)
    //     *e = *obj;
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
    // int id = hash(key) % env->capacity;
    // while(env->entries[id].used && strcmp(env->entries[id].key, key) != 0)
    //         id = (id + 1) % env->capacity;
    // env->entries[id].used = true;
    // strcpy(env->entries[id].key, key);
    // env->entries[id].value = obj;
}

Object* get_env(env_t* env, char* key)
{
    if (NULL == env) return NULL;
    // size_t id = hash(key) * env->capacity;
    while(env)
    {
        for (size_t i = 0; i < env->count; i++)
            if (strcmp(env->entries[i].key, key) == 0)
                return env->entries[i].value;
            // id = (id + 1) % env->capacity;
        env = env->parent;
    }
    return NULL;
}