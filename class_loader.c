//
// Created by Haikal Zain on 9/5/2023.
//
#include <assert.h>
#include "jvm.h"
#include "util.h"


int parse_constant(Constant *c, ByteBuf *buf) {
    u1 tag = bytebuf_read(buf);
    union {
        int32_t i;
        CONSTANT_double_index_info di;
        CONSTANT_index_info si;
        CONSTANT_kind_index_info ki;
    }u;
    switch(tag) {
        case CONSTANT_Integer:
            c->value = MKVAL(TYPE_INT, bytebuf_readu4(buf));
            break;
        case CONSTANT_Float:
            c->value = MKVAL(TYPE_FLOAT, bytebuf_readu4(buf));
            break;
        case CONSTANT_Long:
            c->value = MKVAL(TYPE_LONG, bytebuf_readu8(buf));
            break;
        case CONSTANT_Double:
            c->value = MKVAL(TYPE_DOUBLE, bytebuf_readu8(buf));
            break;
        case CONSTANT_Utf8: {
            u2 size = bytebuf_readu2(buf);
            u1 *data = jmalloc(size);
            bytebuf_readbytes(buf, data, size);
            String *str = jmalloc(sizeof(String));
            str_create(str, data, size);
            c->value = MKPTR(TYPE_STRING, str);
            break;
        }

        case CONSTANT_MethodHandle:
            c->kind_index_info.kind = bytebuf_read(buf);
            c->kind_index_info.index = bytebuf_readu2(buf);
            break;
        case CONSTANT_String:
        case CONSTANT_MethodType:
        case CONSTANT_Class:
        case CONSTANT_Module:
        case CONSTANT_Package:
            c->index_info.index = bytebuf_readu2(buf);
            break;

        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
        case CONSTANT_NameAndType:
        case CONSTANT_Dynamic:
        case CONSTANT_InvokeDynamic:
            c->double_index_info.index1 = bytebuf_readu2(buf);
            c->double_index_info.index2 = bytebuf_readu2(buf);
        break;

        default:
            jvm_printf("Unrecognized constant type %d\n", tag);
            return -1;
    }
    c->tag = tag;
    return 0;
}

int parse_field_type(FieldType *field_type, ByteBuf *buf) {
    u1 c = bytebuf_read(buf);
    u1 *data;
    u1 n = 0;
    u2 l, j;
    field_type->dim = 0;
    while(c == '[') {
        field_type->dim++;
        c = bytebuf_read(buf);
    }
    switch(c) {
        case 'B':
            field_type->type = TYPE_BYTE;
            break;
        case 'C':
            field_type->type = TYPE_CHAR;
            break;
        case 'D':
            field_type->type = TYPE_DOUBLE;
            break;
        case 'F':
            field_type->type = TYPE_FLOAT;
            break;
        case 'I':
            field_type->type = TYPE_INT;
            break;
        case 'J':
            field_type->type = TYPE_LONG;
            break;
        case 'S':
            field_type->type = TYPE_SHORT;
            break;
        case 'Z':
            field_type->type = TYPE_BOOL;
            break;
        case 'L':
            l = 0;
            while(bytebuf_read(buf) != ';') l++;
            data = jmalloc(l);
            j = 0;
            buf->off -= l + 1;
            while((c = bytebuf_read(buf)) != ';') {
                data[j++] = c;
            }
            field_type->type = TYPE_INSTANCE;
            str_create(&field_type->class_name, data, l);
            break;
        case 'V':
            field_type->type = TYPE_VOID;
            break;
        default:
            return -1;

    }
    return 0;
}

int parse_method_descriptor(JMethodDescriptor *d, ByteBuf *buf) {
    // BEWARE: the 2 switch blocks are not identical, return type also supports V
    assert(bytebuf_read(buf) == '(');
    u1 c;
    u1 n = 0;
    u2 l, j;
    u1 * data;
    while((c = bytebuf_read(buf)) != ')') {
        if(n == 255) {
            return -1;
        }
        switch(c) {
            case 'B':
            case 'C':
            case 'D':
            case 'F':
            case 'I':
            case 'J':
            case 'S':
            case 'Z':
                break;
            case 'L':
                while(bytebuf_read(buf) != ';');
                break;
            case '[':
                n--;
                break;
            default:
                return -1;

        }
        n++;
    }

    buf->off = 1;
    d->nparams = n;
    d->field_types = jmalloc(sizeof(d->field_types[0]) * n);
    for(int i=0;i<n;i++) {
        // should reject if void
        if(parse_field_type(&d->field_types[i], buf) == -1) {
            jfree(d->field_types);
            return -1;
        }
    }
    bytebuf_read(buf);

    // return type
    if(parse_field_type(&d->return_type, buf) == -1) {
        return -1;
    }
    assert(buf->off == buf->size);
    return 0;
}

