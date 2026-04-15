#pragma once

static inline void esp_restart(void) { abort(); }
static inline size_t esp_get_free_heap_size(void) { return 1024 * 1024; }
static inline size_t esp_get_minimum_free_heap_size(void) { return 512 * 1024; }
