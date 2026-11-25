// Host-build stub for the ESP-IDF header that helpers.h includes.
// Only the two entry points psram_available()/psram_used() reference are needed;
// the helper tests never call them.
#pragma once

#include <stdint.h>
#include <stddef.h>

#define MALLOC_CAP_SPIRAM (1u << 10)

inline size_t heap_caps_get_total_size(uint32_t) { return 0; }
inline size_t heap_caps_get_free_size(uint32_t) { return 0; }
