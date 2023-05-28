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
    u2 cap;
    u1 *data;
} String;

String *str_from_c(char *cstr);
void str_create(String *str, u1 *data, u2 size);
int str_compare(String *s1, String *s2);
int str_compare_raw(String *s1, char *s2);
String *str_concat(String *s1, String *s2);
void str_free(String *str);
int str_ensure_zeropad(String *str);
char *str_cstr(String *str);

typedef struct RefcountHeader {
    u4 refcount;
} RefcountHeader;

typedef struct GCHeader {
    u4 refcount;
    u1 mark;
} GCHeader;

typedef struct Symbol {
    RefcountHeader header;
    String *str;
    u4 hash;
} Symbol;

typedef struct SymbolTable {
    u4 size;
    u4 cap;
    Symbol *table;
} SymbolTable;

Symbol *symbol_get(String *str);
void symbol_free(Symbol *);



typedef enum ValueType {
    TYPE_VOID,
    TYPE_NULL,
    TYPE_BOOL,
    TYPE_BYTE,
    TYPE_CHAR,
    TYPE_SHORT,
    TYPE_INT,
    TYPE_DOUBLE,
    TYPE_STRING,
    TYPE_LONG,
    TYPE_FLOAT,
    TYPE_CLASS,
    TYPE_INSTANCE,
    TYPE_EXCEPTION,
    // Internal only
    TYPE_METHOD_DESCRIPTOR,
    TYPE_FIELD_DESCRIPTOR
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
#define VAL_VOID MKVAL(TYPE_VOID, 0)
#define VAL_NULL MKVAL(TYPE_NULL, 0)
#define VAL_GET_TAG(v) ((v).tag)
#define VAL_GET_PTR(v) ((v).ptr)
#define VAL_GET_STRING(v) ((String *)v.ptr)

static inline u1 val_is_comp_type1(Value v) {
    if(VAL_GET_TAG(v) != TYPE_LONG && VAL_GET_TAG(v) != TYPE_DOUBLE) return 1;
    return 0;
}

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
    union {
        Value value;
        CONSTANT_index_info index_info;
        CONSTANT_kind_index_info kind_index_info;
        CONSTANT_double_index_info double_index_info;
    };
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

typedef struct JClass JClass;
typedef struct JMethod JMethod;
typedef struct JField JField;
typedef struct JInstance JInstance;
typedef struct Runtime Runtime;

typedef struct FieldType {
    ValueType type;
    String class_name;
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
    u2             name_index;
    u2             descriptor_index;
    u2             attributes_count;
    attribute_info *attributes;
    union {
        // some representation of code
        JByteCode code;
        JBuiltinFunction func; // native function
    };
    JMethodDescriptor descriptor;
    String *name;
    u1 descriptor_cached;
};

struct JClass {
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

    u2             n_attributes;
    attribute_info *attributes;
};

struct JInstance {
    JClass *class;
    JField *fields;
};

struct Runtime {
    JClass *cache;
    String **classpaths;
    u2 n_classpaths;
    // cache loaded classes

    // some instance of GC

};

typedef struct Options {

} Options;

int parse_method_descriptor(JMethodDescriptor *d, ByteBuf *buf);
int method_descriptor_free(JMethodDescriptor *d);

int rt_init(Runtime *runtime, Options *options);
int rt_execute_class(Runtime *runtime, JClass *class, Options *options);
// gets the class, loads it if not already loaded
JClass* rt_get_class(Runtime *runtime, String *name);
int rt_execute_static_method(Runtime *runtime, JMethod *method);
JMethod *rt_constant_resolve_methodref(Runtime *rt, Constant *constants, u2 index);
JClass *rt_constant_resolve_class(Runtime *rt, Constant *constants, u2 index);
int read_class_from_bytes(JClass *cl, ByteBuf *buf);
int read_class_from_path(JClass *cl, char *path);

String *cl_constants_get_string(JClass *class, u2 index);
cp_tags cl_constants_get_tag(JClass *class, u2 index);

JMethod *rt_find_method(Runtime *rt, JClass *class, String *name);
JField *rt_find_field(Runtime *rt, JInstance *ins, String *name);
JInstance *instance_create(JClass *class);
int instance_set_field(Runtime *rt, JInstance *instance, u2 index, Value value);
Value instance_get_field(Runtime *rt, JInstance *instance, u2 index);
void instance_free(JInstance *instance);

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
    DUP = 89
} Opcode;

#endif //TINYJVM_JVM_H
