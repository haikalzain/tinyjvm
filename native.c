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

typedef void * (*_invoke1)(JNIEnv *env, void * v1);
typedef void * (*_invoke2)(JNIEnv *env, void * v1, void * v2);
typedef void * (*_invoke3)(JNIEnv *env, void * v1, void * v2, void * v3);
typedef void * (*_invoke4)(JNIEnv *env, void * v1, void * v2, void * v3, void * v4);
typedef void * (*_invoke5)(JNIEnv *env, void * v1, void * v2, void * v3, void * v4, void * v5);
typedef void * (*_invoke6)(JNIEnv *env, void * v1, void * v2, void * v3, void * v4, void * v5, void * v6);
typedef void * (*_invoke7)(JNIEnv *env, void * v1, void * v2, void * v3, void * v4, void * v5, void * v6, void * v7);
typedef void * (*_invoke8)(JNIEnv *env, void * v1, void * v2, void * v3, void * v4, void * v5, void * v6, void * v7, void * v8);

Value native_method_invoke(void *method, ValueType return_type, JNIEnv *env, Value *args, u1 nargs) {
    // we are playing loose with void * here
    // also dangerous to return a Value from void return type. should return VAL_VOID
    // if more than a certain number of args, need an asm shim
    // floating points don't work due to function call convention requiring two arguments.
    // TODO make sure that instance, class is handled correctly
    Value ret;
    switch(nargs) {
        case 1:
            ret.ptr = ((_invoke1)method)(env, args[0].ptr); break;
        case 2:
            ret.ptr = ((_invoke2)method)(env, args[0].ptr, args[1].ptr); break;
        case 3:
            ret.ptr = ((_invoke3)method)(env, args[0].ptr, args[1].ptr, args[2].ptr); break;
        case 4:
            ret.ptr = ((_invoke4)method)(env, args[0].ptr, args[1].ptr, args[2].ptr, args[3].ptr); break;
        case 5:
            ret.ptr = ((_invoke5)method)(env, args[0].ptr, args[1].ptr, args[2].ptr, args[3].ptr, args[4].ptr); break;
        case 6:
            ret.ptr = ((_invoke6)method)(env, args[0].ptr, args[1].ptr, args[2].ptr, args[3].ptr, args[4].ptr, args[5].ptr); break;
        case 7:
            ret.ptr = ((_invoke7)method)(env, args[0].ptr, args[1].ptr, args[2].ptr, args[3].ptr, args[4].ptr, args[5].ptr, args[6].ptr); break;
        case 8:
            ret.ptr = ((_invoke8)method)(env, args[0].ptr, args[1].ptr, args[2].ptr, args[3].ptr, args[4].ptr, args[5].ptr, args[6].ptr, args[7].ptr); break;
        default:
            jvm_printf("Invalid args %d\n", nargs);
            return MKPTR(TYPE_EXCEPTION, NULL);
    }
    // unsafe
    ret.tag = return_type;
    return ret;
}

