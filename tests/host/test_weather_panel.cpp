// Host-side tests for the production native-weather panel paths: dispatch and
// timeout, reply guards, blank forecast-slot rendering and expiry, all driven
// through the real NSPanelLovelace methods (extracted verbatim by
// panel_sources.py) with real WeatherItem wire rendering.
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#ifdef HOST_ARDUINOJSON
#include <ArduinoJson.h>
#endif

// Capture warnings so the rate-limited diagnostics are assertable; the macro
// must be defined before panel_fixture.h (which defaults them to no-ops).
static std::vector<std::string> host_warnings;
#define ESP_LOGW(tag, ...) \
  do { char nsp_buf[256]; snprintf(nsp_buf, sizeof(nsp_buf), __VA_ARGS__); \
       host_warnings.emplace_back(nsp_buf); } while (0)

#define USE_NSPANEL_TFT_UPLOAD
using esp_http_client_handle_t = void *;
#include "panel_fixture.h"

using namespace esphome;
using namespace esphome::nspanel_lovelace;

namespace {

api::APIServer server;
api::HomeAssistantServiceCallAction<> actions[4];

struct WeatherPanel : Panel {
  using NSPanelLovelace::is_updating_;
  using NSPanelLovelace::reparse_mode_;
  using NSPanelLovelace::expire_weather_slots_;
  using NSPanelLovelace::publish_weather_slots_;
  using NSPanelLovelace::on_weather_temperature_update_;
  using NSPanelLovelace::weather_forecast_json_;
  using NSPanelLovelace::weather_in_flight_;
  using NSPanelLovelace::weather_pending_slot_;
  using NSPanelLovelace::weather_slot_dates_;
  using NSPanelLovelace::weather_slot_used_;
  using NSPanelLovelace::weather_unanswered_;
};

// A screensaver with the current-condition item filled and four fresh forecast
// slots; HA is connected so check_weather_forecast_ can dispatch.
static void setup_panel(WeatherPanel &panel) {
  host_millis += 70000;  // keep every test's first diagnostic past the 60-s throttle baseline
  for (auto &action : actions) action.plays = 0;
  host_warnings.clear();
  display.clear();
  server = api::APIServer{};
  server.state_subscribed = true;
  esphome::api::global_api_server = &server;
  panel.set_weather_entity_id("weather.lab");
  for (uint8_t slot = 0; slot < 4; ++slot)
    panel.set_weather_forecast_action(slot, &actions[slot]);
  auto *screensaver = panel.create_screensaver("screensaver");
  auto current = std::make_shared<WeatherItem>("current");
  current->set_icon_by_weather_condition("sunny");
  current->set_value("21.5");
  screensaver->add_item(current);
  for (int i = 0; i < 4; ++i)
    screensaver->add_item(std::make_shared<WeatherItem>(std::string("fc") + char('0' + i)));
  panel.page_mgr_.find_page("screensaver", true);
  panel.loop();                       // HA-ready transition (no dispatch yet)
  host_millis += 2000;
  panel.loop();                       // connect delay -> dispatch on slot 0
  display.clear();
}

static constexpr char RESTART_TIMER[] = "nspanel_forecast_restart";

static void reconnect(WeatherPanel &panel) {
  server.state_subscribed = false;
  panel.loop();
  server.state_subscribed = true;
  panel.loop();
  host_millis += 2000;
  panel.loop();
}

static void fill_three_slots(WeatherPanel &panel) {
  host_reboot_calls = 0;
  setup_panel(panel);
  for (unsigned i = 0; i < 3; ++i) {
    if (i != 0) {
      host_millis += 1800000;
      panel.loop();
    }
    check(actions[i].plays == 1, "production fetch occupies the next slot");
    host_millis += 31000;
    panel.loop();
  }
}

static void fill_four_slots(WeatherPanel &panel) {
  fill_three_slots(panel);
  reconnect(panel);
  check(actions[3].plays == 1 && panel.weather_in_flight_, "reconnect dispatches on the fourth slot");
}

static void test_stall_restart_disabled() {
  WeatherPanel no_weather;
  no_weather.set_weather_forecast_stall_restart(1000);
  no_weather.loop();
  check(!no_weather.timeouts.count(RESTART_TIMER), "delay alone does not enable weather");
  WeatherPanel panel;
  fill_four_slots(panel);
  host_millis += 31000;
  panel.loop();
  host_millis += 3600000;
  panel.loop();
  check(!panel.timeouts.count(RESTART_TIMER) && host_reboot_calls == 0, "default off remains off at full saturation");
  check(!host_warnings.empty() && host_warnings.back().find("all forecast slots hold unanswered replies") != std::string::npos,
        "default off preserves the capacity diagnostic");
  WeatherPanel zero;
  zero.set_weather_forecast_stall_restart(0);
  fill_three_slots(zero);
  host_millis += 3600000;
  zero.loop();
  check(actions[3].plays == 0 && !zero.timeouts.count(RESTART_TIMER),
        "explicit zero preserves the three-timeout cap while connected");
}

static void test_stall_restart_arms_only_at_four_once_per_boot() {
  for (int boot = 0; boot < 2; ++boot) {
    WeatherPanel panel;
    panel.set_weather_forecast_stall_restart(60000);
    fill_three_slots(panel);
    host_millis += 1799999;
    panel.loop();
    check(panel.weather_unanswered_ == 3 && actions[3].plays == 0 && !panel.timeouts.count(RESTART_TIMER),
          "three timeouts do not arm and the fourth waits for the full backoff");
    ++host_millis;
    panel.loop();
    check(actions[3].plays == 1, "fourth dispatch at the backoff boundary without reconnect");
    host_millis += 59999;
    panel.loop();
    check(!panel.timeouts.count(RESTART_TIMER), "reboot not queued before the deadline");
    ++host_millis;
    panel.loop();
    check(panel.timeouts.count(RESTART_TIMER) == 1 && host_reboot_calls == 0, "reboot deferred exactly at the deadline measured from fill");
    panel.run_timeout(RESTART_TIMER);
    for (int i = 0; i < 10; ++i) panel.loop();
    check(host_reboot_calls == 1 && !panel.timeouts.count(RESTART_TIMER), "one reboot per stall on each fresh boot");
  }
}

static void test_late_reply_after_cap_permits_bounded_retry() {
  WeatherPanel panel;
  panel.set_weather_forecast_stall_restart(1000);
  fill_three_slots(panel);
  panel.handle_weather_forecast_error("late", 0);
  host_millis += 1800000;
  panel.loop();
  check(actions[3].plays == 1 && !panel.timeouts.count(RESTART_TIMER),
        "late reply frees capacity; the first bounded retry does not fill all slots");
  host_millis += 30000;
  panel.loop();
  check(!panel.timeouts.count(RESTART_TIMER), "three occupied slots still cannot arm");
  host_millis += 1799999;
  panel.loop();
  check(actions[0].plays == 1, "retry still observes backoff after a further timeout");
  ++host_millis;
  panel.loop();
  check(actions[0].plays == 2 && actions[1].plays == 1 && actions[2].plays == 1,
        "only the freed slot is reused; prior unanswered cap cannot strand it");
}

static void test_stall_restart_waits_for_live_request_across_wrap() {
  WeatherPanel panel;
  panel.set_weather_forecast_stall_restart(1000);
  host_millis = (1ull << 32) - (72000 + 2 * 1800000 + 3 * 31000 + 2000 + 1000);
  fill_four_slots(panel);
  host_millis += 1000;
  check(millis() == 0, "fixture crosses the 32-bit wrap at the restart deadline");
  panel.loop();
  check(panel.weather_in_flight_ && !panel.timeouts.count(RESTART_TIMER), "expired delay does not interrupt an authoritative reply");
  host_millis += 29000;
  panel.loop();
  panel.run_timeout(RESTART_TIMER);
  check(host_reboot_calls == 1, "original deadline remains due when the request times out across wrap");
}

static void test_stall_restart_rechecks_ha_and_tft_when_deferred() {
  WeatherPanel panel;
  panel.set_weather_forecast_stall_restart(1000);
  fill_four_slots(panel);
  host_millis += 31000;
  server.state_subscribed = false;
  panel.loop();
  check(!panel.timeouts.count(RESTART_TIMER), "expired stall waits offline");
  server.state_subscribed = true;
  panel.loop();
  check(!panel.timeouts.count(RESTART_TIMER), "first reconnect loop cannot queue overdue reboot");
  host_millis += 29999;
  panel.loop();
  check(!panel.timeouts.count(RESTART_TIMER), "1 ms before HA qualifies");
  ++host_millis;
  panel.loop();
  check(panel.timeouts.count(RESTART_TIMER), "HA qualifies at 30 seconds; original countdown still due");
  server.state_subscribed = false;
  panel.run_timeout(RESTART_TIMER);
  check(host_reboot_calls == 0, "deferred reboot rechecks HA before the next panel loop");
  server.state_subscribed = true;
  panel.loop();
  check(!panel.timeouts.count(RESTART_TIMER), "disconnect seen only at commitment resets qualification");
  host_millis += 30000;
  panel.loop();
  for (auto flag : {&WeatherPanel::is_updating_, &WeatherPanel::reparse_mode_}) {
    check(panel.timeouts.count(RESTART_TIMER), "guard release does not reset the countdown");
    panel.*flag = true;
    panel.run_timeout(RESTART_TIMER);
    check(host_reboot_calls == 0 && !panel.timeouts.count(RESTART_TIMER), "deferred reboot rechecks TFT guards");
    panel.*flag = false;
    panel.loop();
  }
  panel.run_timeout(RESTART_TIMER);
  check(host_reboot_calls == 1, "reboot once execution guards permit it");
}

static void test_ha_disconnect_during_tft_guard_requires_fresh_readiness() {
  for (auto flag : {&WeatherPanel::is_updating_, &WeatherPanel::reparse_mode_}) {
    WeatherPanel panel;
    panel.set_weather_forecast_stall_restart(60000);
    fill_four_slots(panel);
    host_millis += 30000;
    panel.loop();  // Retire the fourth request before guarding the panel loop.

    panel.*flag = true;
    server.state_subscribed = false;
    panel.loop();  // Observe the disconnect while the guard is active.
    host_millis += 30000;  // The original stall deadline expires while offline.
    server.state_subscribed = true;
    panel.*flag = false;
    panel.loop();
    check(!panel.timeouts.count(RESTART_TIMER), "HA loss during the TFT guard invalidates prior readiness");
    host_millis += 29999;
    panel.loop();
    check(!panel.timeouts.count(RESTART_TIMER), "TFT guard release still requires a full 30 seconds of HA readiness");
    ++host_millis;
    panel.loop();
    check(panel.timeouts.count(RESTART_TIMER), "fresh readiness qualifies at 30 seconds without rearming the stall deadline");
    panel.run_timeout(RESTART_TIMER);
    check(host_reboot_calls == 1, "the overdue restart runs once after HA requalifies");
  }
}

static void test_shutdown_rejects_pending_tft_upload() {
  WeatherPanel panel;
  panel.set_weather_forecast_stall_restart(1000);
  fill_four_slots(panel);
  host_millis += 31000;
  panel.loop();
  network::connectivity_checks = 0;
  App.teardown = [&panel]() { panel.upload_tft("http://fixture.invalid/panel.tft"); };
  panel.run_timeout(RESTART_TIMER);
  App.teardown = {};
  check(host_reboot_calls == 1 && network::connectivity_checks == 0,
        "TFT request dispatched during safe shutdown is rejected at admission");
  panel.upload_tft("http://fixture.invalid/panel.tft");
  check(network::connectivity_checks == 0, "restart commitment remains in force until the device reboots");
  network::connectivity_checks = 0;
  WeatherPanel fresh;
  fresh.upload_tft("http://fixture.invalid/panel.tft");
  check(network::connectivity_checks == 1, "an uncommitted panel still admits upload to network checks");
}

static void test_late_reply_cancels_deferred_restart_before_slot_refills() {
  WeatherPanel panel;
  panel.set_weather_forecast_stall_restart(120000);
  fill_four_slots(panel);
  host_millis += 120000;
  panel.loop();
  panel.loop();
  check(panel.timeouts.count(RESTART_TIMER), "expired stall has deferred work to cancel");
  panel.handle_weather_forecast_error("late", 1);
  check(!panel.timeouts.count(RESTART_TIMER), "freeing an old slot cancels deferred work immediately");
  reconnect(panel);
  check(actions[1].plays == 2, "only the freed slot was reused");
  host_millis += 31000;
  panel.loop();
  panel.run_timeout(RESTART_TIMER);
  check(host_reboot_calls == 0, "refill uses a new deadline, not the cancelled one");
  host_millis += 89000;
  panel.loop();
  panel.run_timeout(RESTART_TIMER);
  check(host_reboot_calls == 1, "refill arms a fresh stall");
}

// Panel-local "today" under run.sh's TZ=UTC, matching weather_today_date_key_.
static std::string day_key(int offset_days) {
  time_t t = ::time(nullptr) + static_cast<time_t>(offset_days) * 86400;
  tm local{};
  localtime_r(&t, &local);
  char buffer[16];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &local);
  return buffer;
}

