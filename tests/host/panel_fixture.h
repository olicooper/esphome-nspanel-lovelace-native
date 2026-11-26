// Shared host fixture: real panel methods, with ESPHome services and display
// output replaced by host fixtures. Include from a test TU only; it defines
// the ESPHome stub environment, host_panel.inc and the Panel subclass.
#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <deque>
#include <utility>
#include <functional>
#include <map>
#include <optional>
#include <tuple>
#include <string>
#include <vector>

#define USE_TIME
// Native weather is compiled in so the weather dispatch/guard/renderer methods
// are the production code paths; JSON-dependent methods are extracted only
// when a real ArduinoJson include dir is available (see run.sh).
#define USE_NSPANEL_NATIVE_WEATHER
#ifndef ESP_LOGD
#define ESP_LOGD(...) ((void)0)
#endif
#ifndef ESP_LOGE
#define ESP_LOGE(...) ((void)0)
#endif
#ifndef ESP_LOGI
#define ESP_LOGI(...) ((void)0)
#endif
#ifndef ESP_LOGW
#define ESP_LOGW(...) ((void)0)
#endif
#ifndef ESP_LOGV
#define ESP_LOGV(...) ((void)0)
#endif
#define GPIO_NUM_4 4
#define pdMS_TO_TICKS(n) (n)
inline void gpio_set_level(int, int) {}
inline void vTaskDelay(int) {}

#include "host_time.h"
namespace esphome {
using std::optional;
using std::to_string;
using StringRef = std::string;
struct ESPPreferenceObject {};
struct ESPTime {
  static HostLocalTime from_epoch_local(time_t epoch) { return host_local_time(epoch); }
  time_t timestamp = 0;
  bool is_valid() const { return timestamp != 0; }
  tm value{};
  static ESPTime from_c_tm(tm *t, time_t ts) { ESPTime result; result.value = *t; result.timestamp = ts; return result; }
  bool fields_in_range(bool, bool) const {
    tm t = value;
    const time_t ts = timegm(&t);
    return ts != -1 && t.tm_year == value.tm_year && t.tm_mon == value.tm_mon &&
           t.tm_mday == value.tm_mday && t.tm_hour == value.tm_hour &&
           t.tm_min == value.tm_min && t.tm_sec == value.tm_sec;
  }
  void recalc_timestamp_utc(bool) { timestamp = timegm(&value); }
};
namespace time { struct RealTimeClock { ESPTime current; ESPTime now() { return current; } }; }
namespace setup_priority { constexpr float DATA = 0; }
struct Component {
  virtual void setup() {}
  virtual void loop() {}
  virtual void dump_config() {}
  virtual float get_setup_priority() const { return 0; }
  std::function<void()> interval;
  std::map<std::string, std::function<void()>> timeouts;
  void set_timeout(const char *name, int, std::function<void()> fn) { timeouts[name] = std::move(fn); }
  void cancel_timeout(const char *name) { timeouts.erase(name); }
  void set_interval(const char *, int, std::function<void()> fn) { interval = std::move(fn); }
  void cancel_interval(const char *) { interval = {}; }
};
namespace uart {
struct UARTDevice {
  std::deque<uint8_t> rx;
  size_t available() const { return rx.size(); }
  bool read_byte(uint8_t *value) {
    if (rx.empty()) return false;
    *value = rx.front();
    rx.pop_front();
    return true;
  }
};
}
uint64_t host_millis = 0;
uint32_t millis() { return static_cast<uint32_t>(host_millis); }
uint64_t millis_64() { return host_millis; }
int host_reboot_calls = 0;
struct HostApp {
  std::function<void()> teardown;
  void safe_reboot() {
    ++host_reboot_calls;
    if (teardown) teardown();
  }
} App;
#ifdef USE_NSPANEL_TFT_UPLOAD
namespace network {
int connectivity_checks = 0;
bool is_connected() { ++connectivity_checks; return false; }
}
#endif
namespace api {
struct CustomAPIDevice {};
struct APIServer {
  bool state_subscribed = false;
  template<class... Args> void subscribe_home_assistant_state(Args&&...) {}
  bool is_connected_with_state_subscription() const { return state_subscribed; }
};
APIServer *global_api_server = nullptr;
}
namespace api {
// Stub of esphome::api::HomeAssistantServiceCallAction<>: only play() is
// observable from the component; each instance counts its own dispatches.
template<typename... Ts> struct HomeAssistantServiceCallAction {
  int plays = 0;
  void play() { ++plays; }
};
}
template<class> struct CallbackManager;
template<class... Args> struct CallbackManager<void(Args...)> {
  std::vector<std::function<void(Args...)>> callbacks;
  void add(std::function<void(Args...)> fn) { callbacks.push_back(std::move(fn)); }
  void call(Args... args) { for (auto &fn : callbacks) fn(args...); }
};
bool str_startswith(const std::string &s, const char *prefix) { return s.rfind(prefix, 0) == 0; }
std::string str_snprintf(const char *format, size_t len, ...) {
  std::vector<char> buffer(len + 1);
  va_list args;
  va_start(args, len);
  vsnprintf(buffer.data(), buffer.size(), format, args);
  va_end(args);
  return buffer.data();
}
uint16_t encode_uint16(uint8_t high, uint8_t low) { return (high << 8) | low; }
inline std::string format_hex(const std::vector<uint8_t> &) { return {}; }
uint16_t crc16(const uint8_t *bytes, size_t count) {
  uint16_t crc = 0xffff;
  while (count--) {
    crc ^= *bytes++;
    for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) ? 0xa001 : 0);
  }
  return crc;
}
}

