//
// Created by Haikal Zain on 10/5/2023.
//
#include <stdio.h>
#include "../jvm.h"
#include "util.h"

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

void test_basic_class() {
    ByteBuf buf;
    JClass class;
    read_file("tests/data/Basic.class", &buf);
    read_class_from_bytes(&class, &buf);
}

int main() {
    test_basic_class();
    printf("TESTS RUN: %d FAILURES: %d\n", num_tests, failures);

}