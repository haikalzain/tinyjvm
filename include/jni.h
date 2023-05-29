//
// Created by Haikal Zain on 29/5/2023.
//

#ifndef TINYJVM_JNI_H
#define TINYJVM_JNI_H

#include <stdint.h>

#define JNIEXPORT
#define JNICALL // only for windows __stdcall

typedef int32_t jint;
typedef int64_t jlong;
typedef signed char jbyte;
typedef unsigned char   jboolean;
typedef unsigned short  jchar;
typedef short           jshort;
typedef float           jfloat;
typedef double          jdouble;

typedef jint            jsize;

typedef struct _jobject *jobject;
typedef jobject jclass;

typedef struct _JNIEnv JNIEnv;

struct _jobject {

};

struct _JNIEnv {

};

#endif //TINYJVM_JNI_H
