#include "tg_format.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

#define HTML_ESCAPE_COUNT 6
static const char *HTML_ESCAPE_CHARS[HTML_ESCAPE_COUNT][2] = {
    {"&", "&amp;"},
    {"<", "&lt;"},
    {">", "&gt;"},
    {"\"", "&quot;"},
    {"'", "&#39;"},
    {"/", "&#x2F;"}
};

#define MD2_ESCAPE_COUNT 5
static const char *MD2_ESCAPE_CHARS[MD2_ESCAPE_COUNT][2] = {
    {"\\", "\\\\"},
    {"[", "\\["},
    {"]", "\\]"},
    {"(", "\\("},
    {")", "\\)"}
};

static void html_escape(const char *src, char *dst, size_t dst_size)
{
    size_t dst_pos = 0;
    for (size_t i = 0; src[i] && dst_pos < dst_size - 1; i++) {
        bool escaped = false;
        for (int j = 0; j < HTML_ESCAPE_COUNT; j++) {
            size_t escape_len = strlen(HTML_ESCAPE_CHARS[j][0]);
            if (strncmp(src + i, HTML_ESCAPE_CHARS[j][0], escape_len) == 0) {
                size_t escape_out_len = strlen(HTML_ESCAPE_CHARS[j][1]);
                if (dst_pos + escape_out_len < dst_size - 1) {
                    memcpy(dst + dst_pos, HTML_ESCAPE_CHARS[j][1], escape_out_len);
                    dst_pos += escape_out_len;
                }
                escaped = true;
                break;
            }
        }
        if (!escaped) {
            dst[dst_pos++] = src[i];
        }
    }
    dst[dst_pos] = '\0';
}

static void md2_escape(const char *src, char *dst, size_t dst_size)
{
    size_t dst_pos = 0;
    for (size_t i = 0; src[i] && dst_pos < dst_size - 1; i++) {
        bool escaped = false;
        for (int j = 0; j < MD2_ESCAPE_COUNT; j++) {
            if (src[i] == HTML_ESCAPE_CHARS[j][0][0]) {
                if (dst_pos + 2 < dst_size - 1) {
                    dst[dst_pos++] = '\\';
                    dst[dst_pos++] = src[i];
                }
                escaped = true;
                break;
            }
        }
        if (!escaped) {
            dst[dst_pos++] = src[i];
        }
    }
    dst[dst_pos] = '\0';
}

static char *format_alloc(const char *src, void (*escape_fn)(const char *, char *, size_t))
{
    if (!src) return NULL;

    size_t max_escaped = strlen(src) * 6;
    char *escaped = malloc(max_escaped + 1);
    if (!escaped) return NULL;

    escape_fn(src, escaped, max_escaped + 1);

    size_t result_size = strlen(escaped) + 64;
    char *result = malloc(result_size);
    if (!result) {
        free(escaped);
        return NULL;
    }

    snprintf(result, result_size, "<pre>%s</pre>", escaped);
    free(escaped);
    return result;
}

char *tg_format_html(const char *text)
{
    return format_alloc(text, html_escape);
}

char *tg_format_markdownv2(const char *text)
{
    return format_alloc(text, md2_escape);
}

void tg_format_free(char *str)
{
    free(str);
}