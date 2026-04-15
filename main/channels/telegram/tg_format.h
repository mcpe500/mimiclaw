#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

char *tg_format_html(const char *text);

char *tg_format_markdownv2(const char *text);

void tg_format_free(char *str);

#ifdef __cplusplus
}
#endif