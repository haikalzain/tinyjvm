//
// Created by Haikal Zain on 27/5/2023.
//

#include <dlfcn.h>
#include <stddef.h>
#include <string.h>
#include "native.h"
#include "util.h"
#include "list.h"
#include "jvm.h"
#include "jni.h"

void native_libs_init(NativeLibs *native_libs) {
    native_libs->libs = NULL;
}

int native_libs_load(NativeLibs *nativeLibs, char *path) {
    NativeLib *lib = jmalloc(sizeof(NativeLib));
    if(lib == NULL) return -1;
    void *handle = dlopen(path, RTLD_LAZY);
    if(handle == NULL) {
        jvm_printf("Failed to load native lib: %s\n", path);
        return -1;
    }
    lib->handle = handle;
    lib->path = path; // this could be a bad idea
    lib->next = nativeLibs->libs;
    nativeLibs->libs = lib;
    return 0;
}

static char *mangle_method(String *cls, String *method) {
    char *java = "Java";
    char *str = jmalloc(4 + 1 + cls->size + 1 + method->size + 1);
    memcpy(str, java, 4);
    str[4] = '_';
    memcpy(str + 5, cls->data, cls->size);
    for(int i=0;i<cls->size;i++) {
        if(str[5 + i] == '/') str[5 + i] = '_';
    }
    str[5 + cls->size] = '_';
    memcpy(str + 5 + cls->size + 1, method->data, method->size);
    str[5 + cls->size + 1 + method->size] = '\0';
    return str;
}

void *native_libs_find_method(NativeLibs *native_libs, String *cls, String *method) {
    char *mangled = mangle_method(cls, method);
    dlerror(); // clear last error
    NativeLib *el;
    void *ret;
    char *err;
    list_for_each(el, native_libs->libs) {
        dlerror();
        ret = dlsym(el->handle, mangled);
        err = dlerror();
        if(err == NULL) {
            jfree(mangled);
            return ret;
        }
    }
    jfree(mangled);
    jvm_printf("Could not find method: %s\n", method);
    return NULL;
}

typedef Value (*_invoke1)(JNIEnv *env, Value v1);
typedef Value (*_invoke2)(JNIEnv *env, Value v1, Value v2);
typedef Value (*_invoke3)(JNIEnv *env, Value v1, Value v2, Value v3);
typedef Value (*_invoke4)(JNIEnv *env, Value v1, Value v2, Value v3, Value v4);
typedef Value (*_invoke5)(JNIEnv *env, Value v1, Value v2, Value v3, Value v4, Value v5);
typedef Value (*_invoke6)(JNIEnv *env, Value v1, Value v2, Value v3, Value v4, Value v5, Value v6);
typedef Value (*_invoke7)(JNIEnv *env, Value v1, Value v2, Value v3, Value v4, Value v5, Value v6, Value v7);
typedef Value (*_invoke8)(JNIEnv *env, Value v1, Value v2, Value v3, Value v4, Value v5, Value v6, Value v7, Value v8);

Value native_method_invoke(void *method, JNIEnv *env, Value *args, u1 nargs) {
    // we are playing loose with void * here
    // also dangerous to return a Value from void return type. should return VAL_VOID
    // if more than a certain number of args, need an asm shim
    switch(nargs) {
        case 1:
            return ((_invoke1)method)(env, args[0]);
        case 2:
            return ((_invoke2)method)(env, args[0], args[1]);
        case 3:
            return ((_invoke3)method)(env, args[0], args[1], args[2]);
        case 4:
            return ((_invoke4)method)(env, args[0], args[1], args[2], args[3]);
        case 5:
            return ((_invoke5)method)(env, args[0], args[1], args[2], args[3], args[4]);
        case 6:
            return ((_invoke6)method)(env, args[0], args[1], args[2], args[3], args[4], args[5]);
        case 7:
            return ((_invoke7)method)(env, args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
        case 8:
            return ((_invoke8)method)(env, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7]);
        default:
            jvm_printf("Invalid args %d\n", nargs);
            return MKPTR(TYPE_EXCEPTION, NULL);
    }
}

