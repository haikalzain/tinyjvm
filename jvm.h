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
int str_compare(String *s1, String *s2);
int str_compare_raw(String *s1, char *s2);

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
    };
} Value;

#define MKVAL(t, value) (Value){.i = value, .tag = t}
#define MKPTR(t, value) (Value){.ptr = value, .tag = t}
#define VAL_GET_TAG(v) ((v).tag)
#define VAL_GET_PTR(v) ((v).ptr)
#define VAL_GET_STRING(v) ((String *)v.ptr)

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

//all the attribute types

typedef struct attribute_info {
    u2 attribute_name_index;
    u4 attribute_length;
    u1 *data;
} attribute_info;

typedef struct ModuleMainClass_attribute {
u2 attribute_name_index;
u4 attribute_length;
u2 main_class_index;
} ModuleMainClass_attribute;




// fields

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

typedef struct JByteCode {
    u2 max_stack;
    u2 max_locals;
    u4 code_length;
    u1 *code;
    /*u2 exception_table_length;
    {   u2 start_pc;
        u2 end_pc;
        u2 handler_pc;
        u2 catch_type;
    } exception_table[exception_table_length];*/
    u2 attributes_count;
    attribute_info *attributes;
} JByteCode;

typedef struct JMethod {
    u2             access_flags;
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info *attributes;
    // some representation of code
    JByteCode code;
} JMethod;

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
    JMethod    *methods;
    //JMethod *method; should be a hashmap

    u2             n_attributes;
    attribute_info *attributes;
} JClass;

typedef struct JInstance {
    JClass *class;
    //fields

} JInstance;

typedef struct CallFrame {
    struct CallFrame *parent;
    u1 *ip;
    u2 sp;
    // local vars
    // stack
    JInstance *instance;
    JMethod *method;
} CallFrame;

typedef struct Runtime {
    // cache loaded classes

    // some instance of GC


    CallFrame *cf;

} Runtime;

typedef struct Options {

} Options;


void rt_init(Runtime *runtime, Options *options);
int rt_execute_class(Runtime *runtime, JClass *class, Options *options);
JClass* rt_get_class(Runtime *runtime, String *name);


String *cl_constants_get_string(JClass *class, u2 index);
cp_tags cl_constants_get_tag(JClass *class, u2 index);

JMethod *cl_find_method(JClass *class, String *name);
JMethod *instance_find_method(JInstance *instance, String *name);
int instance_create(JInstance *instance, JClass *class);
void instance_destroy(JInstance *instance);

void cf_from_static_method(CallFrame *cf, JMethod *method, CallFrame *parent);
void cf_from_instance_method(CallFrame *cf, JInstance *instance, JMethod *method, CallFrame *parent);
void cf_stack_push(CallFrame *cf, Value v);
Value cf_stack_pop(CallFrame *cf);


typedef enum Opcode {
    AALOAD = 50,
    AASTORE = 83,
    ACONST_NULL = 1,
    ALOAD_0 = 42,
    ALOAD_1 = 43,
    ALOAD_2 = 44,
    ALOAD_3 = 45,
    ANEWARRAY = 189,
    ARETURN = 176,
    ARRAYLENGTH = 190,
    ASTORE = 58,
    ASTORE_0 = 75,
    ASTORE_1 = 76,
    ASTORE_2 = 77,
    ASTORE_3 = 78,
    ATHROW = 191,
    BALOAD = 51,
    BASTORE = 84,
    BIPUSH = 16,
    ICONST_m1 =2,
    ICONST_0 = 3,
    ICONST_1 = 4,
    ICONST_2 = 5,
    ICONST_3 = 6,
    ICONST_4 = 7,
    ICONST_5 = 8,
    ILOAD = 21,
    ILOAD_0 = 26,
    ILOAD_1 = 27,
    ILOAD_2 = 28,
    ILOAD_3 = 29,
    INVOKEDYNAMIC = 186,
    INVOKESPECIAL = 183,
    INVOKEVIRTUAL = 182,
    INVOKESTATIC = 184,
    MONITORENTER = 194,
    MONITOREXIT = 195,
    NEW = 187,
    NEWARRAY = 188,
    NOP = 0,
    POP = 87,
    POP2 = 88,
    PUTFIELD = 181,
    PUTSTATIC = 179,
    RET = 169,
    RETURN = 177
} Opcode;

#endif //TINYJVM_JVM_H
