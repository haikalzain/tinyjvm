//
// Created by Haikal Zain on 29/5/2023.
//

#ifndef TINYJVM_STRING_H
#define TINYJVM_STRING_H

#include "types.h"

typedef struct String {
    u2 size;
    u2 cap;
    u1 *data;
} String;

String *str_from_c(char *cstr);
void str_create(String *str, u1 *data, u2 size);
String *str_dup(String *str);
int str_compare(String *s1, String *s2);
int str_compare_raw(String *s1, char *s2);
String *str_concat(String *s1, String *s2);
void str_free(String *str);
int str_ensure_zeropad(String *str);
char *str_cstr(String *str);


// Define some custom types

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
#define MKFLOAT(value) (Value) {.d = value, .tag = TYPE_FLOAT}
#define VAL_VOID MKVAL(TYPE_VOID, 0)
#define VAL_NULL MKVAL(TYPE_NULL, 0)
#define VAL_GET_TAG(v) ((v).tag)
#define VAL_GET_PTR(v) ((v).ptr)
#define VAL_GET_STRING(v) ((String *)v.ptr)

static inline u1 val_is_comp_type1(Value v) {
    if(VAL_GET_TAG(v) != TYPE_LONG && VAL_GET_TAG(v) != TYPE_DOUBLE) return 1;
    return 0;
}

#endif //TINYJVM_STRING_H
