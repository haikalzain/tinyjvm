//
// Created by Haikal Zain on 9/5/2023.
//
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


int jvm_printf(const char *format, ...) {
    va_list vp;
    va_start(vp, format);

    int result = vfprintf(stderr, format, vp);
    va_end(vp);
    return result;
}

void *jmalloc(size_t n) {
    return malloc(n);
}

int read_file(const char *filename, ByteBuf *b) {
    FILE *f = fopen(filename, "rb");
    if(!f) return -1;
    fseek(f, 0, SEEK_END);
    size_t s = ftell(f);
    if(s == -1) return -1;

    uint8_t *c = (uint8_t *)jmalloc(s);
    if(c == NULL) return -1;

    rewind(f);
    size_t n = fread(c, s, 1, f);
    fclose(f);
    if(n != 1) { // I think this is how it works
        free(c);
        return -1;
    }
    bytebuf_create(b, c, s);
    return 0;
}

void bytebuf_create(ByteBuf *buf, uint8_t *data, size_t size) {
    buf->data = data;
    buf->size = size;
    buf->off = 0;
}

u1 bytebuf_read(ByteBuf *buf) {
    if(buf->off == buf->size) return 0;
    return buf->data[buf->off++];
}

u2 bytebuf_readu2(ByteBuf *buf) {
    if(buf->off == buf->size) return 0;
    u2 r = (buf->data[buf->off] << 8) | buf->data[buf->off+1];
    buf->off += 2;
    return r;
}

u4 bytebuf_readu4(ByteBuf *buf) {
    if(buf->off == buf->size) return 0;
    u4 r = (buf->data[buf->off] << 24) |
           (buf->data[buf->off + 1] << 16) |
           (buf->data[buf->off+2] << 8) |
           buf->data[buf->off+3];
    buf->off += 4;
    return r;
}

u8 bytebuf_readu8(ByteBuf *buf) {
    if(buf->off == buf->size) return 0;
    u8 r = ((u8)buf->data[buf->off] << 56) |
           ((u8)buf->data[buf->off + 1] << 48) |
           ((u8)buf->data[buf->off+2] << 40) |
            ((u8)buf->data[buf->off+3] << 32) |
            ((u8)buf->data[buf->off+4] << 24) |
            ((u8)buf->data[buf->off+5] << 16) |
            ((u8)buf->data[buf->off+6] << 8) |
           buf->data[buf->off+7];
    buf->off += 8;
    return r;
}

int bytebuf_readbytes(ByteBuf *buf, u1 *str, size_t size) {
    if(buf->off + size > buf->size) {
        return -1;
    }
    memcpy(str, &buf->data[buf->off], size);
    buf->off += size;
    return 0;
}