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
    cl->constants = (Constant *)jmalloc(cl->n_constants);
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

    return 0;
}

int read_class_from_bytes(JClass *cl, ByteBuf *buf) {
    u4 magic = bytebuf_readu4(buf);
    if(magic != 0xcafebabe) return -1;
    u2 minor_version = bytebuf_readu2(buf);
    u2 major_version = bytebuf_readu2(buf);
    if(major_version > 55) {
        jvm_printf("Unsupported major version %d\n", cl->cf.major_version);
        return -1;
    }
    parse_constant_pool(cl, buf);
    return 0;
}