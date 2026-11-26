#pragma once

#include "defines.h"
#include "esphome/core/defines.h"

#include <array>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <esp_heap_caps.h>
#include <math.h>
#include <stdint.h>
#include <string>
#include <time.h>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

// see: https://stackoverflow.com/a/13890501/2634818
inline bool replace_first(
    std::string &s, std::string const &toReplace,
    std::string const &replaceWith) {
  std::size_t pos = s.find(toReplace);
  if (pos == std::string::npos)
    return false;
  s.replace(pos, toReplace.length(), replaceWith);
  return true;
}

// see: https://stackoverflow.com/a/13890501/2634818
inline void replace_all(
    std::string &s, std::string const &toReplace,
    std::string const &replaceWith) {
  std::string buf;
  std::size_t pos = 0;
  std::size_t prevPos;

  // Reserves rough estimate of final size of string.
  buf.reserve(s.size());

  while (true) {
    prevPos = pos;
    pos = s.find(toReplace, pos);
    if (pos == std::string::npos)
      break;
    buf.append(s, prevPos, pos - prevPos);
    buf += replaceWith;
    pos += toReplace.size();
  }

  buf.append(s, prevPos, s.size() - prevPos);
  s.swap(buf);
}

template<typename StringT>
inline void replace_all(StringT &s, const char oldChar, const char newChar) {
  for (size_t pos = 0; (pos = s.find(oldChar, pos)) != StringT::npos; ++pos) {
    s[pos] = newChar;
  }
}

// Mixed formats such as "%A %a" render both names. Mark the long name before
// translating the short one so the short pass cannot corrupt the translation.
inline void translate_date_name_pair(
    std::string &s, const char *long_name, const char *long_translation,
    const char *short_name, const char *short_translation) {
  const std::string long_marker(1, '\x01');
  const std::string short_marker(1, '\x02');
  replace_first(s, long_name, long_marker);
  replace_first(s, short_name, short_marker);
  replace_first(s, long_marker, long_translation);
  replace_first(s, short_marker, short_translation);
}

inline const char* value_or_empty(const char* s) {
  return s == nullptr ? "" : s; 
}

template<typename StringT>
inline unsigned long value_or_default(const StringT &str, unsigned long default_value) {
  return str.empty() || (str[0] != '-' && str[0] != '+' && !isdigit(str[0]))
    ? default_value : strtoul(str.c_str(), nullptr, 10);
}

template<typename StringT>
inline int value_or_default(const StringT &str, int default_value) {
  return str.empty() || (str[0] != '-' && str[0] != '+' && !isdigit(str[0]))
    ? default_value : static_cast<int>(strtol(str.c_str(), nullptr, 10));
}

template<typename StringT>
inline unsigned int value_or_default(const StringT &str, unsigned int default_value) {
  return static_cast<unsigned int>(value_or_default(str, static_cast<unsigned long>(default_value)));
}

template<typename StringT>
inline double value_or_default(const StringT &str, double default_value) {
  return str.empty() || (str[0] != '-' && str[0] != '+' && !isdigit(str[0]))
    ? default_value : strtod(str.c_str(), nullptr);
}

inline uint16_t rgb_dec565(uint8_t red, uint8_t green, uint8_t blue) {
  // if type(rgb_color) is str:
  //     rgb_color = apis.ha_api.render_template(rgb_color)
  return ((red >> 3) << 11) | ((green >> 2) << 5) | ((blue >> 3));
}

