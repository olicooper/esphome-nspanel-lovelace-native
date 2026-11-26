#include <cstdio>
#include <cstdlib>
#include "esphome/core/time.h"
#include "host_weather.h"

using namespace esphome::nspanel_lovelace;
int main() {
  setenv("TZ", "UTC", 1);
  tzset();
  esphome::time::ParsedTimezone zone{};
  zone.std_offset_seconds = 5 * 3600;
  zone.dst_offset_seconds = 4 * 3600;
  zone.dst_start = {7200, 0, esphome::time::DSTRuleType::MONTH_WEEK_DAY, 3, 2, 0};
  zone.dst_end = {7200, 0, esphome::time::DSTRuleType::MONTH_WEEK_DAY, 11, 1, 0};
  esphome::time::set_global_tz(zone);
  std::string day;
  uint8_t weekday = 0;
  bool ok = weather_datetime_to_local_day("2026-07-01T03:30:00Z", day, weekday);
  if (!ok || day != "2026-06-30" || weekday != 2) {
    std::fprintf(stderr, "summer rollover: %s weekday %u\n", day.c_str(), weekday);
    return 1;
  }
  ok = weather_datetime_to_local_day("2026-01-01T04:30:00Z", day, weekday);
  if (!ok || day != "2025-12-31" || weekday != 3) {
    std::fprintf(stderr, "winter rollover: %s weekday %u\n", day.c_str(), weekday);
    return 1;
  }
  return 0;
}