int parse_constant_pool(ClassFile *cl, ByteBuf *buf) {
    cl->n_constants = bytebuf_readu2(buf);
    cl->constants = (Constant *)jmalloc(cl->n_constants * sizeof(Constant));
    // need to set cl->constants[0]
    for(int i = 1;i<cl->n_constants;i++) {
        Constant cons;
        int result = parse_constant(&cons, buf);
        cl->constants[i] = cons;
        if(result == -1) {
            jvm_printf("Could not parse constant\n");
            return -1;
        }
    }
    // should error (for all parse functions) if passed eof

    // check if each type of constant points to the right type

    return 0;
}

int parse_interfaces(ClassFile *cl, ByteBuf *buf) {
    cl->n_interfaces = bytebuf_readu2(buf);
    cl->interfaces = jmalloc(cl->n_interfaces * sizeof(cl->interfaces[0]));
    for(int i=0;i<cl->n_interfaces;i++) {
        cl->interfaces[i] = bytebuf_readu2(buf);
    }
    return 0;
}

int _parse_attributes_helper(ByteBuf *buf, u2 *n, attribute_info **a) {
    *n = bytebuf_readu2(buf);
    *a = jmalloc(*n * sizeof(*a[0]));
    for(int i=0;i<*n;i++) {
        (*a)[i].attribute_name_index = bytebuf_readu2(buf);
        (*a)[i].attribute_length = bytebuf_readu4(buf);
        (*a)[i].data = jmalloc((*a)[i].attribute_length);
        if(bytebuf_readbytes(buf, (*a)[i].data, (*a)[i].attribute_length) == -1) {
            return -1;
        }
    }
    return 0;
}

int parse_fields(ClassFile *cl, ByteBuf *buf) {
    cl->n_fields = bytebuf_readu2(buf);
    cl->fields = jmalloc(cl->n_fields * sizeof(cl->fields[0]));
    for(int i=0;i<cl->n_fields;i++) {
        cl->fields[i].access_flags = bytebuf_readu2(buf);
        cl->fields[i].name_index = bytebuf_readu2(buf);
        cl->fields[i].descriptor_index = bytebuf_readu2(buf);
        if(_parse_attributes_helper(
                buf, &cl->fields[i].attributes_count, &cl->fields->attributes) == -1) {
            jvm_printf("Error parsing field attributes\n");
            return -1;
        }
    }
    return 0;
}

int parse_code_attribute(Code_attribute *code, ByteBuf *buf) {
    code->max_stack = bytebuf_readu2(buf);
    code->max_locals = bytebuf_readu2(buf);
    code->code_length = bytebuf_readu4(buf);
    code->code = jmalloc(code->code_length);
    if(bytebuf_readbytes(buf, code->code, code->code_length) == -1) {
        return -1;
    }
    code->exception_table_length = bytebuf_readu2(buf);
    code->exception_table = jmalloc(code->exception_table_length * sizeof(code->exception_table[0]));
    for(int i=0;i<code->exception_table_length;i++) {
        code->exception_table->start_pc = bytebuf_readu2(buf);
        code->exception_table->end_pc = bytebuf_readu2(buf);
        code->exception_table->handler_pc = bytebuf_readu2(buf);
        code->exception_table->catch_type = bytebuf_readu2(buf);

    }
    if(_parse_attributes_helper(buf, &code->attributes_count, &code->attributes) == -1) {
        jvm_printf("Failed to parse Code attributes\n");
        return -1;
    }

    return 0;
}

int parse_methods(ClassFile *cl, ByteBuf *buf) {
    cl->n_methods = bytebuf_readu2(buf);
    cl->methods = jmalloc(cl->n_methods * sizeof(cl->methods[0]));
    for(int i=0;i<cl->n_methods;i++) {
        cl->methods[i].access_flags = bytebuf_readu2(buf);
        cl->methods[i].name_index = bytebuf_readu2(buf);
        cl->methods[i].descriptor_index = bytebuf_readu2(buf);
        if(_parse_attributes_helper(
                buf, &cl->methods[i].attributes_count, &cl->methods[i].attributes) == -1) {
            jvm_printf("Error parsing method attributes\n");
            return -1;
        }

        // get named attributes
        // process attributes
        for(int j=0;j<cl->methods[i].attributes_count;j++) {
            attribute_info a = cl->methods[i].attributes[j];
            String *name = cf_constants_get_string(cl, a.attribute_name_index);
            if(str_compare_raw(name, "Code") == 0) {
                ByteBuf b;
                bytebuf_create(&b, a.data, a.attribute_length);
                if(parse_code_attribute(&cl->methods[i].code, &b) == -1) {
                    return -1;
                }
            }
        }

    }
    return 0;
}

int parse_attributes(ClassFile *cl, ByteBuf *buf) {
    if(_parse_attributes_helper(
            buf, &cl->n_attributes, &cl->attributes) == -1) {
        jvm_printf("Error parsing attributes\n");
        return -1;
    }
    return 0;
}