static WeatherForecastOutputSlot filled_slot(const std::string &date_key, uint8_t weekday,
                                             const char *condition, const char *temperature) {
  WeatherForecastOutputSlot slot;
  slot.date_key = date_key;
  slot.weekday = weekday;
  slot.condition = condition;
  slot.temperature = temperature;
  slot.filled = true;
  return slot;
}

static std::vector<std::string> split_fields(const std::string &wire) {
  std::vector<std::string> fields;
  size_t start = 0, end;
  while ((end = wire.find('~', start)) != std::string::npos) {
    fields.emplace_back(wire, start, end - start);
    start = end + 1;
  }
  fields.emplace_back(wire, start);
  return fields;
}

// weatherUpdate~(5x)[type~internalName~icon~iconColor~displayName~value]:
// item i occupies fields 1+6i .. 6+6i.
static bool item_blank(const std::vector<std::string> &fields, size_t item) {
  const size_t base = 1 + 6 * item;
  if (fields.size() < base + 6) return false;
  for (size_t k = 0; k < 6; ++k)
    if (!fields[base + k].empty()) return false;
  return true;
}

static std::string item_value(const std::vector<std::string> &fields, size_t item) {
  const size_t index = 6 + 6 * item;
  return index < fields.size() ? fields[index] : std::string();
}

