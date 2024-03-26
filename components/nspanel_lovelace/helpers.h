#pragma once

#define M_PI 3.14159265358979323846 // pi

#include <array>
#include <cmath>
// #include <esp.h>
#include <string>
#include <map>
#include <math.h>
#include <stdint.h>
#include <time.h>

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

inline const char* value_or_empty(const char* s) {
  return s == nullptr ? "" : s; 
}

inline bool starts_with(const std::string &input, const std::string &value) {
  return input.rfind(value, 0) == 0;
}

// Arduino functions found in: 'cores/esp32/esp.cpp' and 'cores/esp32/esp32-hal-psram.cpp'
// inline static uint32_t get_psram_used() {
//   return (ESP.getPsramSize() - ESP.getFreePsram());
// }

inline bool iso8601_to_tm(const char* iso8601_string, tm &t) {
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
inline std::array<uint8_t, 3> hsv2rgb(double h, double s, double v) {
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
inline std::array<uint8_t, 3> xy_to_rgb(double x, double y, float wh) {
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

// see: https://stackoverflow.com/a/32821650/2634818
template <typename... Args>
std::string string_sprintf(const char *format, Args... args) {
  size_t length = std::snprintf(nullptr, 0, format, args...);
  assert(length >= 0);

  char *buf = new char[length + 1];
  std::snprintf(buf, length + 1, format, args...);

  std::string str(buf);
  delete[] buf;
  return str;
}

} // namespace nspanel_lovelace
} // namespace esphome