#pragma once
#include "esp_err.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[64];
    char version[16];
    char description[256];
    char author[64];
    int tool_count;
    char tools_required[8][64];
    int dep_count;
    char dependencies[8][64];
} skill_manifest_t;

esp_err_t skill_manifest_parse(const char *json, skill_manifest_t *manifest);
esp_err_t skill_manifest_validate(const skill_manifest_t *manifest);

#ifdef __cplusplus
}
#endif
