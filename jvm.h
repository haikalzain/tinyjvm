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

// fields

typedef struct attribute_info {
    u2 attribute_name_index;
    u4 attribute_length;
    u1 *data;
} attribute_info;

typedef enum field_flags {
    FIELD_PUBLIC = 0x0001,
    FIELD_PRIVATE = 0x0002,
    FIELD_PROTECTED = 0x0004,
    FIELD_STATIC = 0x0008,
    FIELD_FINAL = 0x0010,
    FIELD_VOLATILE = 0x0040,
    FIELD_TRANSIENT = 0x0080,
    FIELD_SYNTHETIC = 0x1000,
    FIELD_ENUM = 0x4000
} field_flags;

typedef struct field_info {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info *attributes;
} field_info;

typedef enum method_flags {
    METHOD_PUBLIC = 0x0001,
    METHOD_PRIVATE = 0x0002,
    METHOD_PROTECTED = 0x0004,
    METHOD_STATIC = 0x0008,
    METHOD_FINAL = 0x0010,
    METHOD_SYNCHRONIZED = 0x0020,
    METHOD_BRIDGE = 0x0040,
    METHOD_VARARGS = 0x0080,
    METHOD_NATIVE = 0x0100,
    METHOD_ABSTRACT = 0x0400,
    METHOD_STRICT = 0x0800,
    METHOD_SYNTHETIC = 0x1000
} method_flags;

typedef struct method_info {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info *attributes;
} method_info;

typedef struct JClass {
    u2 major_version;
    u2 minor_version;
    Constant *constants; // 1-indexed
    u2 n_constants;
    u2             access_flags;
    u2             this_class;
    u2             super_class;
    u2             n_interfaces;
    u2             *interfaces; // indexes into constant table
    u2             n_fields;
    field_info     *fields;
    u2             n_methods;
    method_info    *methods;
    u2             n_attributes;
    attribute_info *attributes;
} JClass;

#endif //TINYJVM_JVM_H
