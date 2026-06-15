#ifndef JOAN_ENV_H
#define JOAN_ENV_H

#include "Joan.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct JnObject JnObject;

typedef struct Jn_environ_E {
    JnObject* value;
    uint64_t hash;
    char* key;
    bool used;
} Jn_environ_E;

struct Jn_environ {  
    struct Jn_environ* parent;
    Jn_environ_E* buckets;
    size_t capacity, size;
};

JN_API Jn_environ* Jn_environ_init(Jn_environ* parent);
Jn_environ_E* environ_get(Jn_environ* env, char* key);
void environ_insert(Jn_environ* env, char* key, JnObject* obj);

#endif