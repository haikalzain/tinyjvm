//
// Created by Haikal Zain on 10/5/2023.
//
#include "jvm.h"

void str_create(String *str, u1 *data, u2 size) {
    str->data = data;
    str->size = size;
}