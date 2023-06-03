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
    uint32_t hash;
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
    TYPE_VOID = 101,
    TYPE_NULL = 100,
    TYPE_BOOL = 4,
    TYPE_BYTE = 8,
    TYPE_CHAR = 5,
    TYPE_SHORT = 9,
    TYPE_INT = 10,
    TYPE_DOUBLE = 7,
    TYPE_STRING = -1,
    TYPE_LONG = 11,
    TYPE_FLOAT = 6,
    TYPE_CLASS = -2,
    TYPE_INSTANCE = -3,
    TYPE_EXCEPTION = -4,
    TYPE_ARRAY = -5,
    // Internal only
    TYPE_METHOD_DESCRIPTOR = -6,
    TYPE_FIELD_DESCRIPTOR = -7
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

void val_zero(Value *v);

#endif //TINYJVM_STRING_H
