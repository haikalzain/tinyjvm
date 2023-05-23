//
// Created by Haikal Zain on 10/5/2023.
//
#include <string.h>
#include "jvm.h"

void str_create(String *str, u1 *data, u2 size) {
    str->data = data;
    str->size = size;
}

// need to execute either static method or instance method
int runtime_execute_method(Runtime *runtime, String *class_name, String *method_name) {

}

int str_compare(String *s1, String *s2) {
    if(s1->size != s2->size) return -1;
    return memcmp(s1->data, s2->data, s1->size);
}

int str_compare_raw(String *s1, char *s2) {
    if(s1->size != strlen(s2)) return -1;
    return memcmp(s1->data, s2, s1->size);
}


JMethod *cl_find_method(JClass *class, String *name) {
    for(int i=0;i<class->n_methods;i++) {
        int index = class->methods[i].name_index;

        if(str_compare(name, (String *)VAL_GET_PTR(class->constants[index].value)) == 0) {
            return &class->methods[i];
        }
    }

    return NULL;
}

void cf_from_static_method(CallFrame *cf, JMethod *method, CallFrame *parent) {
    cf->parent = parent;
    cf->instance = NULL;
    cf->method = method;
    cf->ip = cf->method->code.code;

    // setup the stack and local variables
}

void cf_from_instance_method(CallFrame *cf, JInstance *instance, JMethod *method, CallFrame *parent) {
    cf->parent = parent;
    cf->instance = instance;
    cf->method = method;
    cf->ip = cf->method->code.code;
}

int jvm_execute_class(Runtime *runtime, JClass *class, Options *options) {
    /* for now lets do a canned implementation
       get static methods working first
    */
    String main;
    str_create(&main, "main", 4);
    JMethod *main_method = cl_find_method(class, &main);
    if(main_method == NULL) {
        jvm_printf("Cannot find main method\n");
        return -1;
    }
    if(main_method->access_flags != (METHOD_PUBLIC | METHOD_STATIC)) {
        jvm_printf("Main class has invalid access flags");
        // check return type and params as well
        return -1;
    }

    // load the code
    CallFrame root_cf;
    cf_from_static_method(&root_cf, main_method, NULL);
    CallFrame *cf = &root_cf;

    while(1) {
        switch(*cf->ip) {

        }
    }


    return 0;

    error:
    return -1;
}