// Blocker 2: a shorter (or empty) forecast must serialize surplus slots as
// truly blank — no cached/default icon, no day label, no 0.0 temperature.
static void test_short_reply_blanks_surplus_slots_on_wire() {
  WeatherPanel panel;
  setup_panel(panel);
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  slots[0] = filled_slot(day_key(0), 2, "sunny", "12.3");
  slots[1] = filled_slot(day_key(1), 3, "cloudy", "10.1");
  panel.publish_weather_slots_(slots);
  check(display.size() == 1, "short reply publishes exactly one weatherUpdate");
  if (display.size() != 1) return;
  const auto fields = split_fields(display.back());
  check(fields.size() == 31 && fields.front() == "weatherUpdate",
        "weatherUpdate wire keeps five six-field items");
  check(item_value(fields, 1) == "12.3°C" && item_value(fields, 2) == "10.1°C",
        "short reply still renders its two filled slots");
  check(item_blank(fields, 3) && item_blank(fields, 4),
        "surplus slots serialize blank: no icon, no day, no 0.0 temperature");
  check(item_value(fields, 0) == "21.5°C" && !fields[3].empty(),
        "current-condition item untouched by short-reply blanking");
}

// Blocker 2: a day expired by the panel-local date must serialize blank.
static void test_expired_day_blanks_on_wire() {
  WeatherPanel panel;
  setup_panel(panel);
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  slots[0] = filled_slot(day_key(0), 2, "sunny", "12.3");
  slots[1] = filled_slot(day_key(1), 3, "cloudy", "10.1");
  panel.publish_weather_slots_(slots);
  panel.weather_slot_dates_[0] = "1999-01-01";  // simulate midnight rollover past slot 1
  display.clear();
  panel.expire_weather_slots_();
  check(display.size() == 1, "expiry republishes the screensaver");
  if (display.size() != 1) return;
  const auto fields = split_fields(display.back());
  check(item_blank(fields, 1), "expired slot serializes blank, not the old temperature");
  check(item_value(fields, 2) == "10.1°C", "expiry leaves still-relevant days rendered");
}

