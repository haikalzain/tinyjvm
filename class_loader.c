//
// Created by Haikal Zain on 9/5/2023.
//
#include "jvm.h"
#include "util.h"


int parse_constant(Constant *c, ByteBuf *buf) {
    u1 tag = bytebuf_read(buf);
    Value v;
    union {
        int32_t i;
        CONSTANT_double_index_info di;
        CONSTANT_index_info si;
        CONSTANT_kind_index_info ki;
    }u;
    switch(tag) {
        case CONSTANT_Integer:
            v.tag = INT;
            v.as.i = (int32_t) bytebuf_readu4(buf);
            break;
        case CONSTANT_Float:
            v.tag = FLOAT;
            v.as.f = (float) bytebuf_readu4(buf);
            break;
        case CONSTANT_Long:
            v.tag = LONG;
            v.as.l = (int64_t) bytebuf_readu8(buf);
            break;
        case CONSTANT_Double:
            v.tag = DOUBLE;
            v.as.d = (double) bytebuf_readu8(buf);
            break;
        case CONSTANT_Utf8:
            v.tag = STRING;
            u2 size = bytebuf_readu2(buf);
            u1 *data = jmalloc(size);
            bytebuf_readbytes(buf, data, size);
            String *str = jmalloc(sizeof(String));
            str_create(str, data, size);
            v.as.ptr = str;
            break;

        case CONSTANT_MethodHandle:
            v.tag = INT;
            u.ki.kind = bytebuf_read(buf);
            u.ki.index = bytebuf_readu2(buf);
            v.as.i = u.i;
            break;
        case CONSTANT_String:
        case CONSTANT_MethodType:
        case CONSTANT_Class:
        case CONSTANT_Module:
        case CONSTANT_Package:
            v.tag = INT;
            u.si.index = bytebuf_readu2(buf);
            v.as.i = u.i;
            break;

        case CONSTANT_Fieldref:
        case CONSTANT_Methodref:
        case CONSTANT_InterfaceMethodref:
        case CONSTANT_NameAndType:
        case CONSTANT_Dynamic:
        case CONSTANT_InvokeDynamic:
            v.tag = INT;
            u.di.index1 = bytebuf_readu2(buf);
            u.di.index2 = bytebuf_readu2(buf);
            v.as.i = u.i;
            break;

        default:
            jvm_printf("Unrecognized constant type %d\n", tag);
            return -1;
    }
    c->tag = tag;
    c->value = v;
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
        (*a)->attribute_name_index = bytebuf_readu2(buf);
        if(jclass_constants_get_tag(cl, (*a)->attribute_name_index) != CONSTANT_Utf8) {
            jvm_printf("Expected constant tag to be UTF8\n");
            return -1;
        }

        (*a)->attribute_length = bytebuf_readu4(buf);
        (*a)->data = jmalloc((*a)->attribute_length);
        if(bytebuf_readbytes(buf, (*a)->data, (*a)->attribute_length) == -1) {
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
        cl->methods[i].access_flags = bytebuf_readu2(buf);
        cl->methods[i].name_index = bytebuf_readu2(buf);
        cl->methods[i].descriptor_index = bytebuf_readu2(buf);
        if(_parse_attributes_helper(
                cl, buf, &cl->methods[i].attributes_count, &cl->methods[i].attributes) == -1) {
            jvm_printf("Error parsing method attributes\n");
            return -1;
        }

        // process attributes
        for(int j=0;j<cl->methods[i].attributes_count;j++) {
            attribute_info a = cl->methods[i].attributes[j];
            String *name = jclass_constants_get_string(cl, a.attribute_name_index);
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

String *jclass_constants_get_string(JClass *class, u2 index) {
    return class->constants[index].value.as.ptr;
}
cp_tags jclass_constants_get_tag(JClass *class, u2 index) {
    return class->constants[index].tag;
}