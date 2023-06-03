//
// Created by Haikal Zain on 3/6/2023.
//

#include <assert.h>
#include "hashtable.h"
#include "util.h"

static void __ht_put(HashTable *ht, String *key, HashTableValue value);

static int ht_resize(HashTable *ht, uint32_t size) {
    HashEntry *old_items = ht->items;
    int old_size = ht->size;
    ht->size = size;
    ht->items = jmalloc(size * sizeof(ht->items[0]));
    for(int i =0;i<size;i++) {
        ht->items[i].key = NULL;
        ht->items[i].next = NULL;
    }

    for(int i=0;i<old_size;i++) {
        if(old_items[i].key != NULL) {
            __ht_put(ht, old_items[i].key, old_items[i].value);
        }
    }

    if(old_items != NULL) {
        jfree(old_items);
    }
}

static int key_cmp(String *str1, String *str2) {
    if(str1->hash == str2->hash && str_compare(str1, str2) == 0) {
        return 0;
    }
    return 1;
}

/* returns hash entry for key if exists. otherwise NULL. */
static HashEntry *ht_get_entry(HashTable *ht, String *key) {
    uint32_t index = key->hash & ht->size;
    HashEntry *cur = &ht->items[index];
    while(cur->key != NULL) {
        if(key_cmp(key, cur->key) == 0) {
            return cur;
        }
        if(cur->next == NULL) {
            return NULL;
        }
        cur = cur->next;
    }

    return NULL;
}

static void __ht_put(HashTable *ht, String *key, HashTableValue value) {
    uint32_t index = key->hash & ht->size;
    HashEntry *cur = &ht->items[index];
    if(cur->key == NULL) {
        cur->key = key;
        cur->value = value;
        ht->count++;
        return;
    }
    for(;;) {
        if(key_cmp(key, cur->key) == 0) {
            cur->value = value;
            return;
        }
        if(cur->next == NULL) {
            int idx = (int)(cur - ht->items);
            for(int i =1;i<ht->size;i++) {
                uint32_t ii = idx + i;
                ii = (ii >= ht->size) ? ii - ht->size : ii;
                if((cur + ii)->key == NULL) {
                    cur->next = (cur + ii);
                    cur->next->key = key;
                    cur->next->value = value;
                    ht->count++;
                    return;
                }
            }
            assert(0);
        }
        cur = cur->next;
    }
    assert(0);

}

void ht_init(HashTable *ht) {
    ht->count = 0;
    ht->items = NULL;
    ht->size = 0;
    ht_resize(ht, HASHTABLE_INIT);
}

void ht_put(HashTable *ht, String *key, HashTableValue value) {
    if(ht->count * 2 > ht->size) {
        ht_resize(ht, ht->size * 2);
    }
    __ht_put(ht, key, value);
}
void ht_remove(HashTable *ht, String *key) {

}
HashTableValue ht_get(HashTable *ht, String *key) {
    HashEntry *entry = ht_get_entry(ht, key);
    if(entry == NULL) return NULL;
    return entry->value;
}