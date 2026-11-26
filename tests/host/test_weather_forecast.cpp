// Host-side tests for the production daily-forecast slot selection logic.
// The helper under test is extracted verbatim from nspanel_lovelace.h into
// host_weather.h by panel_sources.py; this TU never uses a copied parser.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "host_time.h"
namespace esphome { struct ESPTime { static HostLocalTime from_epoch_local(time_t t) { return host_local_time(t); } }; }
#include "host_weather.h"

using namespace esphome::nspanel_lovelace;

static int failures = 0;

static void check(bool ok, const char *what) {
  if (!ok) {
    printf("FAIL  %s\n", what);
    ++failures;
  } else {
    printf("ok    %s\n", what);
  }
}

static WeatherForecastSlotData day(const char *date_key, uint8_t weekday,
                                   const char *condition, float temp,
                                   bool valid = true) {
  WeatherForecastSlotData d;
  d.date_key = date_key;
  d.weekday = weekday;
  d.condition = condition;
  d.temperature = temp;
  d.valid = valid;
  return d;
}

static void test_short_list_clears_surplus_slots() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  for (auto &s : slots) s.filled = true;
  std::vector<WeatherForecastSlotData> entries = {
      day("2026-11-25", 2, "sunny", 12.3f),
      day("2026-11-26", 3, "cloudy", 10.1f),
      day("2026-11-27", 4, "rainy", 8.2f),
      day("2026-11-28", 5, "sunny", 14.0f),
      day("2026-11-29", 6, "sunny", 15.0f),
      day("2026-11-30", 0, "cloudy", 9.0f),
  };
  size_t n = build_weather_forecast_slots(entries, "2026-11-25", slots);
  check(n == 4, "six entries render exactly four slots");
  check(slots[0].filled && slots[1].filled && slots[2].filled && slots[3].filled,
        "four earliest slots stay filled");
  (void) n;
}

static void test_four_then_two_clears_surplus_slots() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  const std::vector<WeatherForecastSlotData> four = {
      day("2026-11-25", 2, "sunny", 12.3f),
      day("2026-11-26", 3, "cloudy", 10.1f),
      day("2026-11-27", 4, "rainy", 8.2f),
      day("2026-11-28", 5, "sunny", 14.0f),
  };
  build_weather_forecast_slots(four, "2026-11-25", slots);
  check(slots[2].filled && slots[3].filled, "four entries fill all four slots");
  const std::vector<WeatherForecastSlotData> two = {
      day("2026-11-25", 2, "sunny", 12.3f),
      day("2026-11-26", 3, "cloudy", 10.1f),
  };
  size_t n = build_weather_forecast_slots(two, "2026-11-25", slots);
  check(n == 2 && slots[0].filled && slots[1].filled && !slots[2].filled && !slots[3].filled,
        "shorter reply clears surplus slots 3 and 4 before publishing");
}

static void test_empty_valid_list_clears_all_slots() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  for (auto &s : slots) s.filled = true;
  size_t n = build_weather_forecast_slots({}, "2026-11-25", slots);
  check(n == 0, "empty list yields zero slots");
  check(!slots[0].filled && !slots[1].filled && !slots[2].filled && !slots[3].filled,
        "empty valid list clears all forecast slots");
}

static void test_invalid_entries_are_skipped() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  std::vector<WeatherForecastSlotData> entries = {
      day("2026-11-25", 2, "", 12.3f),                    // missing condition
      day("bad-timestamp", 3, "cloudy", 10.1f),            // malformed date key
      day("2026-11-27", 4, "rainy", std::nanf("")),        // nonnumeric temperature
      day("2026-11-28", 5, "sunny", 14.0f),
  };
  size_t n = build_weather_forecast_slots(entries, "2026-11-25", slots);
  check(n == 1, "missing condition, malformed date, nonnumeric temp all skipped");
  check(slots[0].filled && slots[0].date_key == "2026-11-28" &&
        slots[0].temperature == "14.0", "only valid entry survives, not rendered as zero");
}

