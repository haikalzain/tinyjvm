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


JMethod *rt_find_method(Runtime *rt, JClass *class, String *name) {
    for(int i=0;i<class->n_methods;i++) {
        if(str_compare(name, class->methods[i].name) == 0) {
            return &class->methods[i];
        }
    }
    if(class->super_class != 0) {
        // can this cause infinite loop?
        JClass *c = rt_get_class(rt, cl_constants_get_string(class, class->super_class));
        if(c == NULL) return NULL;
        return rt_find_method(rt, c, name);
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

JClass *rt_constant_resolve_class(Runtime *rt, Constant *constants, u2 index) {
    assert(constants[index].tag == CONSTANT_Class);
    Constant class_str_constant = constants[constants[index].index_info.index];
    JClass *cls = rt_get_class(rt, VAL_GET_STRING(class_str_constant.value));
    if(cls == NULL) {
        jvm_printf("Error resolving class");
        return NULL;
    }
    return cls;
}

// assume that constants have already been verified
JMethod *rt_constant_resolve_methodref(Runtime *rt, Constant *constants, u2 index) {
    // check bounds?
    assert(constants[index].tag == CONSTANT_Methodref);
    Constant name_and_type_constant = constants[constants[index].double_index_info.index2];
    Constant method_constant = constants[name_and_type_constant.double_index_info.index1];
    Constant type_constant = constants[name_and_type_constant.double_index_info.index2];


    JClass *cls = rt_constant_resolve_class(rt, constants, constants[index].double_index_info.index1);
    if(cls == NULL) return NULL;
    JMethod *method = rt_find_method(rt, cls, VAL_GET_STRING(method_constant.value));
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

JInstance *instance_create(JClass *cls) {
    JInstance *ins = jmalloc(sizeof(JInstance));
    // TODO should probably check that malloc is not null
    ins->class = cls;
    ins->fields = jmalloc(sizeof(ins->fields[0]) * cls->n_fields); // 0 indexed?
    return ins;
}

void instance_free(JInstance *ins) {
    jfree(ins->fields);
    jfree(ins);
}

int instance_set_field(JInstance *instance, u2 index, Value value) {
    instance->fields[index] = value;
    return 0;
}

Value instance_get_field(JInstance *instance, u2 index) {
    return instance->fields[index];
}

Value rt_execute_method(Runtime *rt, const JInstance *this, const JMethod *method, Value *args, u1 nargs) {
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
    Value v, v2;
    JClass *c;
    JInstance *ins;

    // Need to type check
    while(1) {
        switch(*ip++) {
            case BIPUSH:
                *sp++ = MKVAL(TYPE_BYTE, *ip++);
                break;
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
            case ASTORE:
            case ISTORE: // TODO should check bounds of index
                index = *ip++;
                locals[index] = *(--sp);
                break;
            case ASTORE_0:
            case ISTORE_0:
                locals[0] = *(--sp);
                break;
            case ASTORE_1:
            case ISTORE_1:
                locals[1] = *(--sp);
                break;
            case ASTORE_2:
            case ISTORE_2:
                locals[2] = *(--sp);
                break;
            case ASTORE_3:
            case ISTORE_3:
                locals[3] = *(--sp);
                break;
            case ALOAD:
            case ILOAD:
                index = *ip++;
                *sp++ = locals[index];
                break;
            case ALOAD_0:
            case ILOAD_0:
                *sp++ = locals[0];
                break;
            case ALOAD_1:
            case ILOAD_1:
                *sp++ = locals[1];
                break;
            case ALOAD_2:
            case ILOAD_2:
                *sp++ = locals[2];
                break;
            case ALOAD_3:
            case ILOAD_3:
                *sp++ = locals[3];
                break;
            case NEW:
                index = read_u2(ip);
                ip += 2;
                c = rt_constant_resolve_class(rt, class->constants, index);
                // should check that this isn't an interface/abstract before trying to instantiate
                assert(c != NULL);
                ins = instance_create(c);
                assert(ins != NULL);
                *sp++ = MKPTR(TYPE_INSTANCE, ins);
                break;
            case GETSTATIC:
                index = read_u2(ip);
                ip += 2;
                break;
            case GETFIELD:
                index = read_u2(ip);
                ip += 2;
                v = *(--sp);
                assert(VAL_GET_TAG(v) == TYPE_INSTANCE);
                ins = VAL_GET_PTR(v);

                // should type check field
                *sp++ = instance_get_field(ins, index);
                break;
            case PUTSTATIC:
                break;
            case PUTFIELD:
                index = read_u2(ip);
                ip += 2;
                v = *(--sp);
                v2 = *(--sp);
                assert(VAL_GET_TAG(v2) == TYPE_INSTANCE);
                ins = VAL_GET_PTR(v2);
                // should type check field
                instance_set_field(ins, index, v);

                break;
            case INVOKEVIRTUAL:
                //break;
            case INVOKESPECIAL:
                index = read_u2(ip);
                ip += 2;
                m = rt_constant_resolve_methodref(rt, class->constants, index);
                if(m == NULL) {
                    jvm_printf("Method resolution failed");
                    return MKPTR(TYPE_EXCEPTION, NULL);
                }
                // subtract number of args
                sp -= m->descriptor.nparams;
                // now get object ref
                v = *(--sp);
                assert(VAL_GET_TAG(v) == TYPE_INSTANCE);
                ins = VAL_GET_PTR(v);
                v = rt_execute_method(rt, ins, m, sp, m->descriptor.nparams + 1);
                if(VAL_GET_TAG(v) == TYPE_EXCEPTION) {
                    return v;
                }
                if(m->descriptor.return_type.type != TYPE_VOID) {
                    *sp++ = v;
                }
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
            case DUP:
                assert(val_is_comp_type1(v));
                *sp = *(sp - 1);
                sp++;
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
    JMethod *main_method = rt_find_method(rt, class, main);
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