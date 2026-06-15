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
        ent->key = NULL;
    }
    return env;
}


Jn_environ_E* environ_get(Jn_environ* env, char* key)
{
    assert(env != NULL && env->buckets != NULL);
    assert(key != NULL);
    int keylen = strlen(key);
    uint64_t hash = fnv_hash(key, keylen) % env->capacity;
    printf("GET HASH %lld \n", hash);
    while (env)
    {
        for (int i = 0; i < env->capacity; ++i)
        {
            Jn_environ_E* ent = &env->buckets[(hash + i) % env->capacity];
            if (match(ent, key))
                return ent;
            if (ent->key == NULL)
            {
                printf("ent->key is NULL\n"); //TODO
                return NULL;
            }
        }
        env = env->parent;
    }
    return NULL;
}

void environ_insert(Jn_environ* env, char* key, JnObject* obj)
{

    assert(env != NULL && key != NULL && obj != NULL);
    uint64_t hash = fnv_hash(key, strlen(key)) % env->capacity;
    printf("HASH %ld\n", hash);
    while (env->buckets[hash].used && match(&env->buckets[hash], key))
        hash = (hash + 1) & env->capacity;
    env->buckets[hash].used = true;
    env->buckets[hash].key = strdup(key);
    env->buckets[hash].value = obj;
}

