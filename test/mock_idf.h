#pragma once
#ifdef TEST_ENV

#include <cstdint>
#include <memory>

#define MALLOC_CAP_SPIRAM 0

#define heap_caps_malloc(size, args) malloc(size)
#define heap_caps_free(block) free(block)
#define heap_caps_realloc(block, size) realloc(block, size)
#define heap_caps_get_total_size(caps) 0
#define heap_caps_get_free_size(caps) 0

#endif // TEST_ENV