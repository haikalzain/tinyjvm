//
// Created by Haikal Zain on 9/5/2023.
//

#ifndef TINYJVM_UTIL_H
#define TINYJVM_UTIL_H

#include "types.h"
#include <stddef.h>

int jvm_printf(const char *format, ...);

void *jmalloc(size_t n);
void jfree(void *p);
void *jrealloc(void *p, size_t n);

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

int bytebuf_readbytes(ByteBuf *buf, u1 *bytes, size_t size);

uint32_t fnv_32_hash(const u1 *str, int len);

int read_file(const char *filename, ByteBuf *b);

#endif //TINYJVM_UTIL_H