// Blocker 3: an explicit HA error for the active forecast blanks the forecast
// slots only, keeps current conditions, and counts the failure.
static void test_active_error_blanks_forecast_only() {
  WeatherPanel panel;
  setup_panel(panel);
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  for (size_t i = 0; i < slots.size(); ++i)
    slots[i] = filled_slot(day_key(static_cast<int>(i)), 2, "sunny", "12.3");
  panel.publish_weather_slots_(slots);  // active forecast on screen
  display.clear();
  panel.handle_weather_forecast_error(
      "Service call requested response data but did not match any entities", 0);
  check(display.size() == 1, "active error republishes the forecast slots");
  if (display.size() != 1) return;
  const auto fields = split_fields(display.back());
  check(item_blank(fields, 1) && item_blank(fields, 2) &&
        item_blank(fields, 3) && item_blank(fields, 4),
        "active error blanks all four forecast slots");
  check(item_value(fields, 0) == "21.5°C" && !fields[3].empty() && !fields[4].empty(),
        "active error keeps the current-condition item's icon and value");
  check(!panel.weather_in_flight_ && panel.weather_pending_slot_ == 4,
        "active error releases the in-flight bookkeeping");
  check(panel.weather_unanswered_ == 0, "active error is an answered request");
}

// Blocker 1 pin: a stale error for a retired slot must not clear or re-render
// the newer forecast and must not disturb the active dispatch's bookkeeping.
static void test_stale_error_cannot_clear_newer_forecast() {
  WeatherPanel panel;
  setup_panel(panel);                    // dispatch on slot 0
  host_millis += 31000;
  panel.loop();                          // self-timeout
  host_millis += 30u * 60u * 1000u;
  panel.loop();                          // fresh dispatch lands on slot 1
  check(actions[1].plays == 1, "newer forecast dispatched on the next free slot");
  std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> slots{};
  slots[0] = filled_slot(day_key(0), 2, "sunny", "12.3");
  slots[1] = filled_slot(day_key(1), 3, "cloudy", "10.1");
  panel.publish_weather_slots_(slots);   // the newer forecast is on screen
  display.clear();
  panel.handle_weather_forecast_error("late error", 0);
  check(display.empty(), "stale error renders nothing over the newer forecast");
  check(panel.weather_in_flight_ && panel.weather_pending_slot_ == 1,
        "stale error leaves the active dispatch bookkeeping alone");
  check(!panel.weather_slot_used_[0], "stale reply retires only its own old slot");
}

