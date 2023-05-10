//
// Created by Haikal Zain on 10/5/2023.
//
#include <stdio.h>
#include "../jvm.h"
#include "util.h"
#include <string.h>

int read_class_from_bytes(JClass *cl, ByteBuf *buf);

static int num_tests = 0;
static int failures = 0;

#define ASSERT_EQ(exp, act)  \
    do {                      \
    num_tests++;             \
    if((exp) != (act)) {        \
        printf("TEST FAILED: %s\n\t %s:%d EXPECTED %s obtained %s\n", __FUNCTION__, __FILE_NAME__, __LINE__, #exp, #act); \
        failures++;\
    }                         \
    }while(0)

// compare char * and String
// TODO exp is unsafe since not NULL terminated
#define ASSERT_STR_EQ(exp, act)  \
    do {                      \
    num_tests++;             \
    if(act->size != strlen(exp) || strncmp(exp, act->data, act->size) != 0) {        \
        printf("TEST FAILED: %s\n\t %s:%d EXPECTED %s obtained %s\n", __FUNCTION__, __FILE_NAME__, __LINE__, exp, act->data); \
        failures++;\
    }                         \
    }while(0)

void test_basic_class() {
    ByteBuf buf;
    JClass class;
    read_file("tests/data/Basic.class", &buf);
    read_class_from_bytes(&class, &buf);
    ASSERT_EQ(27, class.n_constants);
    ASSERT_STR_EQ("<init>", VAL_STR(class.constants[6].value));
    ASSERT_STR_EQ("([Ljava/lang/String;)V", VAL_STR(class.constants[11].value));
}

int main() {
    test_basic_class();
    printf("TESTS RUN: %d FAILURES: %d\n", num_tests, failures);

}