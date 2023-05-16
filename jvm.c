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


JMethod *locate_method(JClass *class) {
    for(int i=0;i<class->n_methods;i++) {
        int index = class->methods[i].name_index;
        String main;
        str_create(&main, "main", 4);
        if(str_compare(&main, (String *)class->constants[index].value.as.ptr) == 0) {
            return &class->methods[i];
        }
    }

    return NULL;
}

int jvm_execute_class(JClass *class, Options *options) {
    // init runtime
    Runtime runtime;
    //find main function
    //runtime_execute_method(&runtime, class->)


    /* for now lets do a canned implementation
       find main, make sure its static
       ex
    */
    JMethod *main_method = locate_method(class);
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


}