// Timeout deadline semantics: the self-timeout fires only past the deadline
// and the timeout cycle itself must not dispatch a second request.
static void test_timeout_deadline_semantics() {
  WeatherPanel panel;
  setup_panel(panel);
  check(panel.weather_in_flight_ && actions[0].plays == 1, "one request in flight after dispatch");
  host_millis += 29000;
  panel.loop();
  check(panel.weather_in_flight_ && panel.weather_unanswered_ == 0 && actions[1].plays == 0,
        "no timeout and no second dispatch before the 30-s deadline");
  host_millis += 1100;
  panel.loop();
  check(!panel.weather_in_flight_ && panel.weather_unanswered_ == 1 && actions[1].plays == 0,
        "self-timeout fires after the deadline without dispatching");
}

// Blocker 3 (diagnostics): active-error and late-reply diagnostics must go
// through the existing 60-s rate limiter, never an unthrottled log per reply.
static void test_reply_diagnostics_are_rate_limited() {
  WeatherPanel panel;
  setup_panel(panel);
  host_millis += 31000;
  panel.loop();  // self-timeout -> first (rate-limited) diagnostic
  check(host_warnings.size() == 1, "self-timeout logs exactly one diagnostic");
  host_warnings.clear();
  panel.handle_weather_forecast_error("e1", 0);
  panel.handle_weather_forecast_error("e2", 2);
  panel.handle_weather_forecast_error("e3", 3);
  check(host_warnings.empty(),
        "three late replies within 60 s add no unthrottled diagnostics");
  host_millis += 61000;
  panel.handle_weather_forecast_error("e4", 1);
  check(host_warnings.size() == 1, "the next diagnostic is allowed after the throttle window");
}

// Requirement: freshly created forecast slots render blank on the wire before
// any reply arrives, via the production init + render paths.
static void test_deadline_and_first_diagnostic() {
  WeatherPanel panel;
  setup_panel(panel);
  host_warnings.clear();
  host_millis += 30000;
  panel.handle_weather_forecast_error("deadline", 0);
  check(display.empty(), "reply at deadline does not render");
  check(!panel.weather_in_flight_ && !panel.weather_slot_used_[0], "expired reply retires callback and active dispatch");
  check(panel.weather_unanswered_ == 1, "expired reply counts as timeout");
  WeatherPanel fresh;
  setup_panel(fresh);
  host_warnings.clear();
  host_millis = 10;
  fresh.handle_weather_forecast_error("early", 0);
  check(host_warnings.size() == 1, "first diagnostic emits before one minute");
}