// note: h,s,v should all be between 0 and 1
inline std::vector<uint8_t> hsv2rgb(double h, double s, double v) {
  if (s <= 0.0) {
    auto val = static_cast<uint8_t>(round(v * 255));
    return {val,val,val};
  }

  // h = h >= 360.0 ? 0.0 : (h / 60.0);
  // auto i = static_cast<uint32_t>(h);
  auto i = static_cast<uint32_t>(h * 6.0);
  double 
      // f = h - i,
      f = (h * 6.0) - i,
      p = v * (1.0 - s),
      q = v * (1.0 - (s * f)),
      t = v * (1.0 - (s * (1.0 - f)));

  double r = 0, g = 0, b = 0;
  switch(i)
  {
    case 0: r = v, g = t, b = p; break;
    case 1: r = q, g = v, b = p; break;
    case 2: r = p, g = v, b = t; break;
    case 3: r = p, g = q, b = v; break;
    case 4: r = t, g = p, b = v; break;
    default: r = v, g = p, b = q; break;
  }

  return {
      static_cast<uint8_t>(round(r * 255)), 
      static_cast<uint8_t>(round(g * 255)), 
      static_cast<uint8_t>(round(b * 255))};
}

// note: x,y should be between 0 and 1, wh (width/height) default is 160
inline std::vector<uint8_t> xy_to_rgb(double x, double y, float wh) {
  double r = wh / 2;
  x = round((x - r) / r * 100) / 100;
  y = round((r - y) / r * 100) / 100;

  r = sqrt((x * x) + (y * y));
  return hsv2rgb(
      std::fmod((atan2(y, x) * (180 / M_PI)) + 360, 360) / 360,
      (r > 1 ? 0 : r),
      1);
}

inline double scale_value(double val, std::array<double, 2> scale_from, std::array<double, 2> scale_to) {
  return
      ((val - scale_from[0]) / (scale_from[1] - scale_from[0])) * 
      (scale_to[1] - scale_to[0]) + scale_to[0];
}

inline bool contains_value(const std::vector<std::string> &array, const char *value) {
  for (auto& item : array) {
    if (item == value) return true;
  }
  return false;
}

template<typename StringT>
inline bool contains_value(const StringT &str, const char *value) {
  return str.find(value, 0) != StringT::npos;
}

inline bool char_printable(const char value) {
  return value >= 0x20 && value <= 0x7e;
}

inline static constexpr bool str_equal(const char *a, const char *b) {
  return a == b || (a != nullptr && b != nullptr && std::strcmp(a, b) == 0);
}

template<typename StringT = std::string>
inline void split_str(char delimiter, const StringT &str,
    std::vector<std::string> &array, uint16_t max_items = UINT16_MAX) {
  size_t pos_start = 0, pos_end = 0;
  uint16_t item_count = 0;
  while ((pos_end = str.find(delimiter, pos_start)) != StringT::npos) {
    if (item_count == max_items) return;
    if (pos_end > pos_start)
      array.emplace_back(str.data() + pos_start, pos_end - pos_start);
    pos_start = pos_end + 1;
    item_count++;
  }
  if (pos_start < str.size())
    array.emplace_back(str.data() + pos_start, str.size() - pos_start);
}

// Fixed-format fields cannot shift left when a field is empty.
template<size_t N, typename StringT = std::string>
inline bool split_fixed(char delimiter, const StringT &str, std::array<std::string, N> &fields) {
  size_t start = 0;
  for (size_t i = 0; i < N; ++i) {
    const size_t end = str.find(delimiter, start);
    if ((end == StringT::npos) != (i + 1 == N)) return false;
    // Last field (no trailing delimiter) has end == npos; clamp its length to
    // the remainder instead of letting npos - start wrap into an out-of-range read.
    const size_t count = (end == StringT::npos) ? (str.size() - start) : (end - start);
    fields[i].assign(str.data() + start, count);
    if (fields[i].empty()) return false;
    if (end != StringT::npos) start = end + 1;
  }
  return true;
}

inline size_t find_nth_of(char delimiter, uint16_t count, const std::string &str) {
  size_t pos = std::string::npos;
  if (count == 0) return pos;

  uint16_t idx = 0;
  do {
    pos = str.find(delimiter, pos + 1);
    idx++;
  } while (pos != std::string::npos && idx < count);

  return pos;
}