// Forward declaration matching the real ArduinoJson 7 class; the JSON-gated
// weather test includes the real header instead (guard: the real class lives
// in an ArduinoJson inline namespace and would be ambiguous with this one).
#ifndef HOST_ARDUINOJSON
namespace ArduinoJson { class JsonObjectConst; }
#endif

#include "host_panel.h"
#include "card_items.h"
#include "cards.h"
#include "pages.h"
#include "translations.h"

namespace esphome::nspanel_lovelace {
FrozenCharMap<const char *, TRANSLATION_MAP_SIZE> TRANSLATION_MAP{};
#include "host_panel.inc"

using Data = std::map<std::string, std::string>;
struct Call { std::string action; Data data; Data data_template; };
static std::vector<Call> calls;
static std::vector<std::string> display;
void NSPanelLovelace::setup() {}
void NSPanelLovelace::render_item_update_() { display.emplace_back("tile update"); }
void NSPanelLovelace::dump_config() {}
bool NSPanelLovelace::save_state_() { return true; }
bool NSPanelLovelace::restore_state_() { return false; }
void NSPanelLovelace::update_datetime(datetime_mode, const char *, const char *) {}
void NSPanelLovelace::render_page_(render_page_option) {}
void NSPanelLovelace::render_current_page_() { display.emplace_back("current page"); }
void NSPanelLovelace::render_popup_notify_page_(const std::string &, const std::string &,
    const std::string &, uint16_t, const std::string &, const std::string &) {}
void NSPanelLovelace::render_climate_detail_update_(Entity *, const std::string &) {}
void NSPanelLovelace::render_climate_detail_update_(StatefulPageItem *) {}
void NSPanelLovelace::process_display_command_queue_() {
  if (command_buffer_.empty()) return;
  display.push_back(command_buffer_);
  command_buffer_.clear();
}
void NSPanelLovelace::call_ha_service_(const std::string &action, const Data &data, const Data &templates) {
  calls.push_back({action, data, templates});
}

struct Panel : NSPanelLovelace {
  using NSPanelLovelace::render_popup_page_update_;
  using NSPanelLovelace::render_cover_detail_update_;
  using NSPanelLovelace::command_buffer_;
  using NSPanelLovelace::page_mgr_;
  using NSPanelLovelace::popup_page_current_uuid_;
  using NSPanelLovelace::on_entity_attribute_update_;
  void feed(const std::vector<uint8_t> &bytes) {
    rx.insert(rx.end(), bytes.begin(), bytes.end());
    loop();
  }
  size_t buffered() const { return buffer_.size(); }
  static std::vector<uint8_t> encode(const std::string &payload) {
    std::vector<uint8_t> wire{0x55, 0xbb, static_cast<uint8_t>(payload.size()),
                             static_cast<uint8_t>(payload.size() >> 8)};
    wire.insert(wire.end(), payload.begin(), payload.end());
    auto crc = crc16(wire.data(), wire.size());
    wire.push_back(crc & 0xff);
    wire.push_back(crc >> 8);
    return wire;
  }
  void run_timeout(const char *name = "btnpr") {
    auto pending = timeouts.extract(name);
    if (!pending.empty()) pending.mapped()();
    loop();
  }
  auto debounce_state() const {
    return std::make_tuple(button_press_timeout_set_, button_press_uuid_, button_press_type_, button_press_value_,
                           timeouts.count("btnpr"));
  }
  void frame(const std::string &payload) {
    feed(encode(payload));
    run_timeout();
  }
};

static int failures;
static void check(bool ok, const std::string &label) {
  printf("%s %s\n", ok ? "ok  " : "FAIL", label.c_str());
  failures += !ok;
}
}  // namespace esphome::nspanel_lovelace
