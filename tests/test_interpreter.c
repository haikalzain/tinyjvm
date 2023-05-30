#include "test.h"
#include "../jvm.h"

int num_tests = 0;
int failures = 0;

void test_exceptions() {
    Runtime rt;
    rt_init(&rt, NULL);
    Value v = execute_static_method(&rt, "Exceptions", "countExceptions", NULL, 0);
    ASSERT_EQ(TYPE_INT, VAL_GET_TAG(v));
    ASSERT_EQ(2, v.i);
}

void test_execute_function_calls_class() {
    Runtime rt;
    rt_init(&rt, NULL);
    Value v = execute_static_method(&rt, "FunctionCalls", "add2", NULL, 0);
    ASSERT_EQ(TYPE_INT, VAL_GET_TAG(v));
    ASSERT_EQ(21, v.i);
}

void test_self_initialize() {
    Runtime rt;
    rt_init(&rt, NULL);
    Value v = execute_static_method(&rt, "SelfInitialize", "test", NULL, 0);
    ASSERT_EQ(TYPE_INT, VAL_GET_TAG(v));
    ASSERT_EQ(3, v.i);
}

int main() {
    test_exceptions();
    test_self_initialize();
    test_execute_function_calls_class();
    PRINT_STATS();
}