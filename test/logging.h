#pragma once
#ifdef TEST_ENV

#include <cstdarg>

#ifdef __GNUC__
#define ESPHOME_LOG_COLOR_BLACK     "30"
#define ESPHOME_LOG_COLOR_RED       "31" // ERROR
#define ESPHOME_LOG_COLOR_GREEN     "32" // INFO
#define ESPHOME_LOG_COLOR_YELLOW    "33" // WARNING
#define ESPHOME_LOG_COLOR_MAGENTA   "35" // CONFIG
#define ESPHOME_LOG_COLOR_CYAN      "36" // DEBUG
#define ESPHOME_LOG_COLOR_GRAY      "37" // VERBOSE
#define ESPHOME_LOG_COLOR_WHITE     "38" // DEFAULT

#define ESPHOME_LOG_COLOR(COLOR) 
#define ESPHOME_LOG_BOLD(COLOR) printf("\033[1;" COLOR "m");
#define ESPHOME_LOG_RESET_COLOR printf("\033[0m");

inline void printf_with_tag(
    const char *color, const char *tag, const char *format, ...) {
  printf("\033[0;" color "m");
  printf(color);
  printf("m[");
  printf(tag);
  printf("] ");
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n\033[0m");
}
#endif

#ifdef _MSC_VER
#include <windows.h>
#include <iostream>

#define ESPHOME_LOG_COLOR_BLACK     0
#define ESPHOME_LOG_COLOR_RED       4  // ERROR
#define ESPHOME_LOG_COLOR_GREEN     2  // INFO
#define ESPHOME_LOG_COLOR_YELLOW    14 // WARNING
#define ESPHOME_LOG_COLOR_MAGENTA   13 // CONFIG
#define ESPHOME_LOG_COLOR_CYAN      11 // DEBUG
#define ESPHOME_LOG_COLOR_GRAY      8  // VERBOSE
#define ESPHOME_LOG_COLOR_WHITE     7  // DEFAULT

inline void printf_with_tag(const uint8_t color, const char *tag,
                            const char *format, ...) {
  static HANDLE handle;
  if (!handle) {
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
  }
  va_list args;
  va_start(args, format);
  char buf[1024];
  vsnprintf(buf, sizeof(buf) / sizeof(char), format, args);
  va_end(args);
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
  std::cout << "[" << tag << "] " << buf << std::endl;
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ESPHOME_LOG_COLOR_WHITE);
}
#endif

#define ESP_LOGE(tag, format, ...)                                             \
  printf_with_tag(ESPHOME_LOG_COLOR_RED, tag, format, __VA_ARGS__);
#define ESP_LOGW(tag, format, ...)                                             \
  printf_with_tag(ESPHOME_LOG_COLOR_YELLOW, tag, format, __VA_ARGS__);
#define ESP_LOGI(tag, format, ...)                                             \
  printf_with_tag(ESPHOME_LOG_COLOR_GREEN, tag, format, __VA_ARGS__);
#define ESP_LOGD(tag, format, ...)                                             \
  printf_with_tag(ESPHOME_LOG_COLOR_CYAN, tag, format, __VA_ARGS__);
#define ESP_LOGCONFIG(tag, format, ...)                                        \
  printf_with_tag(ESPHOME_LOG_COLOR_MAGENTA, tag, format, __VA_ARGS__);
#define ESP_LOGV(tag, format, ...)                                             \
  printf_with_tag(ESPHOME_LOG_COLOR_GRAY, tag, format, __VA_ARGS__);
#define ESP_LOGVV(tag, format, ...)                                            \
  printf_with_tag(ESPHOME_LOG_COLOR_GRAY, tag, format, __VA_ARGS__);

#endif // TEST_ENV