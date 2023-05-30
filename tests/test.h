//
// Created by Haikal Zain on 28/5/2023.
//

#ifndef TINYJVM_TEST_H
#define TINYJVM_TEST_H

#define ASSERT_EQ(exp, act)  \
    do {                      \
    num_tests++;             \
    if((exp) != (act)) {        \
        printf("TEST FAILED: %s\n\t %s:%d EXPECTED %s obtained %s\n", __FUNCTION__, __FILE_NAME__, __LINE__, #exp, #act); \
        failures++;\
    }                         \
    }while(0)

#define ASSERT_FLOAT_EQ(exp, act)  \
    do {                      \
    num_tests++;                   \
    double _v = exp - act;\
    if(_v > 1e-6 || _v < -1e-6) {        \
        printf("TEST FAILED: %s\n\t %s:%d EXPECTED %s obtained %s\n", __FUNCTION__, __FILE_NAME__, __LINE__, #exp, #act); \
        failures++;\
    }                         \
    }while(0)

// compare char * and String
// TODO exp is unsafe since not NULL terminated
#define ASSERT_STR_EQ(exp, act)  \
    do {                      \
    num_tests++;             \
    if(act->size != strlen(exp) || strncmp(exp, (char*)act->data, act->size) != 0) {        \
        printf("TEST FAILED: %s\n\t %s:%d EXPECTED %s obtained %s\n", __FUNCTION__, __FILE_NAME__, __LINE__, exp, act->data); \
        failures++;\
    }                         \
    }while(0)

#endif //TINYJVM_TEST_H
