//
// Created by Haikal Zain on 9/5/2023.
//

#ifndef TINYJVM_JVM_H
#define TINYJVM_JVM_H


#include "types.h"
#include "util.h"

// Define some custom types





// Class format

typedef struct cp_info {
    u1 tag;
    u1 info[];
} cp_info;

typedef struct interfaces {

} interfaces;

typedef struct field_info {

} field_info;

typedef struct method_info {

} method_info;

typedef struct attribute_info {

} attribute_info;

typedef struct ClassFile {
    u4             magic;
    u2             minor_version;
    u2             major_version;
    u2             constant_pool_count;
    cp_info        *constant_pool;
    u2             access_flags;
    u2             this_class;
    u2             super_class;
    u2             interfaces_count;
    u2             *interfaces;
    u2             fields_count;
    field_info     *fields;
    u2             methods_count;
    method_info    *methods;
    u2             attributes_count;
    attribute_info *attributes;
} ClassFile;

typedef struct JClass {
    ClassFile cf;
} JClass;

#endif //TINYJVM_JVM_H
