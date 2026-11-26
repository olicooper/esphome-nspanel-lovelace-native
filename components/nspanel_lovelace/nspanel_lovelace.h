#pragma once

#include "defines.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <queue>
#include <stdint.h>
#include <utility>
#include <vector>

#include "esphome/core/defines.h"
// str_snprintf and friends: not pulled in through helpers.h since ESPHome 2026.11
#include "esphome/core/alloc_helpers.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/core/string_ref.h"
#include "esphome/core/version.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/api/custom_api_device.h"
#ifdef USE_NSPANEL_NATIVE_WEATHER
#include "esphome/components/api/homeassistant_service.h"
#endif

#include <driver/gpio.h>
#include "esphome/components/uart/uart_component_esp_idf.h"
#ifdef USE_NSPANEL_TFT_UPLOAD
#include <esp_http_client.h>
#endif

#ifdef USE_TIME
#include "esphome/core/time.h"
#include "esphome/components/time/real_time_clock.h"
#endif

#include "config.h"
#include "entity.h"
#include "types.h"
#include "helpers.h"
#include "page_base.h"
#include "page_manager.h"
#include "card_base.h"
#include "pages.h"

namespace esphome {
namespace nspanel_lovelace {

PACK(struct NSPanelRestoreState {
  uint8_t display_active_dim_ = 100;
  uint8_t display_inactive_dim_ = 50;
});

// BEGIN WEATHER SLOT LOGIC
// Pure daily-forecast selection. Parsed entries carry a panel-local date key
// and weekday; no JSON, ESPHome or device types appear here so the same code
// is compiled by the firmware and the host renderer test.
struct WeatherForecastSlotData {
  std::string date_key;  // panel-local "YYYY-MM-DD"
  uint8_t weekday{7};     // 0=Sunday..6=Saturday, 7=invalid
  std::string condition;
  float temperature{0.0f};
  bool valid{false};
};

struct WeatherForecastOutputSlot {
  std::string date_key;
  uint8_t weekday{7};
  std::string condition;
  std::string temperature;
  bool filled{false};
};

static constexpr size_t WEATHER_FORECAST_SLOT_COUNT = 4;

inline bool is_weather_date_key_valid(const std::string &key) {
  if (key.size() != 10 || key[4] != '-' || key[7] != '-') return false;
  for (size_t i = 0; i < key.size(); ++i) {
    if (i == 4 || i == 7) continue;
    if (key[i] < '0' || key[i] > '9') return false;
  }
  return true;
}

// Portable UTC epoch from a parsed broken-down time (no timegm on ESP-IDF).
inline time_t weather_tm_to_utc_epoch(const tm &parsed) {
  const unsigned month = static_cast<unsigned>(parsed.tm_mon + 1);
  const int year = parsed.tm_year + 1900 - (month <= 2);
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy =
      (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 +
      static_cast<unsigned>(parsed.tm_mday) - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  const long long days =
      static_cast<long long>(era) * 146097 + static_cast<int>(doe) - 719468;
  return static_cast<time_t>(
      days * 86400 + parsed.tm_hour * 3600 + parsed.tm_min * 60 + parsed.tm_sec);
}

// Convert an HA ISO-8601 daily timestamp using ESPHome's configured timezone.
inline bool weather_datetime_to_local_day(const char *datetime, std::string &date_key,
                                          uint8_t &weekday) {
  if (datetime == nullptr) return false;
  tm parsed{};
  const char *end = strptime(datetime, "%Y-%m-%dT%H:%M:%S", &parsed);
  if (end == nullptr) return false;
  int offset_seconds = 0;
  if (*end == '.') {
    const char *fraction = ++end;
    end += strspn(end, "0123456789");
    if (end == fraction) return false;
  }
  if (*end == 'Z') {
    ++end;
  } else if (*end == '+' || *end == '-') {
    const int sign = *end == '+' ? 1 : -1;
    tm offset{};
    end = strptime(end + 1, "%H:%M", &offset);
    if (end == nullptr || offset.tm_hour > 23 || offset.tm_min > 59) return false;
    offset_seconds = sign * (offset.tm_hour * 3600 + offset.tm_min * 60);
  } else {
    return false;
  }
  if (*end != '\0') return false;
  const time_t utc = weather_tm_to_utc_epoch(parsed) - offset_seconds;
  if (utc == static_cast<time_t>(-1)) return false;
  const auto local = ESPTime::from_epoch_local(utc);
  char buffer[16] = {};
  snprintf(buffer, sizeof(buffer), "%04u-%02u-%02u", local.year, local.month, local.day_of_month);
  date_key = buffer;
  weekday = static_cast<uint8_t>(local.day_of_week - 1);
  return true;
}

inline size_t build_weather_forecast_slots(
    const std::vector<WeatherForecastSlotData> &entries,
    const std::string &today_date_key,
    std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> &out) {
  out = {};
  std::vector<const WeatherForecastSlotData *> valid;
  valid.reserve(entries.size());
  for (const auto &entry : entries) {
    if (!entry.valid || entry.weekday > 6 || entry.condition.empty() ||
        !is_weather_date_key_valid(entry.date_key) || std::isnan(entry.temperature))
      continue;
    // Expire days already past in panel-local time; a transient error must not
    // leave yesterday's forecast on the panel.
    if (!today_date_key.empty() && entry.date_key < today_date_key) continue;
    valid.push_back(&entry);
  }
  std::stable_sort(valid.begin(), valid.end(),
                   [](const auto *a, const auto *b) { return a->date_key < b->date_key; });
  const size_t count = std::min(valid.size(), WEATHER_FORECAST_SLOT_COUNT);
  for (size_t i = 0; i < count; ++i) {
    char buffer[16] = {};
    snprintf(buffer, sizeof(buffer), "%.1f", valid[i]->temperature);
    out[i].date_key = valid[i]->date_key;
    out[i].weekday = valid[i]->weekday;
    out[i].condition = valid[i]->condition;
    out[i].temperature = buffer;
    out[i].filled = true;
  }
  return count;
}
// END WEATHER SLOT LOGIC

class NSPanelLovelace : public Component, public uart::UARTDevice, public api::CustomAPIDevice {
public:
  NSPanelLovelace();
  void setup() override;
  void loop() override;

  std::shared_ptr<Entity> create_entity(const std::string &entity_id);

  template <class... TArgs>
  Screensaver* create_screensaver(TArgs&&... args) {
    if (this->screensaver_ == nullptr) {
      this->screensaver_ = this->page_mgr_
        .find_bookmarked_page<Screensaver>((uint8_t)render_page_option::screensaver_page);
      if (this->screensaver_ != nullptr)
        return this->screensaver_;
    }
    this->screensaver_ = this->page_mgr_
      .insert_page<Screensaver>(0, std::forward<TArgs>(args)...);
    // Ensure the page is skipped when cycling the pages
    this->screensaver_->set_hidden(true);
    this->screensaver_->set_on_item_added_callback(
      std::bind(&NSPanelLovelace::on_page_item_added_callback,
        this, std::placeholders::_1));
    return this->screensaver_;
  }

  template <typename TPage, typename... TArgs>
  TPage* create_page(TArgs&&... args) {
    static_assert(
      std::is_base_of<Page, TPage>::value,
      "TPage must derive from esphome::nspanel_lovelace::Page");
    TPage* page = this->page_mgr_.create_page<TPage>(std::forward<TArgs>(args)...);
    static_cast<Page*>(page)->set_on_item_added_callback(
      std::bind(&NSPanelLovelace::on_page_item_added_callback,
        this, std::placeholders::_1));
    return page;
  }

#ifdef USE_TIME
  void set_time_id(time::RealTimeClock *time_id) { this->time_id_ = time_id; }
  void set_date_format(const std::string &date_format) {
    if (date_format.empty()) return;
    this->date_format_ = date_format;
  }
  void set_time_format(const std::string &time_format) {
    if (time_format.empty()) return;
    this->time_format_ = time_format;
  }

  void update_date(const char *date_format = "") { this->update_datetime(datetime_mode::date, date_format); }
  void update_time(const char *time_format = "") { this->update_datetime(datetime_mode::time, "", time_format); }
  // for time formatting see: https://esphome.io/components/time/#strftime
  void update_datetime(const datetime_mode mode = datetime_mode::both, const char *date_format = "", const char *time_format = "");
#endif
  
  void on_page_item_added_callback(const std::shared_ptr<PageItem> &item);
  void set_language(const std::string &language) { this->language_ = language; }
  // Note: This can only be called after the requested page has been created.
  //       Returns true if sucessful, otherwise false.
  bool set_default_page(const std::string &uuid) {
    auto index = this->page_mgr_.find_page_index(uuid);
    if (index == SIZE_MAX) return false;
    return this->page_mgr_.bookmark_page(
      (uint8_t)render_page_option::default_page, index, true);
  }
  void set_display_timeout(uint16_t timeout);
  void set_display_active_dim(uint8_t active);
  void set_display_inactive_dim(uint8_t inactive);
  // Note: this can be used without parameters to update the display without changing the levels
  void set_display_dim(uint8_t inactive = UINT8_MAX, uint8_t active = UINT8_MAX);
#ifdef USE_NSPANEL_NATIVE_WEATHER
  void set_weather_forecast_action(uint8_t slot, api::HomeAssistantServiceCallAction<> *action) {
    this->weather_forecast_actions_[slot] = action;
  }
  void set_weather_forecast_stall_restart(uint32_t delay_ms) { this->weather_stall_restart_delay_ms_ = delay_ms; }
  void apply_weather_forecast(ArduinoJson::JsonObjectConst response, uint8_t slot);
  void handle_weather_forecast_error(const std::string &error, uint8_t slot);
  // Called from codegen after the screensaver items exist: the four forecast
  // slots start blank until the first accepted reply fills them.
  void init_weather_forecast_slots();
#endif
  void set_weather_entity_id(const std::string &weather_entity_id) { this->weather_entity_id_ = weather_entity_id; }

  bool get_double_tap_to_unlock() const { return this->double_tap_to_unlock_; }
  void set_double_tap_to_unlock(bool value) { this->double_tap_to_unlock_ = value; }
  void set_global_vertical_ui(bool val) { this->global_vertical_ui_ = val; }
  
  void render_screensaver_page() { this->render_page_(render_page_option::screensaver_page); }
  void render_next_page() { this->render_page_(render_page_option::next); }
  void render_previous_page() { this->render_page_(render_page_option::prev); }
  void render_default_page() { this->render_page_(render_page_option::default_page); }

  void notify_on_screensaver(const std::string &heading,
      const std::string &message, uint32_t timeout_ms = 0);

  void send_display_command(const std::string &command);
  /**
   * Softreset the Nextion
   */
  void soft_reset_display() {
    // this->send_nextion_command_("rest"); // only for stock FW
    gpio_set_level(GPIO_NUM_4, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(GPIO_NUM_4, 0);
  }

  float get_setup_priority() const override { return setup_priority::DATA; }

  void dump_config() override;

  void add_incoming_msg_callback(std::function<void(std::string)> callback) { this->incoming_msg_callback_.add(std::move(callback)); }

#ifdef TEST_DEVICE_MODE
  // Only used to simulate TFT commands on test devices
  void process_command(const std::string &message);
#endif

#ifdef USE_NSPANEL_TFT_UPLOAD
  /**
   * Upload the tft file and softreset the Nextion.
   * If the upload fails for any reason, a power cycle of the display and re-upload will be needed
   */
  bool upload_tft(const std::string &url);
#endif

protected:
  bool restore_state_();
  bool save_state_();
  ESPPreferenceObject pref_;

  void init_display_(int baud_rate);
#ifdef USE_NSPANEL_TFT_UPLOAD
  uint16_t recv_ret_string_(std::string &response, uint32_t timeout, bool recv_flag);
#endif
  void send_nextion_command_(const std::string &command);

  void subscribe_homeassistant_state_attr(
    void (NSPanelLovelace::*callback)(const std::string &, const std::string &, esphome::StringRef),
    const std::string &entity_id, const std::string &attr_name
  ) {
    auto f = std::bind(callback, this, entity_id, attr_name, std::placeholders::_1);
    api::global_api_server->
      subscribe_home_assistant_state(entity_id, optional<std::string>(attr_name), std::move(f));
  }

  bool process_data_();
  const std::string *try_replace_uuid_with_entity_id_(const std::string &internal_id);
  void process_command_(const std::string &message);
  void send_buffered_command_();
  void process_display_command_queue_();
  void process_button_press_(const std::string &entity_id,
    const std::string &button_type,
    const std::string &value = "", bool called_from_timeout = false);
  std::string get_mode_from_index_(const std::string &entity_id, ha_attr_type attr_type, const std::string &value);
  StatefulPageItem* get_current_page_item_(const std::string &uuid);
  StatefulPageItem* get_page_item_(const std::string &uuid);
  Entity* get_entity_(const std::string &entity_id);

  void render_page_(const std::string &uuid);
  void render_page_(render_page_option d);
  void render_current_page_();
  void render_item_update_();
  void render_item_update_(Page *page);
  void render_popup_notify_page_(const std::string &internal_id,
    const std::string &heading, const std::string &message, uint16_t timeout = 0U,
    const std::string &btn1_text = "", const std::string &btn2_text = "");
  void render_popup_page_(const std::string &detail, const std::string &internal_id);
  bool render_popup_page_update_(StatefulPageItem *entity, bool opening = false, bool input_select = false);
  void render_light_detail_update_(StatefulPageItem *entity);
  bool render_timer_detail_update_(StatefulPageItem *item);
  void render_cover_detail_update_(StatefulPageItem *item);
  void render_climate_detail_update_(StatefulPageItem *item);
  void render_climate_detail_update_(Entity *entity, const std::string &uuid = "");
  void render_input_select_detail_update_(StatefulPageItem *item);
  void render_fan_detail_update_(StatefulPageItem *item);

#ifdef USE_TIME
  void setup_time_();
  // Check and update clock if required
  void check_time_();
  optional<time::RealTimeClock *> time_id_{};
  std::string date_format_ = "%A, %d. %B %Y";
  std::string time_format_ = "%H:%M";
  uint8_t now_minute_, now_hour_;
  bool time_configured_;
#endif
  
  bool double_tap_to_unlock_ = false;

  uint8_t display_active_dim_ = 100;
  uint8_t display_inactive_dim_ = 50;
  
  void call_ha_service_(
    const std::string& service, const std::string& entity_id);
  void call_ha_service_(
    const char *entity_type, const std::string &action, const std::string& entity_id);
  void call_ha_service_(
    const char *entity_type, const std::string &action,
    const std::map<std::string, std::string> &data,
    const std::map<std::string, std::string> &data_template = {});
  void call_ha_service_(
    const std::string& service,
    const std::map<std::string, std::string> &data,
    const std::map<std::string, std::string> &data_template = {});
  void on_entity_state_update_(const std::string &entity_id, esphome::StringRef state);
  void on_entity_attribute_update_(
    const std::string &entity_id, const std::string &attr_name, esphome::StringRef attr_value);

  void on_weather_state_update_(const std::string &entity_id, esphome::StringRef state);
  void on_weather_temperature_update_(const std::string &entity_id, esphome::StringRef temperature);
  void on_weather_temperature_unit_update_(const std::string &entity_id, esphome::StringRef temperature_unit);
  void send_weather_update_command_();
#ifdef USE_NSPANEL_NATIVE_WEATHER
  void check_weather_forecast_();
  bool weather_forecast_slots_full_() const;
  void observe_weather_restart_ha_();
  bool weather_stall_restart_ready_() const;
  void check_weather_forecast_stall_();
  bool clear_weather_forecast_(uint8_t slot);
  void log_weather_forecast_diagnostic_(const char *reason);
  void publish_weather_slots_(const std::array<WeatherForecastOutputSlot, WEATHER_FORECAST_SLOT_COUNT> &slots);
  void expire_weather_slots_();
  std::string weather_today_date_key_();

  static constexpr uint8_t WEATHER_ACTION_SLOTS = 4;
  api::HomeAssistantServiceCallAction<> *weather_forecast_actions_[WEATHER_ACTION_SLOTS]{};
  bool weather_slot_used_[WEATHER_ACTION_SLOTS]{};
  uint8_t weather_last_slot_{WEATHER_ACTION_SLOTS - 1};
  uint8_t weather_pending_slot_{WEATHER_ACTION_SLOTS};
  uint32_t weather_next_fetch_ms_{0};
  uint32_t weather_response_deadline_ms_{0};
  uint32_t weather_last_diagnostic_ms_{0};
  bool weather_diagnostic_logged_{false};
  uint32_t weather_last_expire_ms_{0};
  uint8_t weather_unanswered_{0};
  bool weather_api_ready_{false};
  bool weather_in_flight_{false};
  uint32_t weather_stall_restart_delay_ms_{0};
  std::optional<uint64_t> weather_stall_restart_deadline_ms_;
  std::optional<uint64_t> weather_restart_ha_ready_since_ms_;
  bool weather_stall_restart_queued_{false};
#ifdef USE_NSPANEL_TFT_UPLOAD
  bool weather_stall_restart_committed_{false};
#endif
  std::string weather_forecast_json_;
  bool weather_forecast_valid_{false};
  std::string weather_slot_dates_[WEATHER_FORECAST_SLOT_COUNT];
#endif
  std::string weather_entity_id_;
  std::string language_;

  std::queue<std::string> command_queue_;
  unsigned long command_last_sent_ = 0;

  // Non-zero while an idle period longer than the TFT's 65 s limit is timed here.
  uint32_t esp_sleep_timeout_ms_{0};
  uint32_t esp_sleep_start_ms_{0};

  bool button_press_timeout_set_ = false;
  std::string button_press_uuid_;
  std::string button_press_type_;
  std::string button_press_value_;

  PageManager page_mgr_;
  std::string popup_page_current_uuid_;
  bool popup_is_input_select_ = false;
  bool force_current_page_update_ = false;
  bool global_vertical_ui_{false};
  std::vector<std::shared_ptr<Entity>> entities_;
  std::vector<std::shared_ptr<StatefulPageItem>> stateful_page_items_;
  Screensaver* screensaver_ = nullptr;
  StatefulPageItem* cached_page_item_ = nullptr;
  Entity* cached_entity_ = nullptr;

  CallbackManager<void(std::string)> incoming_msg_callback_;

  std::vector<uint8_t> buffer_;
  uint32_t rx_last_byte_ms_ = 0;
  std::string command_buffer_;

#ifdef USE_NSPANEL_TFT_UPLOAD
  uint32_t default_baud_rate_ = 0;
  uint32_t update_baud_rate_ = 921600;
  bool is_updating_ = false;
  bool reparse_mode_ = false;
  uint32_t content_length_ = 0;
  size_t tft_size_ = 0;
  bool upload_first_chunk_sent_ = false;
  int upload_by_chunks_(esp_http_client_handle_t http_client, uint32_t &range_start);
  bool upload_end_(bool successful);
#endif // USE_NSPANEL_TFT_UPLOAD
};

}  // namespace nspanel_lovelace
}  // namespace esphome