// Takes the string representation of a Python array (enums, strings etc) and extracts the 
// values to a new string separated by delimiter.
// todo: remove this when esphome starts sending properly formatted array strings
inline std::string convert_python_arr_str(const std::string &str, const char delimiter = ',') {
  if (str.empty()) return str;
  if (str[0] != '[' || str.length() < 3) return str;
  size_t pos_start = 0, pos_end = pos_start;
  bool escape = false;
  std::string tmp;
  const char quote = str[1] == '"' ? '"' : '\'';
  do {
    if (!escape) {
      pos_start = str.find(quote, pos_end + 1);
    }
    if (pos_start == std::string::npos) {
      if (!tmp.empty() && tmp.back() == delimiter) tmp.pop_back();
      break;
    }
    pos_end = str.find(quote, (pos_end > pos_start ? pos_end : pos_start) + 1);
    if (pos_end == std::string::npos) break;
    // ignore empty entries
    if (pos_end - pos_start - 1 == 0) continue;
    // skip escape characters
    if (str.at(pos_end-1) == '\\') {
      escape = true;
      pos_end++;
      continue;
    }
    if (escape) escape = false;
    tmp.append(str.substr(pos_start + 1, pos_end - pos_start - 1)).append(1, delimiter);
  } while (true);
  return tmp.empty() ? str : tmp;
}

inline std::string to_string(const std::vector<std::string> &array, 
    char delimiter = ',', const char prepend_char = '\0', 
    const char append_char = '\0') {
  std::string output;
  if (!char_printable(delimiter)) return output;

  if (char_printable(prepend_char)) {
    output.append(1, prepend_char);
  }
  for (size_t i = 0; i < array.size(); i++) {
    output.append(array.at(i));
    if (i < array.size() - 1) {
      output.append(1, delimiter);
    }
  }
  if (char_printable(append_char)) {
    output.append(1, append_char);
  }
  return output;
}

inline std::string to_string(const std::vector<uint8_t> &array, 
    char delimiter = ',', const char prepend_char = '\0', 
    const char append_char = '\0') {
  std::vector<std::string> str_array;
  str_array.reserve(array.size());
  for (auto &&value : array) {
    str_array.push_back(std::to_string(value));
  }
  return to_string(str_array, delimiter, prepend_char, append_char);
}

inline bool psram_available() {
  return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) > 0 && 
      heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0;
}

