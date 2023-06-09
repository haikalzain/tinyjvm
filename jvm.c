//
// Created by Haikal Zain on 10/5/2023.
//
#include <string.h>
#include <alloca.h>
#include <assert.h>
#include <stdlib.h>
#include "jvm.h"
#include "list.h"

JMethod *rt_find_method(Runtime *rt, JClass *class, String *name) {
    JMethod *m = ht_get(&class->methods_table, name);
    if(m != NULL) {
        return m;
    }
    if(class->super_class != 0) {
        // can this cause infinite loop?
        JClass *c = rt_constant_resolve_class(rt, class->constants, class->super_class);
        if(c == NULL) return NULL;
        return rt_find_method(rt, c, name);
    }

    return NULL;
}

static inline u2 read_u2(const uint8_t *ip) {
    u2 ret = *ip << 8 | *(ip + 1);
    return ret;
}

JClass *_rt_load_class(Runtime *rt, String *name) {
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
            cls->name = str_dup(name);
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

    cls = _rt_load_class(rt, name);
    cls->next = rt->cache;
    rt->cache = cls;

    // clinit if not already done
    if(cls->status == CLASS_LOADING) {
        cls->status = CLASS_INITING;
        // set fields to 0
        ht_init(&cls->static_fields);
        for(int i=0;i<cls->n_fields;i++) {
            if(cls->fields[i].access_flags & FIELD_STATIC) {
                Value *v = jmalloc(sizeof(Value));
                v->tag = cls->fields[i].type.type;
                val_zero(v);
                ht_put(&cls->static_fields, cls->fields[i].name, v);
            }
        }
        // run clinit
        String *clinit_str = str_from_c("<clinit>");
        JMethod *clinit = rt_find_method(rt, cls, clinit_str);
        if (clinit != NULL) {
            Value v = rt_execute_method(rt, NULL, clinit, NULL, 0);
            if (v.tag == TYPE_EXCEPTION) {
                jvm_printf("Exception occurred in <clinit>\n");
                return NULL;
            }
        }
        str_free(clinit_str);
        cls->status = CLASS_LOADED;
    }
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

int rt_constant_resolve_fieldtype(FieldType *type, Constant *constants, u2 index) {
    assert(constants[index].tag == CONSTANT_Class);
    Constant class_str_constant = constants[constants[index].index_info.index];
    ByteBuf buf;
    String *str = VAL_GET_STRING(class_str_constant.value);
    bytebuf_create(&buf, str->data, str->size);

    if(parse_field_type(type, &buf) == -1) {
        jvm_printf("Error parsing field type");
        return -1;
    }
    return 0;
}

// assume that constants have already been verified
JMethod *rt_constant_resolve_methodref(Runtime *rt, Constant *constants, u2 index) {
    // check bounds?
    assert(constants[index].tag == CONSTANT_Methodref);
    Constant name_and_type_constant = constants[constants[index].double_index_info.index2];
    Constant method_constant = constants[name_and_type_constant.double_index_info.index1];

    JClass *cls = rt_constant_resolve_class(rt, constants, constants[index].double_index_info.index1);
    if(cls == NULL) return NULL;
    JMethod *method = rt_find_method(rt, cls, VAL_GET_STRING(method_constant.value));

    return method;
}

static void instance_init_fields(Runtime *rt, JInstance *ins, JClass *cls) {
    for(int i=0;i<cls->n_fields;i++) {
        if(cls->fields[i].access_flags & FIELD_STATIC) continue;

        if(ht_get(&ins->fields_table, cls->fields[i].name) != NULL) continue;
        Value *v = jmalloc(sizeof(Value));
        v->tag = cls->fields[i].type.type;
        val_zero(v);
        ht_put(&ins->fields_table, cls->fields[i].name, v);
    }
    if(cls->super_class) {
        JClass *c = rt_constant_resolve_class(rt, cls->constants, cls->super_class);
        assert(c != NULL);
        instance_init_fields(rt, ins, c);
    }
}

JInstance *instance_create(Runtime *rt, JClass *cls) {
    JInstance *ins = jmalloc(sizeof(JInstance));
    // TODO should probably check that malloc is not null
    ins->class = cls;
    ht_init(&ins->fields_table);
    instance_init_fields(rt, ins, cls);
    return ins;
}

void instance_free(JInstance *ins) {
    // need to delete fields_table
    jfree(ins);
}

JArray *array_create(u4 length, FieldType type) {
    assert(type.dim == 1);
    JArray *arr = jmalloc(sizeof(JArray));
    arr->type = type;
    arr->length = length;
    arr->data = jmalloc(sizeof(arr->data[0]) * length);
    for(u4 i=0;i<arr->length;i++) {
        arr->data[i].tag = type.type;
        val_zero(&arr->data[i]);
    }
    return arr;
}

// horribly inefficient
JArray *multiarray_create(Value *lengths, int n_lengths, FieldType type) {
    assert(n_lengths > 0);
    if(n_lengths == 1) {
        return array_create(lengths[0].i, type);
    }
    JArray *arr = jmalloc(sizeof(JArray));
    arr->type = type;
    arr->length = lengths[0].i;
    arr->data = jmalloc(sizeof(arr->data[0]) * lengths[0].i);
    type.dim--;
    for(int i=0;i<lengths[0].i;i++) {
        arr->data[i] = MKPTR(TYPE_ARRAY, multiarray_create(lengths + 1, n_lengths - 1, type));
    }
    return arr;
}

static Value *_cls_get_field(Runtime *rt, JClass *cls, String *name) {
    Value *v = ht_get(&cls->static_fields, name);
    if(v != NULL) {
        return v;
    }
    if(cls->super_class != 0) {
        JClass *super = rt_constant_resolve_class(rt, cls->constants, cls->super_class);
        return _cls_get_field(rt, super, name);
    }

    return NULL;
}

int cls_set_field(Runtime *rt, JClass *cls, u2 index, Value value) {
    Constant field_ref = cls->constants[index];
    JClass *c = rt_constant_resolve_class(rt, cls->constants, field_ref.double_index_info.index1);

    Constant name_and_descriptor = cls->constants[field_ref.double_index_info.index2];
    String *name = cl_constants_get_string(cls, name_and_descriptor.double_index_info.index1);
    Value *v = _cls_get_field(rt, c, name);
    assert(v != NULL);
    *v = value;

    return 0;
}

Value cls_get_field(Runtime *rt, JClass *cls, u2 index) {
    Constant field_ref = cls->constants[index];
    JClass *c = rt_constant_resolve_class(rt, cls->constants, field_ref.double_index_info.index1);

    Constant name_and_descriptor = cls->constants[field_ref.double_index_info.index2];
    String *name = cl_constants_get_string(cls, name_and_descriptor.double_index_info.index1);
    Value *v = _cls_get_field(rt, c, name);
    assert(v != NULL);

    return *v;
}

int instance_set_field(Runtime *rt, JInstance *instance, u2 index, Value value) {
    Constant field_ref = instance->class->constants[index];
    Constant name_and_descriptor = instance->class->constants[field_ref.double_index_info.index2];
    String *name = cl_constants_get_string(instance->class, name_and_descriptor.double_index_info.index1);
    Value *v = ht_get(&instance->fields_table, name);
    assert(v != NULL);
    *v = value;

    return 0;
}

Value instance_get_field(Runtime *rt, JInstance *instance, u2 index) {
    Constant field_ref = instance->class->constants[index];
    Constant name_and_descriptor = instance->class->constants[field_ref.double_index_info.index2];
    String *name = cl_constants_get_string(instance->class, name_and_descriptor.double_index_info.index1);
    Value *v = ht_get(&instance->fields_table, name);
    assert(v != NULL);
    return *v;
}


Value rt_execute_java_method(Runtime *rt, const JInstance *this, const JMethod *method, Value *args, u1 nargs) {
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
    FieldType field_type;
    int x;
    JArray *arr;

    // Need to type check
    while(1) {
        switch(*ip++) {
            case BIPUSH:
                // promote to int
                *sp++ = MKVAL(TYPE_INT, *ip++);
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
                ins = instance_create(rt, c);
                assert(ins != NULL);
                *sp++ = MKPTR(TYPE_INSTANCE, ins);
                break;
            case GETSTATIC:
                index = read_u2(ip);
                ip += 2;
                *sp++ = cls_get_field(rt, class, index);
                break;
            case GETFIELD:
                index = read_u2(ip);
                ip += 2;
                v = *(--sp);
                assert(VAL_GET_TAG(v) == TYPE_INSTANCE);
                ins = VAL_GET_PTR(v);

                // should type check field
                *sp++ = instance_get_field(rt, ins, index);
                break;
            case PUTSTATIC:
                index = read_u2(ip);
                ip += 2;
                v = *(--sp);
                cls_set_field(rt, class, index, v);
                break;
            case PUTFIELD:
                index = read_u2(ip);
                ip += 2;
                v = *(--sp);
                v2 = *(--sp);
                assert(VAL_GET_TAG(v2) == TYPE_INSTANCE);
                ins = VAL_GET_PTR(v2);
                // should type check field
                instance_set_field(rt, ins, index, v);

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
            case POP:
                sp--;
                break;
            case POP2: // type2 takes only 1 slot in our impl
                sp--;
                if(val_is_comp_type1(*sp)) sp--;
                break;
            case NEWARRAY:
                field_type.type = *ip++;
                field_type.dim = 1;
                assert(VAL_GET_TAG(*(sp - 1)) == TYPE_INT);
                *(sp - 1) = MKPTR(TYPE_ARRAY, array_create((sp - 1)->i, field_type));
                break;
            case ANEWARRAY:
                index = read_u2(ip);
                ip += 2;
                rt_constant_resolve_fieldtype(&field_type, class->constants, index);
                *(sp - 1) = MKPTR(TYPE_ARRAY, array_create((sp - 1)->i, field_type));
                break;
            case MULTINEWARRAY:
                index = read_u2(ip);
                ip += 2;
                x = *ip++;
                rt_constant_resolve_fieldtype(&field_type, class->constants, index);
                arr = multiarray_create( sp - x, x, field_type);
                sp -= x;
                *sp++ = MKPTR(TYPE_ARRAY, arr);
                break;
            case IALOAD:
            case AALOAD:
                sp--;
                index = sp->i;
                assert(VAL_GET_TAG(*(sp - 1)) == TYPE_ARRAY);
                arr = VAL_GET_PTR(*(sp - 1));
                *(sp - 1) = arr->data[index];
                break;
            case AASTORE:
            case IASTORE:
                sp--;
                v = *sp;
                sp--;
                index = sp->i;
                assert(VAL_GET_TAG(*(sp - 1)) == TYPE_ARRAY);
                arr = VAL_GET_PTR(*(sp - 1));
                arr->data[index] = v;
                sp--;
                break;
            default:
                jvm_printf("Unimplemented op %d\n", *(ip - 1));
                abort();

        }
    }

    error:
    return MKPTR(TYPE_EXCEPTION, NULL);
}

Value rt_execute_method(Runtime *rt, const JInstance *this, const JMethod *method, Value *args, u1 nargs) {
    if(method->access_flags & METHOD_NATIVE) {
        void *native_method = native_libs_find_method(&rt->native_libs, method->class->name, method->name);
        if(native_method == NULL) {
            return MKVAL(TYPE_EXCEPTION, 0);
        }
        return native_method_invoke(native_method, method->descriptor.return_type.type, &rt->env, args, nargs);
    }

    return rt_execute_java_method(rt, this, method, args, nargs);
}

int rt_init(Runtime *rt, Options *options) {
    rt->cache = NULL;
    rt->n_classpaths = 1;
    rt->classpaths = jmalloc(sizeof(rt->classpaths[0]) * rt->n_classpaths);
    rt->classpaths[0] = str_from_c(".");
    native_libs_init(&rt->native_libs);
    native_libs_load(&rt->native_libs, "/usr/local/lib/tinyjvm/libjava.dylib");
    if(options != NULL) {
        for (int i = 0; i < options->n_native_libs; i++) {
            native_libs_load(&rt->native_libs, options->native_libs[i]);
        }
    }
    return 0;
}

Value execute_static_method(Runtime *rt, char *cls_name, char *method_name, Value *args, u2 nargs) {
    String *cls_str = str_from_c(cls_name);
    String *method_str = str_from_c(method_name);
    JClass *cls = rt_get_class(rt, cls_str);
    if(cls == NULL) {
        return MKVAL(TYPE_EXCEPTION, 0);
    }
    JMethod *m = rt_find_method(rt, cls, method_str);
    if(m == NULL) {
        return MKVAL(TYPE_EXCEPTION, 0);
    }
    Value v = rt_execute_method(rt, NULL, m, args, nargs);
    str_free(cls_str);
    str_free(method_str);
    return v;
}