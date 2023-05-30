//
// Created by Haikal Zain on 30/5/2023.
//

#include "jni.h"

JNIEXPORT jint JNICALL Java_NativeBasic_addInt
        (JNIEnv *env, jint x, jint y) {
    return x + y;
}

JNIEXPORT jfloat JNICALL Java_NativeBasic_addFloat
        (JNIEnv *env, jfloat x, jfloat y) {
    return x + y;
}

JNIEXPORT jdouble JNICALL Java_NativeBasic_addDouble
        (JNIEnv *env, jdouble x, jdouble y) {
    return x + y;
}
