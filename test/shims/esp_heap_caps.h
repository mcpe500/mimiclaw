#pragma once
#include <stdint.h>
#define MALLOC_CAP_INTERNAL  0x0001
#define MALLOC_CAP_SPIRAM    0x0002

static inline uint32_t heap_caps_get_free_size(uint32_t caps) { (void)caps; return 65536; }
