#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <Joan.h>
#include "object.h"



static Jn_HashEntry* get_hash_entry(Jn_Hashmap* map, JnObject* key)
{
    if (!map) return NULL;
    if (!map->buckets) return NULL;
    for (size_t i = 0; i < map->size; ++i)
    {
        Jn_HashEntry* entry =  &map->buckets[i];
        if (entry->hash == Jn_object_hash(key))
            return entry;
    }
    return NULL;
}


static void insert_hash_entry(Jn_Hashmap* map, JnObject* key, JnObject* value)
{
    assert(map != NULL || key != NULL || value != NULL);
    
    if (map->size >= map->capacity)
    {
        map->capacity *= 2;
        map->buckets =  realloc(map->buckets, sizeof(Jn_HashEntry) * map->capacity);
    }
    assert(map->buckets != NULL);
    size_t old_size = map->size;
    map->buckets[map->size++] = (Jn_HashEntry){
        .key = key, .value = value, .hash = Jn_object_hash(key)
    };
    assert(old_size < map->size);

}

static void get_or_insert_hash_entry(Jn_Hashmap* map, JnObject* key, JnObject* value)
{
    Jn_HashEntry* entry = get_hash_entry(map, key);
    if (entry != NULL)
    {
        entry->value = value;
        return;
    }
    insert_hash_entry(map, key, value);
}


Jn_HashEntry* Jn_hashmap_get(Jn_Hashmap* map, JnObject* key)
{
    return get_hash_entry(map, key);
}


void Jn_hashmap_insert(Jn_Hashmap* map, JnObject* key, JnObject* value, int idx)
{
    assert(map != NULL);
    if (idx >= map->capacity)
    {
        map->capacity *= 2;
        map->buckets =  realloc(map->buckets, sizeof(Jn_HashEntry) * map->capacity);
    }
    assert(map->buckets != NULL);
    map->buckets[idx] = (Jn_HashEntry){
        .key = key, .value = value, .hash = Jn_object_hash(key)
    };
    map->size++;
}


void Jn_hashmap_put(Jn_Hashmap* map, JnObject* key, JnObject* value)
{
    get_or_insert_hash_entry(map, key, value);
}



JnObject* Jnhashmap_get_from_index(Jn_Hashmap* map, int index)
{
    assert(map != NULL);
    size_t size = map->size;
    if (index < 0)
        index += size;
    if (index > size)
        return NULL; 
    return map->buckets[index].value;
}

