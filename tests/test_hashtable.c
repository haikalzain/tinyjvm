//
// Created by Haikal Zain on 3/6/2023.
//

#include <assert.h>
#include "test.h"
#include "../hashtable.h"

int num_tests = 0;
int failures = 0;

void test_simple_insertion() {
    HashTable ht;
    ht_init(&ht);
    String *key1 = str_from_c("key1");
    String *key2 = str_from_c("key2");
    String *key3 = str_from_c("key3");
    String *key4 = str_from_c("key4");

    int val1 = 1;
    int val2 = 2;
    int val3 = 3;
    ht_put(&ht, key1, &val1);
    ht_put(&ht, key2, &val2);
    ht_put(&ht, key3, &val3);

    ASSERT_EQ(NULL, ht_get(&ht, key4));
    ASSERT_EQ(1, *(int *)ht_get(&ht, key1));
    ASSERT_EQ(2, *(int *)ht_get(&ht, key2));
    ASSERT_EQ(3, *(int *)ht_get(&ht, key3));
}

void test_resize() {
    HashTable ht;
    ht_init(&ht);
    String *key1 = str_from_c("key1");
    String *key2 = str_from_c("key2");
    String *key3 = str_from_c("key3");
    String *key4 = str_from_c("key4");
    String *key5 = str_from_c("key5");

    int val1 = 1;
    int val2 = 2;
    int val3 = 3;
    int val4 = 4;
    int val5 = 5;
    ht_put(&ht, key1, &val1);
    ht_put(&ht, key2, &val2);
    ht_put(&ht, key3, &val3);
    ht_put(&ht, key4, &val4);
    ht_put(&ht, key5, &val5);

    ASSERT_EQ(16, ht.size);
    ASSERT_EQ(5, ht.count);


    ASSERT_EQ(1, *(int *)ht_get(&ht, key1));
    ASSERT_EQ(2, *(int *)ht_get(&ht, key2));
    ASSERT_EQ(3, *(int *)ht_get(&ht, key3));
    ASSERT_EQ(4, *(int *)ht_get(&ht, key4));
    ASSERT_EQ(5, *(int *)ht_get(&ht, key5));
}

void test_lots() {
    HashTable ht;
    ht_init(&ht);
    char buf[20];
    int nums[1000];
    int newnums[1000];
    for(int i=0;i<1000;i++) {
        sprintf(buf, "key%d", i);
        String *key = str_from_c(buf);
        nums[i] = i;
        ht_put(&ht, key, &nums[i]);
        ASSERT_EQ(i, *(int*)ht_get(&ht, key));
        assert(ht.count == i + 1);


    }

    for(int i=0;i<1000;i++) {
        sprintf(buf, "key%d", i);
        String *key = str_from_c(buf);
        ASSERT_EQ(i, *(int*)ht_get(&ht, key));
        str_free(key);
    }

    // now try to modify
    for(int i=0;i<1000;i++) {
        sprintf(buf, "key%d", i);
        String *key = str_from_c(buf);
        newnums[i] = i + 1000;
        ht_put(&ht, key, &newnums[i]);
        ASSERT_EQ(i + 1000, *(int*)ht_get(&ht, key));
    }

    for(int i=0;i<1000;i++) {
        sprintf(buf, "key%d", i);
        String *key = str_from_c(buf);
        ASSERT_EQ(i + 1000, *(int*)ht_get(&ht, key));
        str_free(key);
    }
}


int main() {
    test_simple_insertion();
    test_resize();
    test_lots();
    PRINT_STATS();
    return 0;
}