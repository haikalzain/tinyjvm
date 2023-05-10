//
// Created by Haikal Zain on 9/5/2023.
//

#ifndef TINYJVM_UTIL_H
#define TINYJVM_UTIL_H

#include "types.h"
#include <stddef.h>

int jvm_printf(const char *format, ...);

typedef struct ByteBuf {
    uint8_t *data;
    size_t size;
    size_t off;
} ByteBuf;

void bytebuf_create(ByteBuf *buf, uint8_t *data, size_t size);

// These are unsafe
u1 bytebuf_read(ByteBuf *buf);
u2 bytebuf_readu2(ByteBuf *buf);
u4 bytebuf_readu4(ByteBuf *buf);
u8 bytebuf_readu8(ByteBuf *buf);

int read_file(const char *filename, ByteBuf *b);

#endif //TINYJVM_UTIL_H