static void test_chronological_earliest_four() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  std::vector<WeatherForecastSlotData> entries = {
      day("2026-11-30", 0, "cloudy", 9.0f),
      day("2026-11-26", 3, "cloudy", 10.1f),
      day("2026-11-28", 5, "sunny", 14.0f),
      day("2026-11-25", 2, "sunny", 12.3f),
      day("2026-11-27", 4, "rainy", 8.2f),
      day("2026-11-29", 6, "sunny", 15.0f),
  };
  build_weather_forecast_slots(entries, "2026-11-25", slots);
  check(slots[0].date_key == "2026-11-25" && slots[1].date_key == "2026-11-26" &&
        slots[2].date_key == "2026-11-27" && slots[3].date_key == "2026-11-28",
        "more than four valid entries renders earliest four chronologically");
}

static void test_expired_days_dropped() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  std::vector<WeatherForecastSlotData> entries = {
      day("2026-11-23", 0, "cloudy", 9.0f),  // two days past
      day("2026-11-24", 1, "sunny", 11.0f),  // yesterday
      day("2026-11-25", 2, "sunny", 12.3f),
      day("2026-11-26", 3, "cloudy", 10.1f),
  };
  size_t n = build_weather_forecast_slots(entries, "2026-11-25", slots);
  check(n == 2, "past days are expired, not rendered");
  check(slots[0].date_key == "2026-11-25" && slots[1].date_key == "2026-11-26",
        "only current and future days remain");
}

static void test_no_reply_leaves_blank_slots() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  size_t n = build_weather_forecast_slots({}, "", slots);
  check(n == 0 && !slots[0].filled, "no reply leaves initially blank slots");
}

static void test_local_weekday_used_for_label() {
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  // UTC 2026-11-25T23:30 is already 2026-11-26 in a UTC+2 panel timezone.
  std::vector<WeatherForecastSlotData> entries = {
      day("2026-11-26", 4, "sunny", 12.3f),
  };
  build_weather_forecast_slots(entries, "2026-11-26", slots);
  check(slots[0].date_key == "2026-11-26" && slots[0].weekday == 4,
        "panel-local date/weekday drives the label (midnight rollover)");
}

static void set_tz(const char * tz) { setenv("TZ", tz, 1); tzset(); }

static void test_iso_to_local_midnight_rollover() {
  const char *saved = getenv("TZ");
  std::string original = saved ? saved : "";
  set_tz("UTC-2");  // POSIX: UTC+2 panel timezone
  std::string key;
  uint8_t weekday = 7;
  bool ok = weather_datetime_to_local_day("2026-11-25T23:30:00+00:00", key, weekday);
  check(ok && key == "2026-11-26" && weekday == 4,
        "UTC late-evening reply rolls into the next local day with local weekday");
  if (original.empty()) unsetenv("TZ"); else set_tz(original.c_str());
}

static void test_iso_to_local_dst() {
  const char *saved = getenv("TZ");
  std::string original = saved ? saved : "";
  set_tz("CET-1CEST,M3.5.0,M10.5.0");  // Europe/Berlin, DST until late October
  std::string key;
  uint8_t weekday = 7;
  bool summer = weather_datetime_to_local_day("2026-07-15T22:30:00+00:00", key, weekday);
  check(summer && key == "2026-07-16" && weekday == 4,
        "DST offset rolls a summer UTC evening into the next local day");
  bool winter = weather_datetime_to_local_day("2026-11-15T22:30:00+00:00", key, weekday);
  check(winter && key == "2026-11-15" && weekday == 0,
        "standard-time offset keeps a winter UTC evening on the same local day");
  if (original.empty()) unsetenv("TZ"); else set_tz(original.c_str());
}

int main() {
  test_short_list_clears_surplus_slots();
  test_four_then_two_clears_surplus_slots();
  test_empty_valid_list_clears_all_slots();
  test_invalid_entries_are_skipped();
  test_chronological_earliest_four();
  test_expired_days_dropped();
  test_no_reply_leaves_blank_slots();
  test_local_weekday_used_for_label();
  test_iso_to_local_midnight_rollover();
  test_iso_to_local_dst();
  printf("\n%s (%d failures)\n", failures ? "FAILED" : "all passed", failures);
  return failures ? 1 : 0;
}
