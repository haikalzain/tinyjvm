//
// Created by Haikal Zain on 9/5/2023.
//

#ifndef TINYJVM_JVM_H
#define TINYJVM_JVM_H


#include "types.h"
#include "util.h"
#include "native.h"
#include "string.h"
#include "jni.h"
#include "class_loader.h"

typedef struct JByteCode {
    u2 max_stack;
    u2 max_locals;
    u4 code_length;
    u1 *code;

    u2 exception_table_length;
    exception_entry *exception_table;

} JByteCode;

typedef struct JClass JClass;
typedef struct JMethod JMethod;
typedef struct JField JField;
typedef struct JInstance JInstance;
typedef struct Runtime Runtime;
typedef struct JArray JArray;

typedef struct FieldType {
    ValueType type;
    String class_name;
    u1 dim; // array type iff != 0
} FieldType;

typedef struct JMethodDescriptor {
    u1 nparams;
    FieldType return_type;
    FieldType *field_types;

} JMethodDescriptor;

typedef Value (*JBuiltinFunction)(Runtime *rt, JInstance *ins, Value *args, u1 nargs);

struct JField {
    String *name;
    Value value;
};

struct JMethod {
    JClass *class;
    u2             access_flags;
    union {
        // some representation of code
        JByteCode code;
        JBuiltinFunction func; // native function
    };
    JMethodDescriptor descriptor;
    String *name;
};

typedef enum {
    CLASS_LOADING,
    CLASS_INITING,
    CLASS_LOADED
} JClass_Status;

struct JClass {
    JClass_Status status;
    JClass *next;
    String *name;
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

    ClassFile *cf;
};

struct JInstance {
    JClass *class;
    JField *fields;
};

struct JArray {
    // no need to store value type
    FieldType type;
    u4 length;
    Value *data;
};

struct Runtime {
    JClass *cache;
    String **classpaths;
    u2 n_classpaths;
    NativeLibs native_libs;
    JNIEnv env;
    // cache loaded classes

    // some instance of GC

};

typedef struct Options {
    char **native_libs;
    int n_native_libs;
} Options;

int parse_method_descriptor(JMethodDescriptor *d, ByteBuf *buf);
int parse_field_type(FieldType *fieldType, ByteBuf *buf);
int method_descriptor_free(JMethodDescriptor *d);

JArray *array_create(u4 length, FieldType type);
JArray *multiarray_create(Value *lengths, int n_lengths, FieldType type);

int rt_init(Runtime *runtime, Options *options);
int rt_execute_class(Runtime *runtime, JClass *class, Options *options);
// gets the class, loads it if not already loaded
JClass* rt_get_class(Runtime *runtime, String *name);
JMethod *rt_constant_resolve_methodref(Runtime *rt, Constant *constants, u2 index);
int rt_constant_resolve_fieldtype(FieldType *type, Constant *constants, u2 index);
JClass *rt_constant_resolve_class(Runtime *rt, Constant *constants, u2 index);
int read_class_from_bytes(JClass *cl, ByteBuf *buf);
int read_class_from_path(JClass *cl, char *path);

String *cl_constants_get_string(JClass *class, u2 index);

JMethod *rt_find_method(Runtime *rt, JClass *class, String *name);
JField *rt_find_field(Runtime *rt, JInstance *ins, String *name);
JInstance *instance_create(JClass *class);
int instance_set_field(Runtime *rt, JInstance *instance, u2 index, Value value);
Value instance_get_field(Runtime *rt, JInstance *instance, u2 index);
void instance_free(JInstance *instance);

Value rt_execute_method(Runtime *rt, const JInstance *this, const JMethod *method, Value *args, u1 nargs);

// can execute any static method
Value execute_static_method(Runtime *rt, char *class_name, char *method_name, Value *args, u2 nargs);


typedef enum Opcode {
    AALOAD = 50,
    AASTORE = 83,
    ACONST_NULL = 1,
    ALOAD = 25,
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
    ICONST_m1 = 2,
    ICONST_0 = 3,
    ICONST_1 = 4,
    ICONST_2 = 5,
    ICONST_3 = 6,
    ICONST_4 = 7,
    ICONST_5 = 8,
    ISTORE = 54,
    ISTORE_0 = 59,
    ISTORE_1 = 60,
    ISTORE_2 = 61,
    ISTORE_3 = 62,
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
    GETFIELD = 180,
    GETSTATIC = 178,
    RET = 169,
    RETURN = 177,
    IRETURN = 172,
    IADD = 96,
    DUP = 89,
    MULTINEWARRAY = 197,
    IASTORE = 79,
    IALOAD = 46,
} Opcode;

#endif //TINYJVM_JVM_H
