//
// Created by Haikal Zain on 9/5/2023.
//

#ifndef TINYJVM_JVM_H
#define TINYJVM_JVM_H


#include "types.h"
#include "util.h"

// Define some custom types

typedef struct String {
    u2 size;
    u1 *data;
} String;

void str_create(String *str, u1 *data, u2 size);

typedef enum ValueType {
    INT, DOUBLE, STRING, LONG, FLOAT
} ValueType;

typedef struct Value {
    ValueType tag;
    union {
        int32_t i;
        int64_t l;
        float f;
        double d;
        void *ptr;
    } as;
} Value;

#define VAL_STR(v) ((String *)v.as.ptr)

// Constants
// These are stored as int32 Value type
typedef struct CONSTANT_index_info {
    u2 index;
} CONSTANT_index_info;

typedef struct CONSTANT_kind_index_info {
    u1 kind;
    u2 index;
} CONSTANT_kind_index_info;


typedef struct CONSTANT_double_index_info {
    u2 index1;
    u2 index2;
} CONSTANT_double_index_info ;

// Class format

typedef enum cp_tags {
    CONSTANT_Class = 7,
    CONSTANT_Fieldref = 9,
    CONSTANT_Methodref = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_String = 8,
    CONSTANT_Integer = 3,
    CONSTANT_Float = 4,
    CONSTANT_Long = 5,
    CONSTANT_Double = 6,
    CONSTANT_NameAndType = 12,
    CONSTANT_Utf8 = 1,
    CONSTANT_MethodHandle = 15,
    CONSTANT_MethodType = 16,
    CONSTANT_Dynamic = 17,
    CONSTANT_InvokeDynamic = 18,
    CONSTANT_Module = 19,
    CONSTANT_Package = 20
} cp_tags;

typedef struct Constant {
    cp_tags tag;
    Value value;
} Constant;

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
    Constant *constants; // 1-indexed
    u2 n_constants;
} JClass;

#endif //TINYJVM_JVM_H
