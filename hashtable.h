//
// Created by Haikal Zain on 3/6/2023.
//

#ifndef TINYJVM_HASHTABLE_H
#define TINYJVM_HASHTABLE_H
#include "string.h"

#define HASHTABLE_INIT 8

typedef void* HashTableValue;

typedef struct HashEntry {
    struct HashEntry *next; // LL with all entries
    String *key;
    HashTableValue value;
    struct HashEntry *chain_next; // chains to entry in table
}HashEntry;

typedef struct HashTable {
    uint32_t size; // power of two
    uint32_t count;
    HashEntry *items;
    HashEntry *head; // head of ll

    // key_free_func
    // value_free_func
} HashTable;

typedef struct HashTableIterator {
    HashTable *ht;

};

void ht_init(HashTable *ht);
void ht_put(HashTable *ht, String *key, HashTableValue value);
void ht_remove(HashTable *ht, String *key);
HashTableValue ht_get(HashTable *ht, String *key);

#endif //TINYJVM_HASHTABLE_H
