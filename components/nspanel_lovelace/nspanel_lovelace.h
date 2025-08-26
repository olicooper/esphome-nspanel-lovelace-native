#pragma once

#include "defines.h"

#include <functional>
#include <memory>
#include <map>
#include <queue>
#include <stdint.h>
#include <utility>
#include <vector>

#include "esphome/core/defines.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/preferences.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/api/custom_api_device.h"

#ifdef USE_ESP_IDF
#include <driver/gpio.h>
#include "esphome/components/uart/uart_component_esp_idf.h"
#ifdef USE_NSPANEL_TFT_UPLOAD
#include <esp_http_client.h>
#endif
#else
#ifdef USE_NSPANEL_TFT_UPLOAD
#include <HTTPClient.h>
#endif
#include "esphome/components/uart/uart_component_esp32_arduino.h"
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

class NSPanelLovelace : public Component, public uart::UARTDevice, protected api::CustomAPIDevice {
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
  void set_weather_entity_id(const std::string &weather_entity_id) { this->weather_entity_id_ = weather_entity_id; }

  bool get_double_tap_to_unlock() const { return this->double_tap_to_unlock_; }
  void set_double_tap_to_unlock(bool value) { this->double_tap_to_unlock_ = value; }
  
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
#ifdef USE_ESP_IDF
    gpio_set_level(GPIO_NUM_4, 1);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(GPIO_NUM_4, 0);
#else
    digitalWrite(4, 1);
    delay(1000);
    digitalWrite(4, 0);
#endif
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
#ifdef USE_ARDUINO
  void set_reparse_mode_(bool active);
#endif
#endif
  void send_nextion_command_(const std::string &command);

  void subscribe_homeassistant_state_attr(
      void (NSPanelLovelace::*callback)(std::string, std::string, std::string),
      std::string entity_id, std::string attribute) {
    auto f = std::bind(callback, this, entity_id, attribute, std::placeholders::_1);
    api::global_api_server->
      subscribe_home_assistant_state(entity_id, optional<std::string>(attribute), f);
  }

  bool process_data_();
  const std::string &try_replace_uuid_with_entity_id_(const std::string &uuid_or_entity_id);
  void process_command_(const std::string &message);
  void send_buffered_command_();
  void process_display_command_queue_();
  void process_button_press_(std::string &entity_id,
    const std::string &button_type,
    const std::string &value = "", bool called_from_timeout = false);
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
  void render_popup_page_(const std::string &internal_id);
  bool render_popup_page_update_(const std::string &internal_id);
  bool render_popup_page_update_(StatefulPageItem *entity);
  void render_light_detail_update_(StatefulPageItem *entity);
  void render_timer_detail_update_(StatefulPageItem *entity);
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
  void on_entity_state_update_(std::string entity_id, std::string state);
  void on_entity_attribute_update_(
    std::string entity_id, std::string attr_name, std::string attr_value);

  void on_weather_state_update_(std::string entity_id, std::string state);
  void on_weather_temperature_update_(std::string entity_id, std::string temperature);
  void on_weather_temperature_unit_update_(std::string entity_id, std::string temperature_unit);
  void on_weather_forecast_update_(std::string entity_id, std::string forecast_json);
  void send_weather_update_command_();
  std::string weather_entity_id_;
  std::string language_;

  std::queue<std::string> command_queue_;
  unsigned long command_last_sent_ = 0;

  bool button_press_timeout_set_ = false;
  std::string button_press_uuid_;
  std::string button_press_type_;
  std::string button_press_value_;

  PageManager page_mgr_;
  std::string popup_page_current_uuid_;
  bool force_current_page_update_ = false;
  std::vector<std::shared_ptr<Entity>> entities_;
  std::vector<std::shared_ptr<StatefulPageItem>> stateful_page_items_;
  Screensaver* screensaver_ = nullptr;
  StatefulPageItem* cached_page_item_ = nullptr;
  Entity* cached_entity_ = nullptr;

  CallbackManager<void(std::string)> incoming_msg_callback_;

  std::vector<uint8_t> buffer_;
  std::string command_buffer_;

#ifdef USE_NSPANEL_TFT_UPLOAD
  uint32_t default_baud_rate_ = 0;
  uint32_t update_baud_rate_ = 115200;
  bool is_updating_ = false;
  bool reparse_mode_ = false;
  uint32_t content_length_ = 0;
  size_t tft_size_ = 0;
  bool upload_first_chunk_sent_ = false;
#ifdef USE_ESP_IDF
  int upload_by_chunks_(esp_http_client_handle_t http_client, uint32_t &range_start);
#else // USE_ARDUINO
  uint8_t *transfer_buffer_ = nullptr;
  size_t transfer_buffer_size_;
  int upload_by_chunks_(HTTPClient *http, const std::string &url, uint32_t &range_start);
#endif
  bool upload_end_(bool successful);
#endif // USE_NSPANEL_TFT_UPLOAD
};

}  // namespace nspanel_lovelace
}  // namespace esphome