inline size_t psram_used() {
  return heap_caps_get_total_size(MALLOC_CAP_SPIRAM) -
      heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

// Non-throwing alternatives to std::stoi/stof/stod. C++ exceptions are disabled
// on ESP-IDF so the std:: versions abort on empty or non-numeric input.
// Returns false when the string contains no convertible number, overflows,
// or contains trailing junk, so callers can distinguish "parsed zero" from
// "parse failed" instead of acting on a bogus 0.
template<typename StringT = std::string>
[[nodiscard]] inline bool str_to_int(const StringT &s, int &out, int base = 10) {
  // Normalize to std::string so this accepts std::string, psram_string and
  // char[N] literals uniformly, and keeps .size()-based embedded-NUL rejection.
  std::string tmp(s);
  char *end = nullptr;
  errno = 0;
  long val = strtol(tmp.c_str(), &end, base);
  if (errno == ERANGE || end == tmp.c_str() || end != tmp.c_str() + tmp.size()) return false;
  if (val < INT_MIN || val > INT_MAX) return false;
  out = static_cast<int>(val);
  return true;
}

template<typename StringT = std::string>
[[nodiscard]] inline bool str_to_float(const StringT &s, float &out) {
  std::string tmp(s);
  char *end = nullptr;
  errno = 0;
  const float val = strtof(tmp.c_str(), &end);
  if (errno == ERANGE || end == tmp.c_str() || end != tmp.c_str() + tmp.size()) return false;
  if (!std::isfinite(val)) return false;
  out = val;
  return true;
}

template<typename StringT = std::string>
[[nodiscard]] inline bool str_to_double(const StringT &s, double &out) {
  std::string tmp(s);
  char *end = nullptr;
  errno = 0;
  const double val = strtod(tmp.c_str(), &end);
  if (errno == ERANGE || end == tmp.c_str() || end != tmp.c_str() + tmp.size()) return false;
  if (!std::isfinite(val)) return false;
  out = val;
  return true;
}

template<typename StringT = std::string>
inline std::array<double, 2> mired_range(const StringT &min_value, const StringT &max_value) {
  int min = 153, max = 500;
  int parsed = 0;
  if (str_to_int(min_value, parsed) && parsed > 0 && parsed <= UINT16_MAX) min = parsed;
  if (str_to_int(max_value, parsed) && parsed > 0 && parsed <= UINT16_MAX) max = parsed;
  if (min >= max) return {153, 500};
  return {static_cast<double>(min), static_cast<double>(max)};
}

// Return the length of one well-formed UTF-8 scalar, or zero on invalid input.
inline size_t utf8_scalar_size(const std::string &s, size_t pos) {
  if (pos >= s.size()) return 0;
  const auto lead = static_cast<unsigned char>(s[pos]);
  if (lead < 0x80) return 1;
  if (lead < 0xc2 || lead > 0xf4) return 0;
  const size_t len = lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
  if (s.size() - pos < len) return 0;
  const auto second = static_cast<unsigned char>(s[pos + 1]);
  if ((lead == 0xe0 && second < 0xa0) || (lead == 0xed && second >= 0xa0) ||
      (lead == 0xf0 && second < 0x90) || (lead == 0xf4 && second >= 0x90)) return 0;
  for (size_t i = 1; i < len; ++i) {
    if ((static_cast<unsigned char>(s[pos + i]) & 0xc0) != 0x80) return 0;
  }
  return len;
}

template<typename StringT = std::string>
inline bool str_to_percentage(const StringT &s, int &out) {
  return str_to_int(s, out) && out >= 0 && out <= 100;
}

template<typename StringT = std::string>
inline bool parse_timer_duration(const StringT &value, uint16_t &minutes, uint16_t &seconds,
                                 bool tft_editor = false) {
  std::string tmp(value);
  std::array<std::string, 3> parts;
  if (!split_fixed(':', tmp, parts)) return false;
  int hours, mins, secs;
  for (const auto &part : parts) {
    if (part.find_first_not_of("0123456789") != std::string::npos) return false;
  }
  if (!str_to_int(parts[0], hours) || !str_to_int(parts[1], mins) || !str_to_int(parts[2], secs) ||
      secs >= 60 || mins > UINT16_MAX || hours > (UINT16_MAX - mins) / 60) return false;
  // v4.3.3 sends total minutes with the literal "00:" prefix.
  if (mins >= 60 && !(tft_editor && parts[0] == "00")) return false;
  minutes = hours * 60 + mins;
  seconds = secs;
  return true;
}

#ifdef USE_PSRAM
template<typename T>
class PSRAMAllocator {
public:
  using value_type = T;
  PSRAMAllocator() = default;
  template<class U> PSRAMAllocator(const PSRAMAllocator<U>&) noexcept {}
  T* allocate(size_t n) {
    T* ptr = reinterpret_cast<T*>(
      heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (ptr == nullptr)
      ptr = reinterpret_cast<T*>(malloc(n * sizeof(T)));
    return ptr;
  }
  void deallocate(T* p, size_t) noexcept { heap_caps_free(p); }
  template<class U> bool operator==(const PSRAMAllocator<U>&) const noexcept { return true; }
  template<class U> bool operator!=(const PSRAMAllocator<U>&) const noexcept { return false; }
};
// Prefer PSRAM for attribute storage, falling back to internal RAM
using psram_string = std::basic_string<char, std::char_traits<char>, PSRAMAllocator<char>>;
template<typename T> using psram_vector = std::vector<T, PSRAMAllocator<T>>;
#else
using psram_string = std::string;
template<typename T> using psram_vector = std::vector<T>;
#endif

} // namespace nspanel_lovelace
} // namespace esphome