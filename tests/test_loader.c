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
    ASSERT_EQ(0, class.n_interfaces);
    ASSERT_EQ(0, class.n_fields);
    ASSERT_EQ(2, class.n_methods);
    ASSERT_EQ(1, class.n_attributes);

    ASSERT_EQ(METHOD_PUBLIC, class.methods[0].access_flags);
    ASSERT_EQ(7, class.methods[0].descriptor_index);
    ASSERT_EQ(6, class.methods[0].name_index);
    ASSERT_EQ(1, class.methods[0].attributes_count);
    // TODO need to test attributes

    ASSERT_EQ(METHOD_PUBLIC | METHOD_STATIC, class.methods[1].access_flags);
    ASSERT_EQ(11, class.methods[1].descriptor_index);
    ASSERT_EQ(10, class.methods[1].name_index);
    ASSERT_EQ(1, class.methods[1].attributes_count);
}

void test_execute_basic_class() {
    ByteBuf buf;
    JClass class;
    read_file("tests/data/Basic.class", &buf);
    read_class_from_bytes(&class, &buf);
    jvm_execute_class(&class, NULL);
}

int main() {
    test_basic_class();
    test_execute_basic_class();
    printf("TESTS RUN: %d FAILURES: %d\n", num_tests, failures);

}