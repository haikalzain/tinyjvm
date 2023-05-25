//
// Created by Haikal Zain on 10/5/2023.
//
#include <string.h>
#include <alloca.h>
#include <assert.h>
#include <stdlib.h>
#include "jvm.h"
#include "list.h"

void str_create(String *str, u1 *data, u2 size) {
    str->data = data;
    str->size = size;
    str->cap = size;
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


JMethod *cl_find_method(JClass *class, String *name) {
    for(int i=0;i<class->n_methods;i++) {
        int index = class->methods[i].name_index;

        if(str_compare(name, (String *)VAL_GET_PTR(class->constants[index].value)) == 0) {
            return &class->methods[i];
        }
    }

    return NULL;
}

static inline u2 read_u2(const uint8_t *ip) {
    u2 ret = *ip << 8 | *(ip + 1);
    return ret;
}

JClass *rt_load_class(Runtime *rt, String *name) {
    for(u2 i=0;i<rt->n_classpaths;i++) {
        String *slash = str_from_c("/");
        String *class = str_from_c(".class");
        String *path = str_concat(rt->classpaths[i], slash);
        String *full_path = str_concat(path, name);
        String *filename = str_concat(full_path, class);

        JClass *cls = jmalloc(sizeof(JClass));
        if(cls == NULL) return NULL;
        int ret = read_class_from_path(cls, str_cstr(filename));

        str_free(slash);
        str_free(class);
        str_free(path);
        str_free(full_path);
        str_free(filename);

        if(ret == 0) {
            cls->name = name;
            return cls;
        }
    }
    jvm_printf("Class %s not found in classpath\n", name);
    return NULL;
}

JClass *rt_get_class(Runtime *rt, String *name) {
    JClass *cls;
    list_for_each(cls, rt->cache) {
        if(str_compare(cls->name, name) == 0) {
            return cls;
        }
    }

    cls = rt_load_class(rt, name);
    cls->next = rt->cache;
    rt->cache = cls;
    return cls;
}

// assume that constants have already been verified
JMethod *rt_constant_resolve_methodref(Runtime *rt, Constant *constants, u2 index) {
    // check bounds?
    assert(constants[index].tag == CONSTANT_Methodref);
    Constant class_constant = constants[constants[index].double_index_info.index1];
    Constant name_and_type_constant = constants[constants[index].double_index_info.index2];
    Constant class_str_constant = constants[class_constant.index_info.index];
    Constant method_constant = constants[name_and_type_constant.double_index_info.index1];
    Constant type_constant = constants[name_and_type_constant.double_index_info.index2];


    JClass *cls = rt_get_class(rt, VAL_GET_STRING(class_str_constant.value));
    if(cls == NULL) {
        jvm_printf("Error resolving class");
        return NULL;
    }
    JMethod *method = cl_find_method(cls, VAL_GET_STRING(method_constant.value));
    // assume descriptor is legit
    if(!method->descriptor_cached) {
        ByteBuf buf;
        String *descriptor_str = VAL_GET_STRING(type_constant.value);
        bytebuf_create(&buf, descriptor_str->data, descriptor_str->size);
        if(parse_method_descriptor(&method->descriptor, &buf) == -1) {
            // cleanup
            return NULL;
        }
        method->descriptor_cached = 1;
    }
    return method;
}

Value rt_execute_method(Runtime *rt, const JInstance *instance, const JMethod *method, Value *args, u1 nargs) {
    JClass *class = method->class;
    // load the code
    // why do we even need the cf?
    // should just alloca the stack frame and locals
    Value *locals = alloca(sizeof(locals[0]) * method->code.max_locals);
    if(locals == NULL) {
        return MKPTR(TYPE_EXCEPTION, NULL);
    }
    Value *stack = alloca(sizeof(stack[0]) * method->code.max_stack);
    if(stack == NULL) {
        return MKPTR(TYPE_EXCEPTION, NULL);
    }

    // stack points to empty item. use *(sp-1) to get top
    Value *sp = stack;
    u1 *ip  = method->code.code;
    // copy in args
    memcpy(locals, args, nargs * sizeof(args[0]));

    u8 index;
    Constant constant;
    JMethod *m;
    Value v;

    // Need to type check
    while(1) {
        switch(*ip++) {
            case ICONST_m1:
                *sp++ = MKVAL(TYPE_INT, -1);
                break;
            case ICONST_0:
                *sp++ = MKVAL(TYPE_INT, 0);
                break;
            case ICONST_1:
                *sp++ = MKVAL(TYPE_INT, 1);
                break;
            case ICONST_2:
                *sp++ = MKVAL(TYPE_INT, 2);
                break;
            case ICONST_3:
                *sp++ = MKVAL(TYPE_INT, 3);
                break;
            case ICONST_4:
                *sp++ = MKVAL(TYPE_INT, 4);
                break;
            case ICONST_5:
                *sp++ = MKVAL(TYPE_INT, 5);
                break;
            case ISTORE: // TODO should check bounds of index
                index = *ip++;
                locals[index] = *(--sp);
                break;
            case ISTORE_0:
                locals[0] = *(--sp);
                break;
            case ISTORE_1:
                locals[1] = *(--sp);
                break;
            case ISTORE_2:
                locals[2] = *(--sp);
                break;
            case ISTORE_3:
                locals[3] = *(--sp);
                break;

            case ILOAD:
                index = *ip++;
                *sp++ = locals[index];
                break;
            case ILOAD_0:
                *sp++ = locals[0];
                break;
            case ILOAD_1:
                *sp++ = locals[1];
                break;
            case ILOAD_2:
                *sp++ = locals[2];
                break;
            case ILOAD_3:
                *sp++ = locals[3];
                break;
            case GETSTATIC:
                index = read_u2(ip);
                ip += 2;
                break;
            case GETFIELD:
                break;
            case PUTSTATIC:
                break;
            case PUTFIELD:
                break;
            case INVOKEVIRTUAL:
                break;
            case INVOKESTATIC:
                index = read_u2(ip);
                ip += 2;
                m = rt_constant_resolve_methodref(rt, class->constants, index);
                if(m == NULL) {
                    jvm_printf("Method resolution failed");
                    return MKPTR(TYPE_EXCEPTION, NULL);
                }
                // subtract number of args
                sp -= m->descriptor.nparams;
                v = rt_execute_method(rt, NULL, m, sp, m->descriptor.nparams);
                if(VAL_GET_TAG(v) == TYPE_EXCEPTION) {
                    return v;
                }
                if(m->descriptor.return_type.type != TYPE_VOID) {
                    *sp++ = v;
                }
                break;
            case RETURN:
                return VAL_VOID;
            case IRETURN:
                assert(VAL_GET_TAG(*(sp - 1)) == TYPE_INT);
                return *(sp - 1);
            case IADD:
                assert(VAL_GET_TAG(*(sp - 1)) == TYPE_INT);
                assert(VAL_GET_TAG(*(sp - 2)) == TYPE_INT);
                sp--;
                (sp - 1)->i += sp->i;
                break;
            default:
                jvm_printf("Unimplemented op %d\n", *(ip - 1));
                abort();

        }
    }

    error:
    return MKPTR(TYPE_EXCEPTION, NULL);
}

int rt_execute_class(Runtime *rt, JClass *class, Options *options) {
    // this is wrong somehow - should create rt_init
    rt->cache = NULL;
    rt->n_classpaths = 1;
    rt->classpaths = jmalloc(sizeof(rt->classpaths[0]) * rt->n_classpaths);
    rt->classpaths[0] = str_from_c(".");

    String *main = str_from_c("main");
    JMethod *main_method = cl_find_method(class, main);
    if(main_method == NULL) {
        jvm_printf("Cannot find main method\n");
        return -1;
    }
    if(main_method->access_flags != (METHOD_PUBLIC | METHOD_STATIC)) {
        jvm_printf("Main class has invalid access flags");
        // check return type and params as well
        return -1;
    }
    Value args[] = {MKPTR(TYPE_CLASS, class)};
    Value ret = rt_execute_method(rt, NULL, main_method, args, 1);
    if(VAL_GET_TAG(ret) == TYPE_EXCEPTION) {
        jvm_printf("Exception");
        return -1;
    }

    return 0;
}