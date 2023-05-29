//
// Created by Haikal Zain on 29/5/2023.
//

#include "jni.h"
#include <stdio.h>

JNIEXPORT jclass JNICALL Java_java_lang_Object_getClass
        (JNIEnv *env, jclass cls) {
    printf("Hello world!\n");
}