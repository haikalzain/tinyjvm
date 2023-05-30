//
// Created by Haikal Zain on 10/5/2023.
//
#include <stdio.h>
#include "../jvm.h"
#include "util.h"
#include <string.h>
#include "test.h"

static int num_tests = 0;
static int failures = 0;

char *test_native_libs[] = {"/usr/local/lib/tinyjvm/libnative_test.dylib"};
Options options_native_test = {
        .native_libs = test_native_libs,
        .n_native_libs = 1
};

void test_basic_class() {
    ByteBuf buf;
    JClass class;
    read_file("Basic.class", &buf);
    read_class_from_bytes(&class, &buf);
    ASSERT_EQ(27, class.n_constants);
    ASSERT_STR_EQ("<init>", VAL_GET_STRING(class.constants[6].value));
    ASSERT_STR_EQ("([Ljava/lang/String;)V", VAL_GET_STRING(class.constants[11].value));
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

void test_load_object_class() {
    JClass cls;
    read_class_from_path(&cls, "java/lang/Object.class");
    ASSERT_EQ(0, cls.super_class);
}



void test_execute_native_basic_class() {
    Runtime rt;
    rt_init(&rt, NULL);
    Value v = execute_static_method(&rt, "NativeBasic", "main", NULL, 0);
    //ASSERT_EQ(TYPE_INT, VAL_GET_TAG(v));
    //ASSERT_EQ(21, v.i);
}

void test_execute_native_add() {
    Runtime rt;
    Value v;

    rt_init(&rt, &options_native_test);
    Value int_args[] = {MKVAL(TYPE_INT, 200), MKVAL(TYPE_INT, 500)};
    v = execute_static_method(&rt, "NativeBasic", "addInt", int_args, 2);
    ASSERT_EQ(TYPE_INT, VAL_GET_TAG(v));
    ASSERT_EQ(700, v.i);

    Value float_args[] = {MKFLOAT(200.3), MKFLOAT(2.75)};
    Value v2 = execute_static_method(&rt, "NativeBasic", "addFloat", float_args, 2);
    ASSERT_EQ(TYPE_FLOAT, VAL_GET_TAG(v2));
    ASSERT_FLOAT_EQ(203.05, v2.f);

    Value double_args[] = {MKFLOAT(200.3), MKFLOAT(2.75)};
    Value v3 = execute_static_method(&rt, "NativeBasic", "addDouble", double_args, 2);
    ASSERT_EQ(TYPE_DOUBLE, VAL_GET_TAG(v3));
    ASSERT_FLOAT_EQ(203.05, v3.d);
}

void test_static_add() {
    Runtime rt;
    rt_init(&rt, NULL);
    Value args[] = {MKVAL(TYPE_INT, 2), MKVAL(TYPE_INT, 1000)};
    Value v = execute_static_method(&rt, "FunctionCalls", "add", args, 2);
    ASSERT_EQ(TYPE_INT, VAL_GET_TAG(v));
    ASSERT_EQ(1002, v.i);
}


int main() {
    test_execute_native_add();
    test_load_object_class();
    test_basic_class();
    test_execute_native_basic_class();
    test_static_add();
    PRINT_STATS();

}