static void test_error_deadline_wrap() {
  for (uint32_t elapsed : {29999u, 30000u, 30001u}) {
    WeatherPanel panel;
    host_millis = UINT32_MAX - 80000u;
    setup_panel(panel);
    host_millis += elapsed;
    panel.handle_weather_forecast_error("wrap", 0);
    check(panel.weather_unanswered_ == (elapsed >= 30000u ? 1 : 0),
          "error deadline respects millis wrap");
    check(display.empty() == (elapsed >= 30000u),
          "expired error cannot render across millis wrap");
  }
}

static void test_answered_errors_continue_polling() {
  WeatherPanel panel;
  setup_panel(panel);
  for (int i = 0; i < 4; ++i) {
    panel.handle_weather_forecast_error("unsupported", i % 4);
    check(panel.weather_unanswered_ == 0, "answered error does not consume timeout budget");
    host_millis += 30u * 60u * 1000u;
    panel.loop();
  }
  check(actions[0].plays == 2, "polling continues after four answered errors");
}

static void test_initial_slots_blank_on_wire() {
  WeatherPanel panel;
  host_millis += 70000;
  host_warnings.clear();
  display.clear();
  server = api::APIServer{};
  esphome::api::global_api_server = &server;  // not subscribed: no dispatch
  panel.set_weather_entity_id("weather.lab");
  auto *screensaver = panel.create_screensaver("screensaver");
  auto current = std::make_shared<WeatherItem>("current");
  current->set_icon_by_weather_condition("sunny");
  current->set_value("21.5");
  screensaver->add_item(current);
  for (int i = 0; i < 4; ++i)
    screensaver->add_item(std::make_shared<WeatherItem>(std::string("fc") + char('0' + i)));
  panel.page_mgr_.find_page("screensaver", true);
  panel.init_weather_forecast_slots();
  panel.on_weather_temperature_update_("weather.lab", "21.5");  // production render trigger
  check(display.size() == 1, "initial render publishes once");
  if (display.size() != 1) return;
  const auto fields = split_fields(display.back());
  check(item_blank(fields, 1) && item_blank(fields, 2) &&
        item_blank(fields, 3) && item_blank(fields, 4),
        "initial forecast slots serialize blank: no default icon, no 0.0 temperature");
  check(item_value(fields, 0) == "21.5°C", "current-condition item keeps its value");
}

#ifdef HOST_ARDUINOJSON
// Real ArduinoJson 7 (the firmware dependency): drive the production
// apply_weather_forecast callback with the wire shape HA sends for
// weather.get_forecasts (Task 1 contract).
static void apply_forecast(WeatherPanel &panel, const std::string &json, uint8_t slot) {
  ArduinoJson::JsonDocument doc;
  const auto err = ArduinoJson::deserializeJson(doc, json);
  check(!err && !doc.overflowed(),
        std::string("fixture forecast JSON parses (") + err.c_str() + ")");
  panel.apply_weather_forecast(doc.as<ArduinoJson::JsonObjectConst>(), slot);
}

static std::string forecast_json(const char *d0, float t0, const char *d1, float t1) {
  char buffer[512];
  snprintf(buffer, sizeof(buffer),
           R"({"response":{"weather.lab":{"forecast":[)"
           R"({"datetime":"%sT12:00:00Z","temperature":%.1f,"condition":"sunny"},)"
           R"({"datetime":"%sT12:00:00Z","temperature":%.1f,"condition":"cloudy"}])"
           R"(}}})",
           d0, t0, d1, t1);
  return buffer;
}

