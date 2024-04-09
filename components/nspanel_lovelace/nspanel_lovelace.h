#pragma once

#include "defines.h"

#include <memory>
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
#include "esphome/components/uart/uart_component_esp_idf.h"
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
#include "types.h"
#include "helpers.h"
#include "page_base.h"
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

#ifdef USE_TIME
  void set_time_id(time::RealTimeClock *time_id) { this->time_id_ = time_id; }
  void set_date_format(const std::string &date_format) { this->date_format_ = date_format; }
  void set_time_format(const std::string &time_format) { this->time_format_ = time_format; }

  void update_date(const char *date_format = "") { this->update_datetime(datetime_mode::date, date_format); }
  void update_time(const char *time_format = "") { this->update_datetime(datetime_mode::time, "", time_format); }
  // for time formatting see: https://esphome.io/components/time/#strftime
  void update_datetime(const datetime_mode mode = datetime_mode::both, const char *date_format = "", const char *time_format = "");
#endif
  
  void set_screensaver(const std::shared_ptr<Screensaver> &screensaver);
  void add_page(const std::shared_ptr<Page> &page, const size_t position = SIZE_MAX);
  void set_display_timeout(uint16_t timeout);
  void set_display_active_dim(uint8_t active);
  void set_display_inactive_dim(uint8_t inactive);
  // Note: this can be used without parameters to update the display without changing the levels
  void set_display_dim(uint8_t inactive = UINT8_MAX, uint8_t active = UINT8_MAX);
  void set_weather_entity_id(const std::string &weather_entity_id) { this->weather_entity_id_ = weather_entity_id; }
  void set_day_of_week_override(DayOfWeekMap::dow dow, const std::array<const char *, 2> &value);

  void send_display_command(const char *command);
  /**
   * Softreset the Nextion
   */
  void soft_reset_display() { this->send_nextion_command_("rest"); }

  float get_setup_priority() const override { return setup_priority::DATA; }

  void dump_config() override;

  void add_incoming_msg_callback(std::function<void(std::string)> callback) { this->incoming_msg_callback_.add(std::move(callback)); }

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
  void start_reparse_mode_();
  void exit_reparse_mode_();
#endif
  void send_nextion_command_(const std::string &command);

  void add_page_item(const std::shared_ptr<PageItem> &item);

  bool process_data_();
  size_t find_page_index_by_uuid_(const std::string &uuid) const;
  const std::string &try_replace_uuid_with_entity_id_(const std::string &uuid_or_entity_id);
  void process_command_(const std::string &message);
  void send_buffered_command_();
  void process_display_command_queue_();
  void process_button_press_(std::string &entity_id, const std::string &button_type, const std::string &value = "", bool called_from_timeout = false);
  StatefulPageItem* get_entity_by_id_(const std::string &entity_id);
  StatefulPageItem* get_entity_by_uuid_(const std::string &uuid);

  void render_page_(size_t index);
  void render_page_(render_page_option d);
  void render_current_page_();
  void render_item_update_(Page *page);
  void render_popup_page_(const std::string &internal_id);
  void render_popup_page_update_(const std::string &internal_id);
  void render_popup_page_update_(StatefulPageItem *entity);
  void render_light_detail_update_(StatefulPageItem *entity);

#ifdef USE_TIME
  void setup_time_();
  // Check and update clock if required
  void check_time_();
  optional<time::RealTimeClock *> time_id_{};
  std::string date_format_, time_format_;
  uint8_t now_minute_, now_hour_;
  bool time_configured_;
#endif

  uint8_t display_active_dim_ = 100;
  uint8_t display_inactive_dim_ = 50;
  
  void call_ha_service_(const char* entity_type, const char* action, const std::string& entity_id);
  void call_ha_service_(const char* entity_type, const char* action, const std::map<std::string, std::string> &data);
  void call_ha_service_(const char* entity_type, const char* action, 
      const std::map<std::string, std::string> &data, const std::map<std::string, std::string> &data_template);
  void on_entity_state_update_(std::string entity_id, std::string state);
  void on_entity_attr_unit_of_measurement_update_(std::string entity_id, std::string unit_of_measurement);
  void on_entity_attr_device_class_update_(std::string entity_id, std::string device_class);
  void on_entity_attr_supported_color_modes_update_(std::string entity_id, std::string supported_color_modes);
  void on_entity_attr_color_mode_update_(std::string entity_id, std::string color_mode);
  void on_entity_attr_brightness_update_(std::string entity_id, std::string brightness);
  void on_entity_attr_color_temp_update_(std::string entity_id, std::string color_temp);
  void on_entity_attr_min_mireds_update_(std::string entity_id, std::string min_mireds);
  void on_entity_attr_max_mireds_update_(std::string entity_id, std::string max_mireds);
  void on_entity_attribute_update_(const std::string &entity_id, const char *attr_name, const std::string &attr_value);

  void on_weather_state_update_(std::string entity_id, std::string state);
  void on_weather_temperature_update_(std::string entity_id, std::string temperature);
  void on_weather_temperature_unit_update_(std::string entity_id, std::string temperature_unit);
  void on_weather_forecast_update_(std::string entity_id, std::string forecast_json);
  void send_weather_update_command_();
  std::string weather_entity_id_;
  DayOfWeekMap day_of_week_map_;
  bool day_of_week_map_overridden_;

  std::queue<std::string> command_queue_;
  unsigned long command_last_sent_;
  unsigned int update_baud_rate_ = 921600;

  bool button_press_timeout_set_;
  std::string button_press_uuid_;
  std::string button_press_type_;
  std::string button_press_value_;

  uint8_t current_page_index_ = 0;
  std::string popup_page_current_uuid_;
  Page* current_page_ = nullptr;
  Screensaver* screensaver_ = nullptr;
  std::vector<std::shared_ptr<Page>> pages_;
  std::vector<std::shared_ptr<PageItem>> page_items_;
  std::vector<std::shared_ptr<StatefulPageItem>> stateful_page_items_;
  StatefulPageItem* cached_page_item_ = nullptr;

  CallbackManager<void(std::string)> incoming_msg_callback_;

  std::vector<uint8_t> buffer_;
  std::string command_buffer_;

#ifdef USE_NSPANEL_TFT_UPLOAD
  bool is_updating_ = false;
  bool reparse_mode_ = false;
  int content_length_ = 0;
  int tft_size_ = 0;
#ifdef USE_ESP_IDF
  int upload_range_(const std::string &url, int range_start);
#else // USE_ARDUINO
  uint8_t *transfer_buffer_ = nullptr;
  size_t transfer_buffer_size_;
  bool upload_first_chunk_sent_ = false;
  int upload_by_chunks_(HTTPClient *http, const std::string &url, int range_start);
#endif
  bool upload_end_(bool successful);
#endif // USE_NSPANEL_TFT_UPLOAD
};

}  // namespace nspanel_lovelace
}  // namespace esphome