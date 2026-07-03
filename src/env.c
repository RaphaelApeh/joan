#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "env.h"
#include "object.h"
#include "helper.h"

#define ENV_INIT_CAPACITY 256


static bool match(Jn_environ_E* ent, char* key)
{
    if (ent->key == NULL) return false;
    return ent->key && key && strcmp(ent->key, key) == 0;
}

JN_API Jn_environ* Jn_environ_init(Jn_environ* parent)
{

    Jn_environ* env = malloc(sizeof(Jn_environ));
    assert(env != NULL);
    env->capacity = ENV_INIT_CAPACITY;
    env->buckets = malloc(sizeof(Jn_environ_E) * env->capacity);
    env->size = 0;
    env->parent = parent;
    assert(env->buckets != NULL);
    for (int i = 0; i < env->capacity; ++i)
    {
        Jn_environ_E* ent = &env->buckets[i];
        ent->used = false;
        ent->value = NULL;
        ent->key = NULL;
    }
    return env;
}


Jn_environ_E* environ_get(Jn_environ* env, char* key)
{
    assert(env != NULL && env->buckets != NULL);
    assert(key != NULL);
    while (env)
    {
        for (int i = 0; i < env->size; ++i)
        {
            Jn_environ_E* ent = &env->buckets[i];
            if (match(ent, key))
                return ent;
        }
        env = env->parent;
    }
    return NULL;
}

void environ_insert(Jn_environ* env, char* key, JnObject* obj)
{

    assert(env != NULL && key != NULL && obj != NULL);
    Jn_environ_E* e = environ_get(env, key);
    if (NULL != e)
    {
        e->value = obj;
        return;
    }
    if (env->capacity <= env->size)
    {
        env->capacity *= 2;
        env->buckets = realloc(env->buckets, sizeof(Jn_environ_E) * env->capacity);
    }
    set_sumbols(NULL, key);
    if (obj->type == STRUCT_TYPE)
    {
        obj->struct_obj->name = key;
    }else if (JN_IS_FUNCTION(obj))
    {
        if (strcmp(obj->fn->name, DEFAULT_LAMBDA_NAME) == 0)
            obj->fn->name = strdup(key);
    }
    env->buckets[env->size].used = true;
    env->buckets[env->size].key = strdup(key);
    env->buckets[env->size].value = obj;
    env->size++;
}

