#pragma once

#include "tool_registry.h"
#include "memory/memory_store.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t tool_memory_init(void);

esp_err_t tool_memory_read(const char *input_json, char *output, size_t output_size);

esp_err_t tool_memory_write(const char *input_json, char *output, size_t output_size);

esp_err_t tool_memory_append(const char *input_json, char *output, size_t output_size);

esp_err_t tool_memory_recall(const char *input_json, char *output, size_t output_size);

#ifdef __cplusplus
}
#endif