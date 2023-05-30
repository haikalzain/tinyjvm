//
// Created by Haikal Zain on 9/5/2023.
//
#include <assert.h>
#include <sys/fcntl.h>
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
        // skip array for now
        param_start:
        c = bytebuf_read(buf);
        switch(c) {
            case 'B':
                d->field_types[i].type = TYPE_BYTE;
                break;
            case 'C':
                d->field_types[i].type = TYPE_CHAR;
                break;
            case 'D':
                d->field_types[i].type = TYPE_DOUBLE;
                break;
            case 'F':
                d->field_types[i].type = TYPE_FLOAT;
                break;
            case 'I':
                d->field_types[i].type = TYPE_INT;
                break;
            case 'J':
                d->field_types[i].type = TYPE_LONG;
                break;
            case 'S':
                d->field_types[i].type = TYPE_SHORT;
                break;
            case 'Z':
                d->field_types[i].type = TYPE_BOOL;
                break;
            case 'L':
                l = 0;
                while(bytebuf_read(buf) != ';') l++;
                data = jmalloc(l);
                buf->off -= l + 1;
                j = 0;
                while((c = bytebuf_read(buf)) != ';') {
                    data[j++] = c;
                }
                d->return_type.type = TYPE_INSTANCE;
                str_create(&d->return_type.class_name, data, l);
                break;
            case '[':
                goto param_start;
                return -1;
                break;
            default:
                jfree(d->field_types);
                return -1;

        }
    }
    bytebuf_read(buf);

    // skip array for now
    return_start:
    // return type
    switch(bytebuf_read(buf)) {
        case 'B':
            d->return_type.type = TYPE_BYTE;
            break;
        case 'C':
            d->return_type.type = TYPE_CHAR;
            break;
        case 'D':
            d->return_type.type = TYPE_DOUBLE;
            break;
        case 'F':
            d->return_type.type = TYPE_FLOAT;
            break;
        case 'I':
            d->return_type.type = TYPE_INT;
            break;
        case 'J':
            d->return_type.type = TYPE_LONG;
            break;
        case 'S':
            d->return_type.type = TYPE_SHORT;
            break;
        case 'Z':
            d->return_type.type = TYPE_BOOL;
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
            d->return_type.type = TYPE_INSTANCE;
            str_create(&d->return_type.class_name, data, l);
            break;
        case '[':
            goto return_start;
            return -1;
            break;
        case 'V':
            d->return_type.type = TYPE_VOID;
            break;
        default:
            jfree(d->field_types);
            return -1;

    }
    assert(buf->off == buf->size);
    return 0;
}

int parse_constant_pool(JClass *cl, ByteBuf *buf) {
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

int parse_interfaces(JClass *cl, ByteBuf *buf) {
    cl->n_interfaces = bytebuf_readu2(buf);
    cl->interfaces = jmalloc(cl->n_interfaces * sizeof(cl->interfaces[0]));
    for(int i=0;i<cl->n_interfaces;i++) {
        cl->interfaces[i] = bytebuf_readu2(buf);
    }
    return 0;
}

int _parse_attributes_helper(JClass *cl, ByteBuf *buf, u2 *n, attribute_info **a) {
    *n = bytebuf_readu2(buf);
    *a = jmalloc(*n * sizeof(*a[0]));
    for(int i=0;i<*n;i++) {
        (*a)[i].attribute_name_index = bytebuf_readu2(buf);
        if(cl_constants_get_tag(cl, (*a)[i].attribute_name_index) != CONSTANT_Utf8) {
            jvm_printf("Expected constant tag to be UTF8\n");
            return -1;
        }

        (*a)[i].attribute_length = bytebuf_readu4(buf);
        (*a)[i].data = jmalloc((*a)[i].attribute_length);
        if(bytebuf_readbytes(buf, (*a)[i].data, (*a)[i].attribute_length) == -1) {
            return -1;
        }
    }
    return 0;
}

int parse_fields(JClass *cl, ByteBuf *buf) {
    cl->n_fields = bytebuf_readu2(buf);
    cl->fields = jmalloc(cl->n_fields * sizeof(cl->fields[0]));
    for(int i=0;i<cl->n_fields;i++) {
        cl->fields[i].access_flags = bytebuf_readu2(buf);
        cl->fields[i].name_index = bytebuf_readu2(buf);
        cl->fields[i].descriptor_index = bytebuf_readu2(buf);
        if(_parse_attributes_helper(
                cl, buf, &cl->fields[i].attributes_count, &cl->fields->attributes) == -1) {
            jvm_printf("Error parsing field attributes\n");
            return -1;
        }
    }
    return 0;
}

int parse_code_attribute(JByteCode *code, ByteBuf *buf) {
    code->max_stack = bytebuf_readu2(buf);
    code->max_locals = bytebuf_readu2(buf);
    code->code_length = bytebuf_readu4(buf);
    code->code = jmalloc(code->code_length);
    if(bytebuf_readbytes(buf, code->code, code->code_length) == -1) {
        return -1;
    }
    /*
     * u2 exception_table_length;
    {   u2 start_pc;
        u2 end_pc;
        u2 handler_pc;
        u2 catch_type;
    } exception_table[exception_table_length];
    u2 attributes_count;
    attribute_info attributes[attributes_count];
     */
    return 0;
}

int parse_methods(JClass *cl, ByteBuf *buf) {
    cl->n_methods = bytebuf_readu2(buf);
    cl->methods = jmalloc(cl->n_methods * sizeof(cl->methods[0]));
    for(int i=0;i<cl->n_methods;i++) {
        cl->methods[i].class = cl;

        cl->methods[i].access_flags = bytebuf_readu2(buf);
        cl->methods[i].name_index = bytebuf_readu2(buf);
        cl->methods[i].name = VAL_GET_STRING(cl->constants[cl->methods[i].name_index].value); // unsafe
        cl->methods[i].descriptor_index = bytebuf_readu2(buf);
        if(_parse_attributes_helper(
                cl, buf, &cl->methods[i].attributes_count, &cl->methods[i].attributes) == -1) {
            jvm_printf("Error parsing method attributes\n");
            return -1;
        }

        // method descriptor
        ByteBuf buf2;
        String *descriptor_str = cl_constants_get_string(cl, cl->methods[i].descriptor_index);
        bytebuf_create(&buf2, descriptor_str->data, descriptor_str->size);
        if(parse_method_descriptor(&cl->methods[i].descriptor, &buf2) == -1) {
            // cleanup
            jvm_printf("Error parsing method descriptor\n");
            return -1;
        }

        // process attributes
        for(int j=0;j<cl->methods[i].attributes_count;j++) {
            attribute_info a = cl->methods[i].attributes[j];
            String *name = cl_constants_get_string(cl, a.attribute_name_index);
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

int parse_attributes(JClass *cl, ByteBuf *buf) {
    if(_parse_attributes_helper(
            cl, buf, &cl->n_attributes, &cl->attributes) == -1) {
        jvm_printf("Error parsing attributes\n");
        return -1;
    }
    return 0;
}

int read_class_from_bytes(JClass *cl, ByteBuf *buf) {
    // TODO Should verify data after parsing. jmalloc failures should be checked (or panic for now)
    cl->next = NULL;
    cl->name = NULL;
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
        jvm_printf("Error parsing fields\n");
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
cp_tags cl_constants_get_tag(JClass *class, u2 index) {
    return class->constants[index].tag;
}