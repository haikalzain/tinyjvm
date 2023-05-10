//
// Created by Haikal Zain on 9/5/2023.
//
#include "jvm.h"
#include "util.h"

int parse_constant_pool(JClass *cl, ByteBuf *buf) {

}

int read_class_from_bytes(JClass *cl, ByteBuf *buf) {
    u4 magic = bytebuf_readu4(buf);
    if(magic != 47806) return -1;
    u2 minor_version = bytebuf_readu2(buf);
    u2 major_version = bytebuf_readu2(buf);
    if(major_version > 55) {
        jvm_printf("Unsupported major version %d\n", cl->cf.major_version);
        return -1;
    }
    cl->cf.constant_pool_count = bytebuf_readu2(buf);
    return 0;
}