// Blocker 1: a success reply for an old (timed-out, retired) slot must not
// overwrite or re-render the newer forecast, including its stored slot dates.
static void test_out_of_order_success_cannot_overwrite_newer_data() {
  WeatherPanel panel;
  setup_panel(panel);                    // dispatch on slot 0
  host_millis += 31000;
  panel.loop();                          // slot 0 self-timeout, slot stays retired-pending
  host_millis += 30u * 60u * 1000u;
  panel.loop();                          // newer dispatch on slot 1
  const std::string today = day_key(0), tomorrow = day_key(1);
  apply_forecast(panel, forecast_json(today.c_str(), 12.3f, tomorrow.c_str(), 10.1f), 1);
  check(display.size() == 1, "newer success renders once");
  if (display.size() != 1) return;
  check(item_value(split_fields(display.back()), 1) == "12.3°C",
        "newer success renders its temperatures");
  display.clear();
  apply_forecast(panel, forecast_json(today.c_str(), 99.9f, tomorrow.c_str(), 88.8f), 0);
  check(display.empty(), "out-of-order success does not re-render the screensaver");
  // Force a fresh production render: the stored forecast must still be the
  // newer reply's, not the stale one's.
  panel.on_weather_temperature_update_("weather.lab", "20.0");
  check(display.size() == 1, "current-condition update republishes the screensaver");
  if (display.size() != 1) return;
  const auto fields = split_fields(display.back());
  check(item_value(fields, 1) == "12.3°C",
        "stale success cannot overwrite the newer stored forecast");
  check(item_value(fields, 0) == "20.0°C", "current conditions still update normally");
  check(!panel.weather_slot_used_[0], "stale success retires only its own old slot");
}

// Blocker 2: an empty (but valid) forecast list from HA must blank every slot
// on the wire while the current-condition item stays intact.
static void test_fourth_reply_recovers_without_restart() {
  WeatherPanel panel;
  panel.set_weather_forecast_stall_restart(1000);
  fill_three_slots(panel);
  host_millis += 1800000;
  panel.loop();
  const auto today = day_key(0);
  const auto tomorrow = day_key(1);
  apply_forecast(panel, forecast_json(today.c_str(), 42.3f, tomorrow.c_str(), 43.1f), 3);
  check(display.size() == 1 && item_value(split_fields(display.back()), 1) == "42.3°C",
        "timely fourth reply at backoff renders fresh forecast without reconnect");
  host_millis += 30000;
  panel.loop();
  check(!panel.weather_slot_used_[3] && !panel.timeouts.count(RESTART_TIMER) && host_reboot_calls == 0,
        "fourth success frees the slot and cancels its restart");
}

static void test_apply_empty_forecast_blanks_wire() {
  WeatherPanel panel;
  setup_panel(panel);
  apply_forecast(panel, R"({"response":{"weather.lab":{"forecast":[]}}})", 0);
  check(display.size() == 1, "empty forecast publishes once");
  if (display.size() != 1) return;
  const auto fields = split_fields(display.back());
  check(item_blank(fields, 1) && item_blank(fields, 2) &&
        item_blank(fields, 3) && item_blank(fields, 4),
        "empty forecast list serializes all four slots blank");
  check(item_value(fields, 0) == "21.5°C", "empty forecast keeps current conditions");
}
#endif  // HOST_ARDUINOJSON

}  // namespace

int main() {
  test_short_reply_blanks_surplus_slots_on_wire();
  test_expired_day_blanks_on_wire();
  test_active_error_blanks_forecast_only();
  test_stale_error_cannot_clear_newer_forecast();
  test_timeout_deadline_semantics();
  test_reply_diagnostics_are_rate_limited();
  test_deadline_and_first_diagnostic();
  test_error_deadline_wrap();
  test_answered_errors_continue_polling();
  test_initial_slots_blank_on_wire();
  test_stall_restart_disabled();
  test_stall_restart_arms_only_at_four_once_per_boot();
  test_late_reply_after_cap_permits_bounded_retry();
  test_stall_restart_waits_for_live_request_across_wrap();
  test_stall_restart_rechecks_ha_and_tft_when_deferred();
  test_ha_disconnect_during_tft_guard_requires_fresh_readiness();
  test_late_reply_cancels_deferred_restart_before_slot_refills();
  test_shutdown_rejects_pending_tft_upload();
#ifdef HOST_ARDUINOJSON
  test_out_of_order_success_cannot_overwrite_newer_data();
  test_fourth_reply_recovers_without_restart();
  test_apply_empty_forecast_blanks_wire();
#endif
  printf("\n%s (%d failures)\n", failures ? "FAILED" : "all passed", failures);
  return failures ? 1 : 0;
}
