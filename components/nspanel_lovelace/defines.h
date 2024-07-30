#pragma once

#define M_PI 3.14159265358979323846 // pi

#ifdef __GNUC__
#define PACK(__Declaration__) __Declaration__ __attribute__((__packed__))
#endif
#ifdef _MSC_VER
#define PACK(__Declaration__)                                 \
  __pragma(pack(push, 1)) __Declaration__ __pragma(pack(pop))
#endif

#if defined(__cpp_char8_t)
typedef char8_t icon_char_t;
#define CHAR8_CAST(icon) reinterpret_cast<const char *>(icon)
#else
#define CHAR8_CAST(icon) icon
typedef char icon_char_t;
#endif