#pragma once

#include "defines.h"

#include <array>
#include <cassert>
#include <cmath>
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
inline void replace_first(
    std::string &s, std::string const &toReplace,
    std::string const &replaceWith) {
  std::size_t pos = s.find(toReplace);
  if (pos == std::string::npos)
    return;
  s.replace(pos, toReplace.length(), replaceWith);
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

inline void replace_all(std::string &s, const char oldChar, const char newChar) {
  size_t pos = std::string::npos;
  while ((pos = s.find(oldChar, pos + 1)) != std::string::npos) {
    s.at(pos) = newChar;
  }
}

inline const char* value_or_empty(const char* s) {
  return s == nullptr ? "" : s; 
}

inline unsigned long value_or_default(const std::string &str, unsigned long default_value) {
  return str.empty() || (str[0] != '-' && str[0] != '+' && !isdigit(str[0]))
    ? default_value : std::stoul(str);
}

inline int value_or_default(const std::string &str, int default_value) {
  return str.empty() || (str[0] != '-' && str[0] != '+' && !isdigit(str[0]))
    ? default_value : std::stoi(str);
}

inline unsigned int value_or_default(const std::string &str, unsigned int default_value) {
  return value_or_default(str, static_cast<unsigned long>(default_value));
}

inline double value_or_default(const std::string &str, double default_value) {
  return str.empty() || (str[0] != '-' && str[0] != '+' && !isdigit(str[0]))
    ? default_value : std::stod(str);
}

inline bool iso8601_to_tm(const char* iso8601_string, tm &t) {
  if (iso8601_string == nullptr) return false;
  
	// note: don't need to know the timezone
	static constexpr const char* format = "%d-%d-%dT%d:%d:%d";// "%d-%d-%dT%d:%d:%d+%d:%d";

	uint8_t parseCount = std::sscanf(iso8601_string, format, 
		&t.tm_year, &t.tm_mon, &t.tm_mday, 
		&t.tm_hour, &t.tm_min, &t.tm_sec
		//&tz_hr, &tz_min
	);
	
	if (parseCount < 3) {
		return false;
	}
	
	t.tm_year -= 1900;
	t.tm_mon -= 1;

  const time_t time_temp = mktime(&t);
	if (time_temp == -1) {
		return false;
	}
  // convert to utc
  t = *gmtime(&time_temp);

	return true;
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
      std::fmod((atan2(y, x) * (180 / M_PI)), 360) / 360,
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

inline bool contains_value(const std::string &str, const char *value) {
  return str.find(value, 0) != std::string::npos;
}

inline bool char_printable(const char value) {
  return value >= 0x20 && value <= 0x7e;
}

inline static constexpr bool str_equal(const char *a, const char *b) {
  return a == b || (a != nullptr && b != nullptr && std::strcmp(a, b) == 0);
}

inline void split_str(char delimiter, const std::string &str, std::vector<std::string> &array, uint16_t max_items = UINT16_MAX) {
  size_t pos_start = 0, pos_end = 0;
  std::string item;
  uint16_t item_count = 0;
  while ((pos_end = str.find(delimiter, pos_start)) != std::string::npos) {
    if (item_count == max_items) return;
    item = str.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + 1;
    if (!item.empty()) { array.push_back(item); }
    item_count++;
  }
  if (!item.empty()) { array.push_back(str.substr(pos_start)); }
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
  size_t pos_start = std::string::npos, pos_end = pos_start;
  std::string tmp;
  do {
      pos_start = str.find('\'', pos_end + 1);
      if (pos_start == std::string::npos) {
        if (tmp.back() == delimiter) tmp.pop_back();
        break;
      }
      pos_end = str.find('\'', pos_start + 1);
      if (pos_end == std::string::npos) break;
      // ignore empty entries
      if (pos_end - pos_start - 1 == 0) continue;
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

} // namespace nspanel_lovelace
} // namespace esphome