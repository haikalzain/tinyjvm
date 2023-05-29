//
// Created by Haikal Zain on 27/5/2023.
//

#ifndef TINYJVM_NATIVE_H
#define TINYJVM_NATIVE_H

#include "string.h"
#include "jni.h"

typedef struct NativeLib {
    struct NativeLib *next;
    void *handle;
    char *path;
} NativeLib;
typedef struct NativeLibs {
    NativeLib *libs;
} NativeLibs;

void native_libs_init(NativeLibs *native_libs);
int native_libs_load(NativeLibs *native_libs, char *path);
void *native_libs_find_method(NativeLibs *native_libs, String *cls, String *method);
Value native_method_invoke(void *method, JNIEnv *env, Value *args, u1 nargs);

#endif //TINYJVM_NATIVE_H
