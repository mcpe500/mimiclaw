#pragma once
#include <stdint.h>
#include <stdlib.h>
#define MALLOC_CAP_INTERNAL  0x0001
#define MALLOC_CAP_SPIRAM    0x0002

static inline uint32_t heap_caps_get_free_size(uint32_t caps) { (void)caps; return 65536; }
static inline uint32_t esp_get_free_heap_size(void) { return 65536; }
static inline uint32_t esp_get_minimum_free_heap_size(void) { return 32768; }
static inline void *heap_caps_calloc(size_t nmemb, size_t size, uint32_t caps) { (void)caps; return calloc(nmemb, size); }
