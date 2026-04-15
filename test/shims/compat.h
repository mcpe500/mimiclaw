#pragma once
#include <string.h>
#include <stddef.h>

static inline size_t strlcpy(char *dst, const char *src, size_t size)
{
    size_t srclen = strlen(src);
    if (size > 0) {
        size_t copylen = (srclen < size - 1) ? srclen : size - 1;
        memcpy(dst, src, copylen);
        dst[copylen] = '\0';
    }
    return srclen;
}

static inline size_t strlcat(char *dst, const char *src, size_t size)
{
    size_t dstlen = strlen(dst);
    size_t srclen = strlen(src);
    if (dstlen < size - 1) {
        size_t copylen = (srclen < size - dstlen - 1) ? srclen : size - dstlen - 1;
        memcpy(dst + dstlen, src, copylen);
        dst[dstlen + copylen] = '\0';
    }
    return dstlen + srclen;
}
