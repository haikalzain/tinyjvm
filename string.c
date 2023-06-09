//
// Created by Haikal Zain on 29/5/2023.
//

#include "string.h"
#include "util.h"
#include <string.h>

void str_create(String *str, u1 *data, u2 size) {
    str->data = data;
    str->size = size;
    str->cap = size;
    str->hash = fnv_32_hash(data, size);
}

String *str_dup(String *str) {
    String *new_str = jmalloc(sizeof(String));
    u1 *data = jmalloc(str->size);
    memcpy(data, str->data, str->size);
    str_create(new_str, data, str->size);
    return new_str;
}

int str_compare(String *s1, String *s2) {
    if(s1->size != s2->size) return -1;
    return memcmp(s1->data, s2->data, s1->size);
}

int str_compare_raw(String *s1, char *s2) {
    if(s1->size != strlen(s2)) return -1;
    return memcmp(s1->data, s2, s1->size);
}

String *str_concat(String *s1, String *s2) {
    u1 *data = jmalloc(s1->size + s2->size);
    if(data == NULL) return NULL;
    memcpy(data, s1->data, s1->size);
    memcpy(data + s1->size, s2->data, s2->size);
    String *str = jmalloc(sizeof(String));
    str_create(str, data, s1->size + s2->size);
    return str;
}

void str_free(String *str) {
    jfree(str->data);
    jfree(str);
}

int str_ensure_zeropad(String *str) {
    if(str->cap > str->size) return 0;
    str->cap = str->size + 1;
    u1 *tmp = jrealloc(str->data, str->cap);
    if(tmp == NULL) {
        jvm_printf("OOM");
        return -1;
    }
    str->data = tmp;
    str->data[str->size] = 0;
    return 0;
}

char *str_cstr(String *str) {
    if(str_ensure_zeropad(str) == -1) {
        return NULL;
    }
    return (char *)str->data;
}

String *str_from_c(char *cstr) {
    String *s = jmalloc(sizeof(String));
    s->data = jmalloc(strlen(cstr));
    if(s == NULL || s->data == NULL) return NULL;
    memcpy(s->data, cstr, strlen(cstr));
    str_create(s, s->data, strlen(cstr));
    return s;
}

void val_zero(Value *v) {
    switch(v->tag) {
        case TYPE_BOOL:
        case TYPE_BYTE:
        case TYPE_CHAR:
        case TYPE_INT:
        case TYPE_LONG:
        case TYPE_SHORT:
            v->l = 0;
            break;
        case TYPE_DOUBLE:
        case TYPE_FLOAT:
            v->f = 0;
            break;

        default:
            *v = VAL_NULL;
    }
}