int read_classfile_from_bytes(ClassFile *cl, ByteBuf *buf) {
    // TODO Should verify data after parsing. jmalloc failures should be checked (or panic for now)
    u4 magic = bytebuf_readu4(buf);
    if(magic != 0xcafebabe) return -1;
    cl->minor_version = bytebuf_readu2(buf);
    cl->major_version = bytebuf_readu2(buf);
    if(cl->major_version > 55) {
    jvm_printf("Unsupported major version %d\n", cl->major_version);
    return -1;
    }
    if(parse_constant_pool(cl, buf) == -1) {
    jvm_printf("Error parsing constant pool\n");
    return -1;
    }
    cl->access_flags = bytebuf_readu2(buf);
    cl->this_class = bytebuf_readu2(buf);
    cl->super_class = bytebuf_readu2(buf);
    if(parse_interfaces(cl, buf) == -1) {
    jvm_printf("Error parsing interfaces\n");
    return -1;
    }
    if(parse_fields(cl, buf) == -1) {
    jvm_printf("Error parsing fields_table\n");
    return -1;
    }
    if(parse_methods(cl, buf) == -1) {
    jvm_printf("Error parsing methods\n");
    return -1;
    }
    if(parse_attributes(cl, buf) == -1) {
    jvm_printf("Error parsing attributes\n");
    return -1;
    }
    return 0;
}

int read_class_from_path(JClass *cls, char *path) {
    ByteBuf buf;
    if(read_file(path, &buf) == -1) {
        jvm_printf("Could not open %s, check error\n", path);
        return -1;
    }
    if(read_class_from_bytes(cls, &buf) == -1) {
        jfree(buf.data);
        jvm_printf("Could not parse class at %s\n", path);
        return -1;
    }
    return 0;
}

String *cl_constants_get_string(JClass *class, u2 index) {
    assert(class->constants[index].tag == CONSTANT_Utf8);
    return VAL_GET_PTR(class->constants[index].value);
}

String *cf_constants_get_string(ClassFile *class, u2 index) {
    assert(class->constants[index].tag == CONSTANT_Utf8);
    return VAL_GET_PTR(class->constants[index].value);
}

cp_tags cf_constants_get_tag(ClassFile *class, u2 index) {
    return class->constants[index].tag;
}

int process_methods(JClass *cl, ClassFile *cf) {
    ht_init(&cl->methods_table);
    cl->methods = jmalloc(sizeof(cl->methods[0]) * cf->n_methods);
    cl->n_methods = cf->n_methods;
    for(int i=0;i<cl->n_methods;i++) {
        cl->methods[i].class = cl;
        cl->methods[i].access_flags = cf->methods[i].access_flags;
        cl->methods[i].name = cf_constants_get_string(cf, cf->methods[i].name_index);

        // method descriptor
        ByteBuf buf2;
        String *descriptor_str = cf_constants_get_string(cf, cf->methods[i].descriptor_index);
        bytebuf_create(&buf2, descriptor_str->data, descriptor_str->size);
        if(parse_method_descriptor(&cl->methods[i].descriptor, &buf2) == -1) {
            // cleanup
            jvm_printf("Error parsing method descriptor\n");
            return -1;
        }

        // process code
        cl->methods[i].code.code = cf->methods[i].code.code;
        cl->methods[i].code.code_length = cf->methods[i].code.code_length;
        cl->methods[i].code.max_locals = cf->methods[i].code.max_locals;
        cl->methods[i].code.max_stack = cf->methods[i].code.max_stack;
        cl->methods[i].code.exception_table_length = cf->methods[i].code.exception_table_length;
        cl->methods[i].code.exception_table = cf->methods[i].code.exception_table;

        ht_put(&cl->methods_table, cl->methods[i].name, &cl->methods[i]);
    }
    return 0;
}

static int process_fields(JClass *cl, ClassFile *cf) {
    cl->n_fields = cf->n_fields;
    cl->fields = jmalloc(sizeof(cl->fields[0]) * cl->n_fields);
    for(int i=0;i<cl->n_fields;i++) {
        cl->fields[i].name = cf_constants_get_string(cf, cf->fields[i].name_index);
        ByteBuf buf;
        String *descriptor_str = cf_constants_get_string(cf, cf->fields[i].descriptor_index);
        bytebuf_create(&buf, descriptor_str->data, descriptor_str->size);
        if(parse_field_type(&cl->fields[i].type, &buf) == -1) {
            return -1;
        }
        cl->fields[i].access_flags = cf->fields[i].access_flags;
    }
    return 0;
}


int process_classfile(JClass *cl, ClassFile *cf) {
    cl->next = NULL;
    cl->name = NULL;
    cl->cf = cf;
    cl->status = CLASS_LOADING;

    cl->minor_version = cf->minor_version;
    cl->major_version = cf->major_version;
    cl->access_flags = cf->access_flags;

    cl->this_class = cf->this_class;
    cl->super_class = cf->super_class;

    cl->n_constants = cf->n_constants;
    cl->constants = cf->constants;

    if(process_fields(cl, cf) != 0) {
        return -1;
    }

    if(process_methods(cl, cf) != 0) {
        return -1;
    }

    // should process fields_table here
    return 0;

}

int read_class_from_bytes(JClass *cl, ByteBuf *buf) {
    ClassFile cf;
    if(read_classfile_from_bytes(&cf, buf) == -1) {
        jvm_printf("Failed to read classfile\n");
        return -1;
    }
    if(process_classfile(cl, &cf) == -1) {
        jvm_printf("Failed to process classfile\n");
    }
    return 0;
}