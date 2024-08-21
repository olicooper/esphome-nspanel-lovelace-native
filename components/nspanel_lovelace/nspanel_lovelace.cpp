#include "nspanel_lovelace.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <time.h>
#include <vector>
#include <utility>
#ifdef USE_ESP_IDF
#include <driver/gpio.h>
#endif
#include <esp_heap_caps.h>
// #include <esp32/rom/rtc.h>
#include <esp_system.h>
// #include <regex>
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/components/json/json_util.h"

#include "cards.h"
#include "card_items.h"
#include "pages.h"
#include "page_item_visitor.h"
#include "page_visitor.h"
#include "translations.h"

namespace esphome {
namespace nspanel_lovelace {

// Use PSRAM for ArduinoJson (if available, otherwise use normal malloc)
// see: https://arduinojson.org/v6/how-to/use-external-ram-on-esp32/#how-to-use-the-psram-with-arduinojson
struct SpiRamAllocator {
  void* allocate(size_t size) {
   if (psram_available())
     return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
   else
     return malloc(size);
  }

  void deallocate(void* pointer) {
    if (psram_available())
      heap_caps_free(pointer);
    else
      return free(pointer);
  }

  void* reallocate(void* ptr, size_t new_size) {
    if (psram_available())
      return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
    else
      return realloc(ptr, new_size);
  }
};
using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;

static const char *const TAG = "nspanel_lovelace";

NSPanelLovelace::NSPanelLovelace() {
  command_buffer_.reserve(1024);
}

bool NSPanelLovelace::restore_state_() {
  NSPanelRestoreState recovered{};
  this->pref_ = global_preferences->make_preference<NSPanelRestoreState>(/*this->get_object_id_hash() ^ */RESTORE_STATE_VERSION);
  bool restored = this->pref_.load(&recovered);
  if (restored) {
    this->display_active_dim_ = recovered.display_active_dim_;
    this->display_inactive_dim_ = recovered.display_inactive_dim_;
  }
  return restored;
}
bool NSPanelLovelace::save_state_() {
  NSPanelRestoreState state{};
  state.display_active_dim_ = this->display_active_dim_;
  state.display_inactive_dim_ = this->display_inactive_dim_;
  return this->pref_.save(&state);
}

void NSPanelLovelace::setup() {
  this->restore_state_();

#ifdef USE_TIME
  this->setup_time_();
#endif
  // The display isn't reset when ESP is reset (on ota update etc.)
  // so we need to simulate the display 'startup' instead
  // see: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/misc_system_api.html#_CPPv418esp_reset_reason_t
  auto reason = esp_reset_reason();
  if (reason == esp_reset_reason_t::ESP_RST_SW ||
      reason == esp_reset_reason_t::ESP_RST_DEEPSLEEP/* ||
      reason == esp_reset_reason_t::ESP_RST_USB*/) {
#ifdef TEST_DEVICE_MODE
    this->process_command("event,startup,53,eu");
#elif defined(USE_ESP_IDF)
    gpio_set_level(GPIO_NUM_4, 1);
    delay(1000);
    gpio_set_level(GPIO_NUM_4, 0);
#else
    digitalWrite(GPIO4, 1);
    delay(1000);
    digitalWrite(GPIO4, 0);
#endif
  }

  // todo: create entity for weather instead, so others can subscribe
  if (!this->weather_entity_id_.empty()) {
    // state provides the information for the icon
    this->subscribe_homeassistant_state(
        &NSPanelLovelace::on_weather_state_update_, this->weather_entity_id_);
    this->subscribe_homeassistant_state(
        &NSPanelLovelace::on_weather_temperature_update_,
        this->weather_entity_id_, to_string(ha_attr_type::temperature));
    this->subscribe_homeassistant_state(
        &NSPanelLovelace::on_weather_temperature_unit_update_,
        this->weather_entity_id_, to_string(ha_attr_type::temperature_unit));
    this->subscribe_homeassistant_state(
        &NSPanelLovelace::on_weather_forecast_update_, this->weather_entity_id_,
        to_string(ha_attr_type::forecast));
  }
  
  for (auto &entity : this->entities_) {
    auto &entity_id = entity->get_entity_id();
    ESP_LOGV(TAG, "Adding subscriptions for entity '%s'", entity_id.c_str());
    bool add_state_subscription = false;
    if (entity->is_type(entity_type::light)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::supported_color_modes));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::color_mode));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::min_mireds));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::max_mireds));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::color_temp));
      // need to subscribe to brightness to know if brightness is supported
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::brightness));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::effect_list));
    }
    else if (entity->is_type(entity_type::switch_) ||
        entity->is_type(entity_type::input_boolean) ||
        entity->is_type(entity_type::input_text) ||
        entity->is_type(entity_type::text) ||
        entity->is_type(entity_type::automation) ||
        entity->is_type(entity_type::sun) ||
        entity->is_type(entity_type::vacuum) ||
        entity->is_type(entity_type::lock) ||
        entity->is_type(entity_type::person)) {
      add_state_subscription = true;
    }
    // icons and unit_of_measurement based on state and device_class
    else if (entity->is_type(entity_type::sensor) ||
        entity->is_type(entity_type::binary_sensor)) {
      add_state_subscription = true;
      // if (!entity->is_icon_value_overridden()) {
        this->subscribe_homeassistant_state_attr(
            &NSPanelLovelace::on_entity_attribute_update_, 
            entity_id, to_string(ha_attr_type::device_class));
      // }
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::unit_of_measurement));
    }
    else if (entity->is_type(entity_type::cover)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::device_class));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::supported_features));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::current_position));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::current_tilt_position));
    }
    else if (entity->is_type(entity_type::alarm_control_panel)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::code_arm_required));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::open_sensors));
    }
    else if (entity->is_type(entity_type::timer)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::editable));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::duration));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::remaining));
      this->subscribe_homeassistant_state_attr(
        &NSPanelLovelace::on_entity_attribute_update_,
        entity_id, to_string(ha_attr_type::finishes_at));
    }
    else if (entity->is_type(entity_type::climate)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::temperature));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::current_temperature));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::target_temp_high));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::target_temp_low));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::target_temp_step));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::min_temp));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::max_temp));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::hvac_action));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::preset_modes));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::swing_modes));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::fan_modes));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::hvac_modes));
    }
    else if (entity->is_type(entity_type::media_player)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::supported_features));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::media_content_type));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::media_title));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::media_artist));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::volume_level));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::shuffle));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::source_list));
    }
    else if (entity->is_type(entity_type::select) ||
        entity->is_type(entity_type::input_select)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::options));
    }
    else if (entity->is_type(entity_type::number) ||
        entity->is_type(entity_type::input_number)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::min));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::max));
    }
    else if (entity->is_type(entity_type::weather)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::temperature));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::temperature_unit));
    }
    else if (entity->is_type(entity_type::fan)) {
      add_state_subscription = true;
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::percentage_step));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::percentage));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::preset_modes));
      this->subscribe_homeassistant_state_attr(
          &NSPanelLovelace::on_entity_attribute_update_, 
          entity_id, to_string(ha_attr_type::preset_mode));
    }

    if (add_state_subscription) {
      this->subscribe_homeassistant_state(
        &NSPanelLovelace::on_entity_state_update_,
        entity_id);
    }
  }
}

void NSPanelLovelace::loop() {
#ifdef USE_NSPANEL_TFT_UPLOAD
  if (this->is_updating_ || this->reparse_mode_) {
    return;
  }
#endif

  // Monitor for commands arriving from the screen over UART
  uint8_t d;
  while (this->available()) {
    this->read_byte(&d);
    this->buffer_.push_back(d);
    if (!this->process_data_()) {
      ESP_LOGW(TAG, "Unparsed data: %s", esphome::format_hex(this->buffer_).c_str());
      this->buffer_.clear();
    }
  }

  if (this->force_current_page_update_) {
    this->force_current_page_update_ = false;
    ESP_LOGD(TAG, "Render HA update");
    if (this->popup_page_current_uuid_.empty()) {
      this->render_item_update_(this->current_page_);
    } else {
      this->render_popup_page_update_(this->cached_page_item_);
    }
  }

  // Throttle command processing to avoid flooding the display with commands
  if ((millis() - this->command_last_sent_) > COMMAND_COOLDOWN) {
    this->process_display_command_queue_();
  }
}

std::shared_ptr<Entity> NSPanelLovelace::create_entity(const std::string &entity_id) {
  for (auto &e : this->entities_) {
    if (entity_id == e->get_entity_id()) return e;
  }
  auto entity = std::make_shared<Entity>(entity_id);
  this->entities_.push_back(entity);
  return entity;
}

void NSPanelLovelace::on_page_item_added_callback(const std::shared_ptr<PageItem> &item) {
  bool found = false;
  auto &item_uuid = item->get_uuid();

  if (page_item_cast<StatefulPageItem>(item.get())) {
    for (auto &item : this->stateful_page_items_) {
      if (item->get_uuid() == item_uuid) {
        found = true;
        break;
      }
    }
    if (!found) {
      auto& stateful_item = (const std::shared_ptr<StatefulPageItem>&)item;
      this->stateful_page_items_.push_back(stateful_item);
      ESP_LOGV(TAG, "Adding stateful item uuid.%s %s", 
        item_uuid.c_str(),
        stateful_item->get_entity_id().c_str());
    }
  }
}

void NSPanelLovelace::set_display_timeout(uint16_t timeout) {
  this->command_buffer_
    .assign("timeout").append(1, SEPARATOR)
    .append(esphome::to_string(timeout));
  this->send_buffered_command_();
}

void NSPanelLovelace::set_display_active_dim(uint8_t active) {
  this->set_display_dim(UINT8_MAX, active);
}
void NSPanelLovelace::set_display_inactive_dim(uint8_t inactive) {
  this->set_display_dim(inactive);
}
void NSPanelLovelace::set_display_dim(uint8_t inactive, uint8_t active) {
  bool save_state = false;

  if (active != UINT8_MAX) {
    if (active > 100) {
      active = 100;
    }
    save_state = this->display_active_dim_ != active;
    this->display_active_dim_ = active;
  }

  if (inactive != UINT8_MAX) {
    if (inactive >= active) {
      // inactive must be less than active otherwise the Nextion display breaks
      inactive = active - 1;
    }
    save_state = save_state || this->display_inactive_dim_ != inactive;
    this->display_inactive_dim_ = inactive;
  }

  if (save_state) this->save_state_();
  
  this->command_buffer_
    .assign("dimmode").append(1, SEPARATOR)
    // brightness when inactive (after timeout reached)
    .append(esphome::to_string(this->display_inactive_dim_)).append(1, SEPARATOR)
    // brightness when active (when buttons pressed)
    .append(esphome::to_string(this->display_active_dim_)).append(1, SEPARATOR)
    // background colour when active (not screensaver background, defaults to ha-dark)
    .append(esphome::to_string(6371));
  
  this->send_buffered_command_();
}

void NSPanelLovelace::set_day_of_week_override(DayOfWeekMap::dow dow, const std::array<const char *, 2> &value) {
  assert(dow < 7);
  switch(dow) {
  case DayOfWeekMap::dow::sunday:
    this->day_of_week_map_.set_sunday(value);
    break;
  case DayOfWeekMap::dow::monday:
    this->day_of_week_map_.set_monday(value);
    break;
  case DayOfWeekMap::dow::tuesday:
    this->day_of_week_map_.set_tuesday(value);
    break;
  case DayOfWeekMap::dow::wednesday:
    this->day_of_week_map_.set_wednesday(value);
    break;
  case DayOfWeekMap::dow::thursday:
    this->day_of_week_map_.set_thursday(value);
    break;
  case DayOfWeekMap::dow::friday:
    this->day_of_week_map_.set_friday(value);
    break;
  case DayOfWeekMap::dow::saturday:
    this->day_of_week_map_.set_saturday(value);
    break;
  }
}

bool NSPanelLovelace::process_data_() {
  uint32_t at = this->buffer_.size() - 1;
  auto *data = &this->buffer_[0];
  uint8_t new_byte = data[at];

  // Nextion Startup event
  // todo: store 'tft_connected' state?
  if (data[0] == 0x0) {
    if (at > 5) return false;
    static constexpr uint8_t seq[] = {0x00,0x00,0x00,0xFF,0xFF,0xFF};
    if (at == 5 && data[at] == seq[at]) {
      ESP_LOGD(TAG, "Nextion started");
      this->buffer_.clear();
    }
    return data[at] == seq[at];
  }
  // Nextion Ready event
  // note: This event can be removed by custom firmware and may never occur
  if (data[0] == 0x88) {
    if (at > 3) return false;
    static constexpr uint8_t seq[] = {0x88,0xFF,0xFF,0xFF};
    if (at == 3 && data[at] == seq[at]) {
      ESP_LOGD(TAG, "Nextion ready");
      this->buffer_.clear();
    }
    return data[at] == seq[at];
  }

  // Byte 0: HEADER1 (always 0x55)
  if (at == 0)
    return new_byte == 0x55;
  // Byte 1: HEADER2 (always 0xBB)
  if (at == 1)
    return new_byte == 0xBB;

  // Byte 3 & 4 - length (little endian)
  if (at == 2 || at == 3) {
    return true;
  }
  uint16_t length = encode_uint16(data[3], data[2]);

  // Wait until all data comes in
  if (at - 4 < length) {
    //    ESP_LOGD(TAG, "Message (%d/%d): 0x%02x", at - 3, length, new_byte);
    return true;
  }

  // Last two bytes: CRC; return after first one
  if (at == 4 + length) {
    return true;
  }

  uint16_t crc16 = encode_uint16(data[4 + length + 1], data[4 + length]);
  uint16_t calculated_crc16 = esphome::crc16(data, 4 + length);

  if (crc16 != calculated_crc16) {
    ESP_LOGW(TAG, "Received invalid message checksum %02X!=%02X", crc16, calculated_crc16);
    return false;
  }

  const uint8_t *message_data = data + 4;
  std::string message(message_data, message_data + length);

  this->process_command_(message);
  this->buffer_.clear();
  return true;
}

#ifdef TEST_DEVICE_MODE
void NSPanelLovelace::process_command(const std::string &message) {
  this->process_command_(message);
};
#endif

void NSPanelLovelace::process_command_(const std::string &message) {
  ESP_LOGD(TAG, "TFT CMD IN: %s", message.c_str());

  std::vector<std::string> tokens;
  tokens.reserve(5);
  split_str(',', message, tokens);
  if (tokens.size() < 2 || tokens.at(0) != "event") { return; }

  // note: from luibackend/mqtt.py
  if (tokens.at(1) == action_type::buttonPress2) {
    if (tokens.size() == 5) {
      this->process_button_press_(tokens.at(2), tokens.at(3), tokens.at(4));
    } else if (tokens.size() == 4) {
      this->process_button_press_(tokens.at(2), tokens.at(3));
    }
  } else if (tokens.at(1) == action_type::pageOpenDetail) {
    this->render_popup_page_(tokens.at(3));
  } else if (tokens.at(1) == action_type::sleepReached) {
    //std::string page = tokens.at(2);

    // todo: temporary, render default page instead
    this->render_page_(render_page_option::screensaver);
  } else if (tokens.at(1) == action_type::startup) {
    if (tokens.size() == 4) {
      uint16_t ver = 0;
      if(std::sscanf(tokens.at(2).c_str(), "%" PRIu16, &ver) == 1) {
        Configuration::set_version(ver);
      }
      Configuration::set_model(tokens.at(3));
    }
    if (Configuration::get_model() == nspanel_model_t::unknown) {
      ESP_LOGW(TAG, "Unknown NSPanel model!");
    }
    if (Configuration::get_version() == 0) {
      ESP_LOGW(TAG, "Unknown NSPanel version!");
    }
    // restore dimmode state
    this->set_display_dim();
    this->render_page_(render_page_option::screensaver);
#ifdef USE_TIME
    // If the TFT is reset then the time needs reconfiguring
    if (this->time_configured_) {
      this->update_datetime();
    }
#endif
  }

  this->incoming_msg_callback_.call(message);
}

void NSPanelLovelace::render_page_(size_t index) {
  if (index > this->pages_.size() - 1) return;
  this->current_page_index_ = index;
  this->current_page_ = this->pages_.at(index).get();
  this->force_current_page_update_ = false;
  this->render_current_page_();
}

void NSPanelLovelace::render_page_(render_page_option d) {
  uint8_t start_page_index = 1;
  if (d == render_page_option::default_page) {
    // todo: fetch default page from config
    this->current_page_index_ = start_page_index;
  } if (d == render_page_option::screensaver) {
    this->current_page_index_ = this->screensaver_ == nullptr ? start_page_index : 0;
  } else if (d == render_page_option::next) {
    if (this->current_page_index_ == this->pages_.size() - 1)
      this->current_page_index_ = start_page_index;
    else 
      ++this->current_page_index_;
  } else if (d == render_page_option::prev) {
    if (this->current_page_index_ <= start_page_index)
      this->current_page_index_ = this->pages_.size() - 1;
    else
      --this->current_page_index_;
  }
  this->current_page_ = this->pages_.at(this->current_page_index_).get();
  this->force_current_page_update_ = false;
  this->render_current_page_();
}

void NSPanelLovelace::render_current_page_() {
  if (this->current_page_ == nullptr)
    this->render_page_(render_page_option::default_page);

  this->command_buffer_.assign("pageType")
      .append(1, SEPARATOR)
      .append(this->current_page_->get_render_type_str());
  this->send_buffered_command_();
  this->popup_page_current_uuid_.clear();

  this->set_display_timeout(this->current_page_->get_sleep_timeout());
  
  this->render_item_update_(this->current_page_);
}

void NSPanelLovelace::render_item_update_(Page *page) {
  page->render(this->command_buffer_);
  this->send_buffered_command_();

  if (page->is_type(page_type::screensaver) && this->screensaver_ != nullptr) {
    if (this->screensaver_->should_render_status_update()) {
      this->screensaver_->render_status_update(this->command_buffer_);
      this->send_buffered_command_();
    }
  }
}

// entityUpdateDetail~{internalName}~{tHeading}~{tHeadingColor}~{b1}~{tB1Color}~{b2}~[tB2Color}~{tText}~{tTextColor}~{sleepTimeout}~{font}~{alt_icon}~{altIconColor}
// Possible 'font' options:
//    Font 0 - Default - Size 24 (No Icons, Support for various special chars from different langs)
//    Font 1 - Size 32 (Icons and limited chars)
//    Font 2 - Size 32 (No Icons, Support for various special chars from different langs)
//    Font 3 - Size 48 (Icons and limited chars)
//    Font 4 - Size 80 (Icons and limited chars)
//    Font 5 - Size 128 (ascii only)
void NSPanelLovelace::render_popup_notify_page_(const std::string &internal_id,
    const std::string &heading, const std::string &message, uint16_t timeout,
    const std::string &btn1_text, const std::string &btn2_text) {

  this->command_buffer_.assign("pageType")
      .append(1, SEPARATOR).append("popupNotify");
  this->send_buffered_command_();

  auto text_colour = std::to_string(65535U);
  this->command_buffer_
    .assign("entityUpdateDetail").append(1, SEPARATOR)
    .append(internal_id).append(1, SEPARATOR)
    // heading
    .append(heading).append(1, SEPARATOR)
    .append(text_colour).append(1, SEPARATOR)
    // 'no' button
    .append(btn1_text).append(1, SEPARATOR)
    .append(text_colour).append(1, SEPARATOR)
    // 'yes' button
    .append(btn2_text).append(1, SEPARATOR)
    .append(text_colour).append(1, SEPARATOR)
    // message
    .append(message).append(1, SEPARATOR)
    .append(text_colour).append(1, SEPARATOR)
    // timeout
    .append(std::to_string(timeout));

  this->send_buffered_command_();
}

void NSPanelLovelace::render_popup_page_(const std::string &internal_id) {
  if (this->current_page_ == nullptr) return;
  if (!this->render_popup_page_update_(internal_id)) return;
  this->set_display_timeout(10);
}

bool NSPanelLovelace::render_popup_page_update_(const std::string &internal_id) {
  if (this->current_page_ == nullptr) return false;

  // Sometimes a StatefulPageItem does not exist for an entity,
  // handle this edge case. Only certain cards support this.
  if (!esphome::str_startswith(internal_id, entity_type::uuid)) {
    auto entity = this->get_entity_(internal_id);
    if (entity == nullptr) {
      ESP_LOGW(TAG, "[popup] entity not found '%s'", internal_id.c_str());
      return false;
    }
    bool rendered = false;
    if (this->current_page_->is_type(page_type::cardThermo)) {
      if (entity->is_type(entity_type::climate)) {
        this->render_climate_detail_update_(entity);
        rendered = true;
      }
    }
    if (rendered) this->send_buffered_command_();
    return rendered;
  }

  auto uuid = internal_id.substr(5);

  if (this->cached_page_item_ == nullptr || this->cached_page_item_->get_uuid() != uuid) {
    if (this->current_page_->get_items().size() == 0) return false;
    // Only search for items in the current page to reduce processing time
    for (auto &item : this->current_page_->get_items()) {
      if (item->get_uuid() != uuid) continue;
      if (auto page_item = page_item_cast<StatefulPageItem>(item.get())) {
        this->cached_page_item_ = page_item;
        break;
      } else {
        ESP_LOGW(TAG, "[popup] item cast failed");
        return false;
      }
    }
  }

  if (this->cached_page_item_ == nullptr || this->cached_page_item_->get_uuid() != uuid) {
    ESP_LOGW(TAG, "[popup] entity not found on page '%s'", internal_id.c_str());
    return false;
  }
  
  this->popup_page_current_uuid_ = uuid;
  return this->render_popup_page_update_(this->cached_page_item_);
}

bool NSPanelLovelace::render_popup_page_update_(StatefulPageItem *item) {
  if (item == nullptr) return false;

  if (item->is_type(entity_type::light)) {
    this->render_light_detail_update_(item);
  } else if (item->is_type(entity_type::timer)) {
    this->set_display_timeout(30);
    this->render_timer_detail_update_(item);
    this->set_interval(entity_type::timer, 1000, [this, item]() {
      if (this->popup_page_current_uuid_ != item->get_uuid()) {
        this->cancel_interval(entity_type::timer);
        return;
      }
      this->render_timer_detail_update_(item);
    });
  } else if (item->is_type(entity_type::cover)) {
    this->render_cover_detail_update_(item);
  } else if (item->is_type(entity_type::climate)) {
    this->render_climate_detail_update_(item);
  } else if (
      item->is_type(entity_type::select) ||
      item->is_type(entity_type::input_select) ||
      item->is_type(entity_type::media_player)) {
    this->render_input_select_detail_update_(item);
  } else if (item->is_type(entity_type::fan)) {
    this->render_fan_detail_update_(item);
  } else {
    return false;
  }

  this->send_buffered_command_();
  return true;
}

// entityUpdateDetail~{entity_id}~{pos}~{pos_translation}: {pos_status}~{pos_translation}~{icon_id}~{icon_up}~{icon_stop}~{icon_down}~{icon_up_status}~{icon_stop_status}~{icon_down_status}~{textTilt}~{iconTiltLeft}~{iconTiltStop}~{iconTiltRight}~{iconTiltLeftStatus}~{iconTiltStopStatus}~{iconTiltRightStatus}~{tilt_pos}"
void NSPanelLovelace::render_cover_detail_update_(StatefulPageItem *item) {
  if(item == nullptr) return;

  auto entity = item->get_entity();

  std::array<const char *, 4> cover_icons{};
  bool cover_icons_found = try_get_value(COVER_MAP,
    cover_icons,
    entity->get_attribute(ha_attr_type::device_class),
    entity_cover_type::window);

  auto &position_str = entity->
    get_attribute(ha_attr_type::current_position);
  auto &supported_features_str = entity->
    get_attribute(ha_attr_type::supported_features);
  auto &tilt_position_str = entity->
    get_attribute(ha_attr_type::current_tilt_position);

  uint8_t position = value_or_default(position_str, 0U);
  uint8_t tilt_position = value_or_default(entity->
    get_attribute(ha_attr_type::current_tilt_position), 0U);
  uint16_t supported_features = value_or_default(entity->
    get_attribute(ha_attr_type::supported_features), 0U);

  // Icons
  const char* cover_icon = generic_type::empty;
  const char* icon_up   = generic_type::empty;
  const char* icon_stop = generic_type::empty;
  const char* icon_down = generic_type::empty;
  const char* icon_tilt_left   = generic_type::empty;
  const char* icon_tilt_stop = generic_type::empty;
  const char* icon_tilt_right = generic_type::empty;

  std::string text_position = "";
  std::string text_tilt = "";

  // Icon Status
  bool icon_up_status = false;
  bool icon_stop_status = false;
  bool icon_down_status = false;
  bool position_status = false;
  bool icon_tilt_left_status = false;
  bool icon_tilt_stop_status = false;
  bool icon_tilt_right_status = false;
  bool tilt_position_status = false;

  if (cover_icons_found) {
    if (entity->is_state(entity_state::closed)) {
      cover_icon = cover_icons.at(1);
    } else {
      cover_icon = cover_icons.at(0);
    }
  }

  // Position
  if (supported_features & 0b00001111) {
    text_position = get_translation(translation_item::position);
    position_status = true;
  }
  // OPEN
  if (supported_features & 0b00000001) {
    if (position != 100 && !((entity->is_state(entity_state::open) ||
        entity->is_state(entity_state::unknown)) &&
        position_str.empty())) {
      icon_up_status = true;
    }
    if (cover_icons_found)
      icon_up = cover_icons.at(2);
  }
  // CLOSE
  if (supported_features & 0b00000010) {
    if (position != 0 && !((entity->is_state(entity_state::closed) ||
        entity->is_state(entity_state::unknown)) &&
        position_str.empty())) {
      icon_down_status = true;
    }
    if (cover_icons_found)
      icon_down = cover_icons.at(3);
  }
  // STOP
  if (supported_features & 0b00001000) {
    icon_stop_status = !entity->is_state(entity_state::unknown);
    icon_stop = icon_t::stop;
  }

  // Tilt supported
  if (supported_features & 0b11110000) {
    text_tilt = get_translation(translation_item::tilt_position);
  }
  // SUPPORT_OPEN_TILT
  if (supported_features & 0b00010000) {
    icon_tilt_left = icon_t::arrow_top_right;
    icon_tilt_left_status = true;
  }
  // SUPPORT_CLOSE_TILT
  if (supported_features & 0b00100000) {
    icon_tilt_right = icon_t::arrow_bottom_left;
    icon_tilt_right_status = true;
  }
  // SUPPORT_STOP_TILT
  if (supported_features & 0b01000000) {
    icon_tilt_stop = icon_t::stop;
    icon_tilt_stop_status = true;
  }
  // SUPPORT_SET_TILT_POSITION
  if (supported_features & 0b10000000) {
    tilt_position_status = true;
    if (tilt_position == 0) {
      icon_tilt_right_status = false;
    }
    if (tilt_position == 100) {
      icon_tilt_left_status = false;
    }
  }

  this->command_buffer_
    // entityUpdateDetail~
    .assign("entityUpdateDetail").append(1, SEPARATOR)
    // entity_id~
    .append("uuid.").append(item->get_uuid()).append(1, SEPARATOR)
    // slider_pos~
    .append(esphome::to_string(position)).append(1, SEPARATOR)
    // position text + state / value~
    .append(text_position).append(": ")
    .append(position_status
      ? esphome::to_string(position).append("%")
      : entity->get_state())
    .append(1, SEPARATOR)
    // position text~
    .append(text_position).append(1, SEPARATOR)
    // icon~
    .append(cover_icon).append(1, SEPARATOR)
    // icon_up~
    .append(icon_up).append(1, SEPARATOR)
    // icon_stop~
    .append(icon_stop).append(1, SEPARATOR)
    // icon_down~
    .append(icon_down).append(1, SEPARATOR)
    // icon_up_status~
    .append(icon_up_status ? generic_type::enable : generic_type::disable)
    .append(1, SEPARATOR)
    // icon_stop_status~
    .append(icon_stop_status ? generic_type::enable : generic_type::disable)
    .append(1, SEPARATOR)
    // icon_down_status~
    .append(icon_down_status ? generic_type::enable : generic_type::disable)
    .append(1, SEPARATOR)
    // tilt text~
    .append(text_tilt).append(1, SEPARATOR)
    // icon_tilt_left~
    .append(icon_tilt_left).append(1, SEPARATOR)
    // icon_tilt_stop~
    .append(icon_tilt_stop).append(1, SEPARATOR)
    // icon_tilt_right~
    .append(icon_tilt_right).append(1, SEPARATOR)
    // icon_tilt_left_status~
    .append(icon_tilt_left_status
      ? generic_type::enable : generic_type::disable)
    .append(1, SEPARATOR)
    // icon_tilt_stop_status~
    .append(icon_tilt_stop_status
      ? generic_type::enable : generic_type::disable)
    .append(1, SEPARATOR)
    // icon_tilt_right_status~
    .append(icon_tilt_right_status
      ? generic_type::enable : generic_type::disable)
    .append(1, SEPARATOR)
    // tilt_position_status
    .append(tilt_position_status
      ? std::to_string(tilt_position).append("%") : generic_type::disable);
}

// entityUpdateDetail~{entity_id}~~{icon_color}~{switch_val}~{brightness}~{color_temp}~{color}~{color_translation}~{color_temp_translation}~{brightness_translation}~{effect_supported}
void NSPanelLovelace::render_light_detail_update_(StatefulPageItem *item) {
  if (item == nullptr) return;

  auto entity = item->get_entity();
  auto &supported_modes = entity->get_attribute(ha_attr_type::supported_color_modes);
  bool enable_color_wheel = entity->is_state(entity_state::on) &&
      (contains_value(supported_modes, ha_attr_color_mode::xy) || 
      contains_value(supported_modes, ha_attr_color_mode::hs) ||
      contains_value(supported_modes, ha_attr_color_mode::rgb) ||
      contains_value(supported_modes, ha_attr_color_mode::rgbw) ||
      contains_value(supported_modes, ha_attr_color_mode::rgbww));

  std::string color_mode = entity->get_attribute(ha_attr_type::color_mode);
  std::string color_temp = generic_type::disable;
  if (contains_value(supported_modes, ha_attr_color_mode::color_temp)) {
    if (color_mode == ha_attr_color_mode::color_temp) {
      color_temp = entity->get_attribute(ha_attr_type::color_temp, generic_type::disable);
    } else {
      color_temp = entity_state::unknown;
    }
  } else {
    color_temp = generic_type::disable;
  }

  this->command_buffer_
    // entityUpdateDetail~
    .assign("entityUpdateDetail").append(1, SEPARATOR)
    // entity_id~~
    .append("uuid.").append(item->get_uuid()).append(2, SEPARATOR)
    // icon_color~
    .append(item->get_icon_color_str()).append(1, SEPARATOR)
    // switch_val~
    .append(std::to_string(entity->is_state(entity_state::on) ? 1 : 0)).append(1, SEPARATOR)
    // brightness~ (0-100)
    .append(entity->get_attribute(ha_attr_type::brightness, generic_type::disable)).append(1, SEPARATOR)
    // color_temp~ (color temperature value or 'disable')
    .append(color_temp).append(1, SEPARATOR)
    // color~ ('enable' or 'disable')
    .append(enable_color_wheel ? generic_type::enable : generic_type::disable).append(1, SEPARATOR)
    // color_translation~
    .append(get_translation(translation_item::color)).append(1, SEPARATOR)
    // color_temp_translation~
    .append(get_translation(translation_item::color_temp)).append(1, SEPARATOR)
    // brightness_translation~
    .append(get_translation(translation_item::brightness)).append(1, SEPARATOR)
    // effect_supported ('enable' or 'disable')
    .append(entity->has_attribute(ha_attr_type::effect_list) ?
      generic_type::enable : generic_type::disable);
}

// entityUpdateDetail~{entity_id}~~{icon_color}~{entity_id}~{min_remaining}~{sec_remaining}~{editable}~{action1}~{action2}~{action3}~{label1}~{label2}~{label3}
void NSPanelLovelace::render_timer_detail_update_(StatefulPageItem *item) {
  if (item == nullptr) return;

  auto &state = item->get_state();
  bool render = false;
  uint16_t min_remaining = 0, sec_remaining = 0;
  bool idle = state == entity_state::paused || state == entity_state::idle;

  if (idle) {
    this->cancel_interval(entity_type::timer);
    std::string time_remaining_str;
    if (state == entity_state::paused) {
      time_remaining_str = item->get_attribute(ha_attr_type::remaining);
    } else {
      time_remaining_str = item->get_attribute(ha_attr_type::duration);
    }
    if (!time_remaining_str.empty()) {
      std::vector<std::string> time_parts;
      split_str(':', time_remaining_str, time_parts);
      if (time_parts.size() == 3) {
        min_remaining = (stoi(time_parts[0]) * 60) + stoi(time_parts[1]);
        sec_remaining = stoi(time_parts[2]);
        render = true;
      }
    }
  }
  // active
  else {
    auto &finishes_at = item->get_attribute(ha_attr_type::finishes_at);
    if (!finishes_at.empty()) {
      tm t{};
      if (iso8601_to_tm(finishes_at.c_str(), t)) {
        ESPTime now = this->time_id_.value()->now();
        if (now.is_valid()) {
          double seconds = difftime(mktime(&t), now.timestamp);
          if (seconds >= UINT16_MAX) seconds = UINT16_MAX;
          if (seconds < 0) seconds = 0;
          min_remaining = static_cast<uint16_t>(seconds) / 60;
          sec_remaining = static_cast<uint16_t>(seconds) % 60;
          render = true;
          if (seconds == 0) {
            this->cancel_interval(entity_type::timer);
          }
        }
      }
    }
  }

  if (!render) {
    this->render_current_page_();
    return;
  }

  this->command_buffer_
    // entityUpdateDetail~
    .assign("entityUpdateDetail").append(1, SEPARATOR)
    // entity_id~~
    .append("uuid.").append(item->get_uuid()).append(2, SEPARATOR)
    // icon_color~
    .append(item->get_icon_color_str()).append(1, SEPARATOR)
    // entity_id~~
    .append("uuid.").append(item->get_uuid()).append(1, SEPARATOR)
    // min_remaining~
    .append(std::to_string(min_remaining)).append(1, SEPARATOR)
    // sec_remaining~
    .append(std::to_string(sec_remaining)).append(1, SEPARATOR)
    // editable~
    .append((idle && 
      item->get_attribute(ha_attr_type::editable) == entity_state::on)
        ? "1" : "0")
    .append(1, SEPARATOR)
    // action1~
    .append(idle ? "" : ha_action_type::pause).append(1, SEPARATOR)
    // action2~
    .append(idle ? ha_action_type::start : ha_action_type::cancel)
    .append(1, SEPARATOR)
    // action3~
    .append(idle ? "" : ha_action_type::finish)
    .append(1, SEPARATOR)
    // label1~
    .append(idle ? "" : get_translation(translation_item::pause))
    .append(1, SEPARATOR)
    // label2~
    .append(get_translation(idle ? 
      translation_item::start : translation_item::cancel))
    .append(1, SEPARATOR)
    // label3
    .append(idle ? "" : get_translation(translation_item::finish));
}

void NSPanelLovelace::render_climate_detail_update_(StatefulPageItem *item) {
  if(item == nullptr) return;
  this->render_climate_detail_update_(item->get_entity(), item->get_uuid());
}

// entityUpdateDetail~{entity_id}~{icon_id}~{icon_color}~(3x)[{heading}~{mode}~{cur_mode}~{modes_res}~]
void NSPanelLovelace::render_climate_detail_update_(Entity *entity, const std::string &uuid) {
  if(entity == nullptr) return;

  uint16_t icon_colour = 64512U;
  auto &state = entity->get_state();
  if (state == entity_state::auto_ ||
      state == entity_state::heat_cool) {
    icon_colour = 1024U;
  } else if (state == entity_state::off ||
      state == entity_state::fan_only) {
    icon_colour = 35921U;
  } else if (state == entity_state::cool) {
    icon_colour = 11487U;
  } else if (state == entity_state::dry) {
    icon_colour = 60897U;
  }

  this->command_buffer_
    // entityUpdateDetail~
    .assign("entityUpdateDetail").append(1, SEPARATOR);

  // entity_id~
  if (!uuid.empty())
    this->command_buffer_.append("uuid.").append(uuid);
  else
    this->command_buffer_.append(entity->get_entity_id());

  this->command_buffer_.append(1, SEPARATOR)
    // icon_id~
    .append(get_icon(CLIMATE_ICON_MAP, entity->get_state()))
    .append(1, SEPARATOR)
    // icon_color~
    .append(std::to_string(icon_colour)).append(1, SEPARATOR);

  std::vector<ha_attr_type> mode_types = {
    ha_attr_type::preset_modes,
    ha_attr_type::swing_modes,
    ha_attr_type::fan_modes
  };

  for (auto mt : mode_types) {
    auto &supported_modes = entity->get_attribute(mt);
    if (supported_modes.empty()) continue;
    
    std::string mode_res;
    if (mt == ha_attr_type::preset_modes) {
      mode_res.reserve(supported_modes.size());
      size_t pos_start = std::string::npos, pos_end = pos_start;
      do {
        pos_end = supported_modes.find(',', pos_start + 1);
        mode_res.append(
          get_translation(
            supported_modes.substr(pos_start + 1, pos_end - pos_start - 1)));
        if (pos_end != std::string::npos) mode_res.append(1, '?');
        pos_start = pos_end;
      } while (pos_start != std::string::npos);
    } else {
      mode_res = supported_modes;
      replace_all(mode_res, ',', '?');
    }
    std::string mode_type = to_string(mt);
    mode_type.pop_back();

    this->command_buffer_
      // heading~
      .append(get_translation(mode_type)).append(1, SEPARATOR)
      // mode~
      .append(to_string(mt)).append(1, SEPARATOR)
      // curr_mode~
      .append(entity->get_attribute(to_ha_attr(mode_type))).append(1, SEPARATOR)
      // mode_res~ (mode names separated by '?')
      .append(mode_res).append(1, SEPARATOR);
  }
}

// entityUpdateDetail2~{entity_id}~~{icon_color}~{ha_type}~{state}~{options}~
void NSPanelLovelace::render_input_select_detail_update_(StatefulPageItem *item) {
  if(item == nullptr) return;

  auto state = item->get_state();
  std::string options;
  if (item->is_type(entity_type::input_select) || 
      item->is_type(entity_type::select)) {
    options = item->get_attribute(ha_attr_type::options);
  }
  else if (item->is_type(entity_type::light)) {
    options = item->get_attribute(ha_attr_type::effect_list);
  }
  else if (item->is_type(entity_type::media_player)) {
    options = item->get_attribute(ha_attr_type::source_list);
    state = item->get_attribute(ha_attr_type::source);
  }
  if (!options.empty()) replace_all(options, ',', '?');

  this->command_buffer_
    // entityUpdateDetail2~
    .assign("entityUpdateDetail2").append(1, SEPARATOR)
    // entity_id~~
    .append("uuid.").append(item->get_uuid()).append(2, SEPARATOR)
    // icon_color~
    .append(item->get_icon_color_str()).append(1, SEPARATOR)
    // ha_type~
    .append(item->get_type()).append(1, SEPARATOR)
    // state~
    .append(state).append(1, SEPARATOR)
    // options~
    .append(options).append(1, SEPARATOR);
}

// entityUpdateDetail~{entity_id}~~{icon_color}~{switch_val}~{speed}~{speed_max}~{speed_translation}~{preset_mode}~{preset_modes}
void NSPanelLovelace::render_fan_detail_update_(StatefulPageItem *item) {
  if(item == nullptr) return;

  auto speed = item->get_attribute(ha_attr_type::percentage);
  auto percentage_step = item->get_attribute(ha_attr_type::percentage_step);
  auto preset_mode = item->get_attribute(ha_attr_type::preset_mode);
  auto preset_modes = item->get_attribute(ha_attr_type::preset_modes);
  if (!preset_modes.empty()) replace_all(preset_modes, ',', '?');

  uint8_t speed_max = 100;
  if (!percentage_step.empty()) {
    float speed_val = 0.0f;
    if (speed.empty()) {
      speed = "0";
    } else {
      speed_val = std::stof(speed);
    }
    auto step_val = std::stof(percentage_step);
    if (step_val < 1.0f) step_val = 1.0f; // avoid divide-by-zero
    speed = esphome::to_string(
      static_cast<uint16_t>(round(speed_val / step_val)));
    speed_max = static_cast<uint16_t>(round(100.0f / step_val));
  }

  this->command_buffer_
    // entityUpdateDetail~
    .assign("entityUpdateDetail").append(1, SEPARATOR)
    // entity_id~~
    .append("uuid.").append(item->get_uuid()).append(2, SEPARATOR)
    // icon_color~
    .append(item->get_icon_color_str()).append(1, SEPARATOR)
    // switch_val~
    .append(esphome::to_string(item->is_state(entity_state::on) ? 1 : 0))
    .append(1, SEPARATOR)
    // speed~
    .append(percentage_step.empty() ? generic_type::disable : speed)
    .append(1, SEPARATOR)
    // speed_max~
    .append(esphome::to_string(speed_max)).append(1, SEPARATOR)
    // speed_translation~
    .append(get_translation(translation_item::speed)).append(1, SEPARATOR)
    // preset_mode~
    .append(preset_mode).append(1, SEPARATOR)
    // preset_modes
    .append(preset_modes);
}

void NSPanelLovelace::dump_config() {
  ESP_LOGCONFIG(TAG, "NSPanelLovelace:");
  ESP_LOGCONFIG(TAG, "\tVersion: %s", NSPANEL_LOVELACE_BUILD_VERSION);
  ESP_LOGCONFIG(TAG, "\tRAM: psram_used:%zu int_free:%zu int_free_blk:%zu",
    psram_used(),
    heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
    heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
  ESP_LOGCONFIG(TAG, "\tState: pages:%zu,stateful_items:%zu,entities:%zu",
      this->pages_.size(),
      this->stateful_page_items_.size(),
      this->entities_.size());
}

void NSPanelLovelace::send_nextion_command_(const std::string &command) {
  ESP_LOGD(TAG, "Sending: %s", command.c_str());
  this->write_str(command.c_str());
  const uint8_t to_send[3] = {0xFF, 0xFF, 0xFF};
  this->write_array(to_send, sizeof(to_send));
}

void NSPanelLovelace::process_display_command_queue_() {
#ifdef USE_NSPANEL_TFT_UPLOAD
  // don't execute custom commands when the screen is updating - UI updates could spoil the upload
  if (this->is_updating_) return;
#endif
  // nothing to process
  if (this->command_buffer_.empty() && this->command_queue_.empty()) return;

  // Store the command for later processing so the function can return quickly
  if (!this->command_buffer_.empty()) {
    this->command_queue_.push(this->command_buffer_);
    ESP_LOGVV(TAG, "Command queued (size: %u)", this->command_queue_.size());
    this->command_buffer_.clear();
    return;
  } else if (!this->command_queue_.empty()) {
    // todo: can we use std::move? it changes the capacity of the buffer
    this->command_buffer_.assign(this->command_queue_.front());
    this->command_queue_.pop();
    ESP_LOGVV(TAG, "Command un-queued (size: %u)", this->command_queue_.size());
  }

  ESP_LOGD(TAG, "TFT CMD OUT: %s", this->command_buffer_.c_str());
  std::array<uint8_t, 4> crc_data = {
    0x55, 0xBB, 
    static_cast<uint8_t>(command_buffer_.length() & 0xFF),
    static_cast<uint8_t>((command_buffer_.length() >> 8) & 0xFF)
  };
  auto crc = esphome::crc16(crc_data.data(), 4);
  crc = esphome::crc16(
    reinterpret_cast<const uint8_t *>(command_buffer_.c_str()),
    command_buffer_.length(), crc);

  this->write_array(crc_data);
  App.feed_wdt();
  this->write_str(this->command_buffer_.c_str());
  crc_data[0] = static_cast<uint8_t>(crc & 0xFF);
  crc_data[1] = static_cast<uint8_t>((crc >> 8) & 0xFF);
  this->write_array(crc_data.data(), 2);
  
  this->command_buffer_.clear();
  this->command_last_sent_ = millis();
}

void NSPanelLovelace::send_buffered_command_() {
  if (this->command_buffer_.empty()) return;
  this->process_display_command_queue_();
}

void NSPanelLovelace::notify_on_screensaver(
    const std::string &heading, const std::string &message,
    uint32_t timeout_ms) {
  // notification will not show up if we are not on the screensaver
  // todo: could force a switch to screensaver or show the notification if
  //       the user navigates back to screensaver within the timeout period
  if (!this->current_page_->is_type(page_type::screensaver)) return;
  
  this->send_display_command(
    std::string("notify").append(1,SEPARATOR)
      .append(heading).append(1, SEPARATOR)
      .append(message));
  
  if (timeout_ms > 0) {
    // hide the notification after a period of time
    this->set_timeout(timeout_ms, [this]() {
      if (!this->current_page_->is_type(page_type::screensaver)) return;
      this->render_screensaver();
    });
  }
}

void NSPanelLovelace::send_display_command(const std::string &command) {
  this->command_buffer_.assign(command);
  this->send_buffered_command_();
}

#ifdef USE_NSPANEL_TFT_UPLOAD
uint16_t NSPanelLovelace::recv_ret_string_(std::string &response, uint32_t timeout, bool recv_flag) {
#ifdef FAKE_TFT_UPLOAD
  response.assign(1, 0x05); //ok response
  return 0U;
#else
  uint16_t ret;
  uint8_t c = 0;
  uint8_t nr_of_ff_bytes = 0;
  uint64_t start;
  bool exit_flag = false;
  bool ff_flag = false;

  start = millis();

  while ((timeout == 0 && this->available()) || millis() - start <= timeout) {
    if (!this->available()) {
      App.feed_wdt();
      continue;
    }

    this->read_byte(&c);
    if (c == 0xFF) {
      nr_of_ff_bytes++;
    } else {
      nr_of_ff_bytes = 0;
      ff_flag = false;
    }

    if (nr_of_ff_bytes >= 3)
      ff_flag = true;

    response += (char)c;
    if (recv_flag) {
      if (response.find(0x05) != std::string::npos) {
        exit_flag = true;
      }
    }
    App.feed_wdt();
    delay(2);

    if (exit_flag || ff_flag) {
      break;
    }
  }

  if (ff_flag)
    response = response.substr(0, response.length() - 3); // Remove last 3 0xFF

  ret = response.length();
  return ret;
#endif
}

void NSPanelLovelace::set_reparse_mode_(bool active) {
  if (this->reparse_mode_ == active) return;

  if (active) {
    this->send_nextion_command_("recmod=1");
  } else {
    this->send_nextion_command_("DRAKJHSUYDGBNCJHGJKSHBDN");
    this->send_nextion_command_("recmod=0");
    this->send_nextion_command_("recmod=0");
    this->send_nextion_command_("connect");
  }

  this->reparse_mode_ = active;
}
#endif // USE_NSPANEL_TFT_UPLOAD

void NSPanelLovelace::init_display_(int baud_rate) {
  // hopefully on NSPanel it should always be an ESP32ArduinoUARTComponent instance
#ifdef USE_ESP_IDF
  auto *uart = reinterpret_cast<uart::IDFUARTComponent*>(this->parent_);
#else
  auto *uart = reinterpret_cast<uart::ESP32ArduinoUARTComponent*>(this->parent_);
#endif
  uart->set_baud_rate(baud_rate);
  uart->setup();
}

#ifdef USE_TIME
// see: https://esphome.io/components/time/#strftime
void NSPanelLovelace::update_datetime(const datetime_mode mode, const char *date_format, const char *time_format) {
  ESPTime now = this->time_id_.value()->now();

  if (!now.is_valid()) {
    ESP_LOGW(TAG, "esphome time invalid, using default time");
    now = esphome::ESPTime::from_epoch_utc(0);
  }

  // ESP_LOGV(TAG, "datetime update %u,%u %u,%u", now.hour, this->now_hour_, now.minute, this->now_minute_);
  this->now_hour_ = now.hour;
  this->now_minute_ = now.minute;

  if ((mode & datetime_mode::date) == datetime_mode::date) {
    std::string datefmt(date_format);
    // todo: fetch from config before using default value
    if (datefmt.empty())
      datefmt = this->date_format_;
    
    DayOfWeekMap::dow_mode dow_mode = DayOfWeekMap::dow_mode::none;
    if (datefmt.find("%A", 0) != std::string::npos) {
      dow_mode = (DayOfWeekMap::dow_mode)(dow_mode |
                  DayOfWeekMap::dow_mode::long_dow);
    }
    if (datefmt.find("%a", 0) != std::string::npos ||
        datefmt.find("%c", 0) != std::string::npos || 
        datefmt.find("%h", 0) != std::string::npos) {
      dow_mode = (DayOfWeekMap::dow_mode)(dow_mode |
                  DayOfWeekMap::dow_mode::short_dow);
    }

    auto timestr = now.strftime(datefmt);
    this->command_buffer_
      .assign("date").append(1, SEPARATOR)
      .append(this->day_of_week_map_.replace(timestr, dow_mode));
    this->send_buffered_command_();
  }

  if ((mode & datetime_mode::time) == datetime_mode::time) {
    std::string timefmt(time_format);
    // todo: fetch from config before using default value
    if (timefmt.empty())
      timefmt = this->time_format_;
    
    this->command_buffer_
      .assign("time").append(1, SEPARATOR)
      .append(now.strftime(timefmt));
    this->send_buffered_command_();
  }
}

void NSPanelLovelace::setup_time_() {
  if (this->time_id_.has_value()) {
    this->time_id_.value()->add_on_time_sync_callback([this] {
      this->update_datetime(datetime_mode::both);
    });
    // Configure a callback to check the time every 1 second
    this->set_interval("check_time", 1000, [this] {
      this->check_time_();
    });
    this->time_configured_ = true;
  } else {
    ESP_LOGW(TAG, "time_id not configured, default time displayed");
  }
}

void NSPanelLovelace::check_time_() {
  if (!this->time_id_.has_value() ||
      !this->time_id_.value()->now().is_valid())
    return;

  // todo: only check if the current page is screensaver?
  ESPTime now = this->time_id_.value()->now();
  // update the date once an hour to account for daylight saving etc.
  if (now.hour != this->now_hour_) {
    this->update_datetime(datetime_mode::both);
  }
  // update the time every minute
  else if (now.minute != this->now_minute_) {
    this->update_datetime(datetime_mode::time);
  }
}

#endif

size_t NSPanelLovelace::find_page_index_by_uuid_(const std::string &uuid) const {
  size_t index = 0;
  for (auto &p : this->pages_) {
    if (p->get_uuid() == uuid)
      return index;
    ++index;
  }
  return SIZE_MAX;
}

const std::string &NSPanelLovelace::try_replace_uuid_with_entity_id_(
    const std::string &uuid_or_entity_id) {
  // not a uuid if it does not begin with the uuid prefix
  // (navigation uuids are dealt with separately)
  if (!esphome::str_startswith(uuid_or_entity_id, entity_type::uuid))
    return uuid_or_entity_id;

  auto uuid = uuid_or_entity_id.substr(5);
  auto item = this->get_page_item_(uuid);
  if (item == nullptr) return uuid_or_entity_id;
  
  return item->get_entity_id();
}

void NSPanelLovelace::process_button_press_(
    std::string &internal_id, 
    const std::string &button_type, 
    const std::string &value,
    bool called_from_timeout) {
  if (button_type.empty()) return;
  
  // Throttle and filter processing of spammy actions to avoid command flooding
  if (!called_from_timeout) {
    if (internal_id == this->button_press_uuid_ && 
        button_type == this->button_press_type_) {
      this->button_press_value_ = value;
      if (this->button_press_timeout_set_) return;
      this->set_timeout("btnpr", 200, [this]() {
        this->button_press_timeout_set_ = false;
        ESP_LOGD(TAG, "Button press delayed: %s,%s,%s", 
            this->button_press_uuid_.c_str(), this->button_press_type_.c_str(), 
            this->button_press_value_.c_str());
        this->process_button_press_(
            this->button_press_uuid_, this->button_press_type_, 
            this->button_press_value_, true);
      });
      this->button_press_timeout_set_ = true;
      return;
    } else if (this->button_press_timeout_set_) {
      this->cancel_timeout("btnpr");
      this->button_press_timeout_set_ = false;
    }
    this->button_press_uuid_ = internal_id;
    this->button_press_type_ = button_type;
  }

  auto entity_type = get_entity_type(internal_id);
  std::string& entity_id = internal_id;
  
  if (entity_type == entity_type::uuid) {
    entity_id = this->try_replace_uuid_with_entity_id_(internal_id);
    ESP_LOGV(TAG, "Lookup %s -> %s", internal_id.c_str(), entity_id.c_str());
    entity_type = get_entity_type(entity_id);
    if (entity_type == nullptr) return;
  }

  // Screen tapped when on the screensaver, show the default card or use the first card in the config.
  if (internal_id == to_string(page_type::screensaver) && button_type == button_type::bExit) {
    // todo: make a note of last used card
    //
    // config.get("screensaver.defaultCard")
    // use defaultCard if defaultCard not null

    // _previous_card.clear();
    // _current_card = action_type::screensaver;
    // render_card(_current_card);

    // todo: temporary for testing
    this->render_page_(render_page_option::default_page);
    return;
  }

  if (button_type == button_type::sleepReached) {
    // todo
    // make a note of last used card then render screensaver
    // _previous_card = _current_card;
    // _current_card = action_type::screensaver;
    // render_page_(_current_card);
    this->render_page_(render_page_option::screensaver);
    return;
  }

  if (button_type == button_type::bExit) {
    this->render_current_page_();
    return;
  }

  if (button_type == button_type::onOff) {
    if (!value.empty()) {
      this->call_ha_service_(
        entity_type, 
        value == "1" ? ha_action_type::turn_on : ha_action_type::turn_off, 
        entity_id);
    }
  } 
  // fan, number, input_number
  else if (button_type == button_type::numberSet) {
    if (entity_type == entity_type::fan) {
      auto entity = this->get_entity_(entity_id);
      if (entity == nullptr) return;
      auto step = std::stof(
        entity->get_attribute(ha_attr_type::percentage_step, "0"));
      if (step > 100.0f) step = 100.0f;
      auto val = std::stof(value) * step;
      if (val > 100.0f) val = 100.0f;
      auto pct = esphome::str_snprintf("%.6f", 11, val);
      
      this->call_ha_service_(
        entity_type, 
        ha_action_type::set_percentage, 
        {{
          {to_string(ha_attr_type::entity_id), entity_id},
          {to_string(ha_attr_type::percentage), pct}
        }});
    } else {
      this->call_ha_service_(
        entity_type, 
        ha_action_type::set_value, 
        {{
          {to_string(ha_attr_type::entity_id), entity_id},
          {to_string(ha_attr_type::value), value}
        }});
    }
  }
  // cover and shutter cards
  else if (button_type == button_type::up) {
    this->call_ha_service_(
      entity_type, ha_action_type::open_cover, entity_id);
  } else if (button_type == button_type::stop) {
    this->call_ha_service_(
      entity_type, ha_action_type::stop_cover, entity_id);
  } else if (button_type == button_type::down) {
    this->call_ha_service_(
      entity_type, ha_action_type::close_cover, entity_id);
  } else if (button_type == button_type::positionSlider) {
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_cover_position, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::position), value}
      }});
  } else if (button_type == button_type::tiltOpen) {
    this->call_ha_service_(
      entity_type, ha_action_type::open_cover_tilt, entity_id);
  } else if (button_type == button_type::tiltStop) {
    this->call_ha_service_(
      entity_type, ha_action_type::stop_cover_tilt, entity_id);
  } else if (button_type == button_type::tiltClose) {
    this->call_ha_service_(
      entity_type, ha_action_type::close_cover_tilt, entity_id);
  } else if (button_type == button_type::tiltSlider) {
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_cover_tilt_position, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::tilt_position), value}
      }});
  } else if (button_type == button_type::button) {
    if (entity_type == entity_type::navigate ||
        entity_type == entity_type::navigate_uuid) {
      auto uuid = internal_id.substr(strlen(entity_type) + 1);
      this->render_page_(this->find_page_index_by_uuid_(uuid));
    } else if (
        entity_type == entity_type::scene ||
        entity_type == entity_type::script) {
      this->call_ha_service_(
        entity_type, ha_action_type::turn_on, entity_id);
    } else if (
        entity_type == entity_type::light ||
        entity_type == entity_type::switch_ ||
        entity_type == entity_type::input_boolean ||
        entity_type == entity_type::automation ||
        entity_type == entity_type::fan) {
      this->call_ha_service_(
        entity_type, ha_action_type::toggle, entity_id);
    } else if (
        entity_type == entity_type::button ||
        entity_type == entity_type::input_button) {
      this->call_ha_service_(
        entity_type, ha_action_type::press, entity_id);
    } else if (entity_type == entity_type::input_select) {
      this->call_ha_service_(
        entity_type, ha_action_type::select_next, entity_id);
    } else if (entity_type == entity_type::vacuum) {
      auto entity = this->get_entity_(entity_id);
      if (entity == nullptr) return;
      this->call_ha_service_(entity_type,
        entity->is_state(entity_state::docked) 
          ? ha_action_type::start 
          : ha_action_type::return_to_base,
        entity_id);
    } else if (entity_type == entity_type::lock) {
      auto entity = this->get_entity_(entity_id);
      if (entity == nullptr) return;
      this->call_ha_service_(entity_type,
        entity->is_state(entity_state::locked) 
          ? ha_action_type::unlock 
          : ha_action_type::lock,
        entity_id);
    }
  }
  // media cards
  else if (button_type == button_type::mediaNext) {
    this->call_ha_service_(
      entity_type, ha_action_type::media_next_track, entity_id);
  } else if (button_type == button_type::mediaBack) {
    this->call_ha_service_(
      entity_type, ha_action_type::media_previous_track, entity_id);
  } else if (button_type == button_type::mediaPause) {
    this->call_ha_service_(
      entity_type, ha_action_type::media_play_pause, entity_id);
  } else if (button_type == button_type::mediaOnOff) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    this->call_ha_service_(
      entity_type,
      entity->is_state(entity_state::on) 
        ? ha_action_type::turn_off 
        : ha_action_type::turn_on,
      entity_id);
  } else if (button_type == button_type::mediaShuffle) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto shuffle = entity->get_attribute(ha_attr_type::shuffle);
    if (shuffle.empty()) return;
    shuffle = shuffle == entity_state::off 
      ? entity_state::on : entity_state::off;
    this->call_ha_service_(
      entity_type,
      ha_action_type::shuffle_set,
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::shuffle), shuffle}
      }});
  } else if (button_type == button_type::volumeSlider) {
    auto volume = esphome::str_snprintf("%.2f", 7, std::stoi(value) * 0.01f);
    this->call_ha_service_(
      entity_type,
      ha_action_type::volume_set,
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::volume_level), volume}
      }});
  } else if (button_type == button_type::speakerSel) {
    this->call_ha_service_(
      entity_type,
      ha_action_type::select_source,
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::source), value}
      }});
  } else if (button_type == button_type::modeMediaPlayer) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto source_list_str = entity->get_attribute(ha_attr_type::source_list);
    if (source_list_str.empty()) return;
    std::vector<std::string> source_list;
    split_str(',', source_list_str, source_list);
    uint8_t index = stoi(value);
    if (source_list.size() <= index) return;
    this->call_ha_service_(
      entity_type,
      ha_action_type::select_source,
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::source), source_list.at(index)}
      }});
  }
  // light cards
  else if (button_type == button_type::brightnessSlider) {
    if (value.empty()) return;
    this->call_ha_service_(
      entity_type, 
      ha_action_type::turn_on, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        // scale 0-100 to ha brightness range
        {to_string(ha_attr_type::brightness), std::to_string(
          static_cast<int>(
            scale_value(std::stoi(value), {0, 100}, {0, 255})
          ))}
      }});
  } else if (button_type == button_type::colorTempSlider) {
    if (value.empty()) return;
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto &minstr = entity->get_attribute(ha_attr_type::min_mireds);
    auto &maxstr = entity->get_attribute(ha_attr_type::max_mireds);
    uint16_t min_mireds = minstr.empty() ? 153 : std::stoi(minstr);
    uint16_t max_mireds = maxstr.empty() ? 500 : std::stoi(maxstr);
    if (min_mireds >= max_mireds) {
      ESP_LOGW(TAG, "min/max mired range invalid %i>=%i", min_mireds, max_mireds);
      min_mireds = 153;
      max_mireds = 500;
    }
    
    this->call_ha_service_(
      entity_type, 
      ha_action_type::turn_on, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        // scale 0-100 from slider to color range of the light
        {to_string(ha_attr_type::color_temp), std::to_string(
          static_cast<int>(
            scale_value(std::stoi(value), {0, 100},
            {static_cast<double>(min_mireds), static_cast<double>(max_mireds)})
          ))}
      }});
  } else if (button_type == button_type::colorWheel) {
    if (value.empty()) return;

    std::vector<std::string> xy_tokens;
    split_str('|', value, xy_tokens);
    if (xy_tokens.size() != 3) return;

    std::string rgb_str = to_string(
        xy_to_rgb(
          std::stod(xy_tokens[0]),
          std::stod(xy_tokens[1]),
          std::stod(xy_tokens[2])
        ), ',', '[', ']');

    this->call_ha_service_(
      entity_type, 
      ha_action_type::turn_on, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id}
      }},
      {{
        {to_string(ha_attr_type::rgb_color), rgb_str}
      }});
  }
  // thermo/climate card
  else if (button_type == button_type::tempUpd) {
    auto val = esphome::str_snprintf("%.1f", 6, std::stoi(value) * 0.1);
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_temperature, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::temperature), val}
      }});
  } else if (button_type == button_type::tempUpdHighLow) {
    std::vector<std::string> temp_values;
    split_str('|', value, temp_values);
    auto temp_high = esphome::str_snprintf(
      "%.1f", 6, std::stoi(temp_values[0]) * 0.1);
    auto temp_low = esphome::str_snprintf(
      "%.1f", 6, std::stoi(temp_values[1]) * 0.1);
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_temperature, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::target_temp_high), temp_high},
        {to_string(ha_attr_type::target_temp_low), temp_low}
      }});
  } else if (button_type == button_type::hvacAction) {
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_hvac_mode, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::hvac_mode), value}
      }});
  } else if (button_type == button_type::modePresetModes) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto &modes_str = entity->get_attribute(ha_attr_type::preset_modes);
    if (modes_str.empty()) return;
    std::vector<std::string> modes;
    split_str(',', modes_str, modes);
    auto &selected_mode = modes.at(std::stoi(value));
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_preset_mode, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::preset_mode), selected_mode}
      }});
  } else if (button_type == button_type::modeSwingModes) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto &modes_str = entity->get_attribute(ha_attr_type::swing_modes);
    if (modes_str.empty()) return;
    std::vector<std::string> modes;
    split_str(',', modes_str, modes);
    auto &selected_mode = modes.at(std::stoi(value));
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_swing_mode, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::swing_mode), selected_mode}
      }});
  } else if (button_type == button_type::modeFanModes) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto &modes_str = entity->get_attribute(ha_attr_type::fan_modes);
    if (modes_str.empty()) return;
    std::vector<std::string> modes;
    split_str(',', modes_str, modes);
    auto &selected_mode = modes.at(std::stoi(value));
    this->call_ha_service_(
      entity_type, 
      ha_action_type::set_fan_mode, 
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::fan_mode), selected_mode}
      }});
  }
  // alarm card
  else if (
      button_type == button_type::armHome ||
      button_type == button_type::armAway ||
      button_type == button_type::armNight ||
      button_type == button_type::armVacation ||
      button_type == button_type::disarm) {
    auto action = std::string("alarm_").append(button_type);
    if (value.empty()) {
      this->call_ha_service_(entity_type, action.c_str(), entity_id);
    } else {
      this->call_ha_service_(
        entity_type, action.c_str(), 
        {{
          {to_string(ha_attr_type::entity_id), entity_id},
          {to_string(ha_attr_type::code), value}
        }});
    }
  } else if (button_type == button_type::opnSensorNotify) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto &open_sensors_str = entity->get_attribute(ha_attr_type::open_sensors);
    if (open_sensors_str.empty()) return;
    std::string message;
    message.reserve(open_sensors_str.size());
    std::vector<std::string> open_sensors;
    split_str(',', open_sensors_str, open_sensors);
    // todo: Find a way to populate entitity 'friendly_name' without subscribing to all entities
    for (auto &&sensor : open_sensors) {
      message.append("- ").append(sensor).append("\r\n");
    }
    this->render_popup_notify_page_("", "", message);
  }
  // unlock card
  else if (button_type == button_type::cardUnlockUnlock) {
    if (!this->current_page_->is_type(page_type::cardUnlock)) return;
    // todo
  }
  // select & input_select
  else if (
      button_type == button_type::modeInputSelect ||
      button_type == button_type::modeSelect) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto options_str = entity->get_attribute(ha_attr_type::options);
    if (options_str.empty()) return;
    std::vector<std::string> options;
    split_str(',', options_str, options);
    uint8_t index = stoi(value);
    if (options.size() <= index) return;
    this->call_ha_service_(
      entity_type,
      ha_action_type::select_option,
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::option), options.at(index)}
      }});
  }
  // light
  else if (button_type == button_type::modeLight) {
    auto entity = this->get_entity_(entity_id);
    if (entity == nullptr) return;
    auto effects_str = entity->get_attribute(ha_attr_type::effect_list);
    if (effects_str.empty()) return;
    std::vector<std::string> effects;
    split_str(',', effects_str, effects);
    uint8_t index = stoi(value);
    if (effects.size() <= index) return;
    this->call_ha_service_(
      entity_type,
      ha_action_type::turn_on,
      {{
        {to_string(ha_attr_type::entity_id), entity_id},
        {to_string(ha_attr_type::effect), effects.at(index)}
      }});
  }
  // timer card
  else if (esphome::str_startswith(button_type, entity_type::timer)) {
    std::string service(button_type);
    service[5] = '.';
    if (value.empty()) {
      this->call_ha_service_(service, entity_id);
    } else {
      this->call_ha_service_(service, 
        {{
          {to_string(ha_attr_type::entity_id), entity_id},
          {to_string(ha_attr_type::duration), value}
        }});
    }
  }
}

StatefulPageItem* NSPanelLovelace::get_page_item_(const std::string &uuid) {
  // use cached version if possible
  if (this->cached_page_item_ != nullptr && 
      this->cached_page_item_->get_uuid() == uuid) 
    return this->cached_page_item_;

  for (auto& item : this->stateful_page_items_) {
    if (item->get_uuid() != uuid) continue;
    return this->cached_page_item_ = item.get();
  }
  return this->cached_page_item_ = nullptr;
}

Entity* NSPanelLovelace::get_entity_(const std::string &entity_id) {
  // use cached version if possible
  if (this->cached_entity_ != nullptr && 
      this->cached_entity_->get_entity_id() == entity_id) 
    return this->cached_entity_;

  for (auto& entity : this->entities_) {
    if (entity->get_entity_id() != entity_id) continue;
    return this->cached_entity_ = entity.get();
  }
  return this->cached_entity_ = nullptr;
}

void NSPanelLovelace::call_ha_service_(
    const std::string &service, const std::string &entity_id) {
  this->call_ha_service_(service, {{to_string(ha_attr_type::entity_id), entity_id}});
}

void NSPanelLovelace::call_ha_service_(
    const char *entity_type, const std::string &action, const std::string &entity_id) {
  this->call_ha_service_(entity_type, action, {{to_string(ha_attr_type::entity_id), entity_id}});
}

void NSPanelLovelace::call_ha_service_(
    const char *entity_type, const std::string &action,
    const std::map<std::string, std::string> &data,
    const std::map<std::string, std::string> &data_template) {
  this->call_ha_service_(
    std::string(entity_type).append(1, '.').append(action),
    data, data_template);
}

void NSPanelLovelace::call_ha_service_(
    const std::string &service,
    const std::map<std::string, std::string> &data,
    const std::map<std::string, std::string> &data_template) {
  api::HomeassistantServiceResponse resp;
  resp.service = service;

  auto it = data.find(to_string(ha_attr_type::entity_id));
  if (it == data.end())
    ESP_LOGD(TAG, "Call HA: %s -> %s", resp.service.c_str(), it->second.c_str());
  else
    ESP_LOGD(TAG, "Call HA: %s", resp.service.c_str());

  for (auto &it : data) {
    api::HomeassistantServiceMap kv;
    kv.key = it.first;
    kv.value = it.second;
    resp.data.push_back(kv);
  }
  for (auto &it : data_template) {
    api::HomeassistantServiceMap kv;
    kv.key = it.first;
    kv.value = it.second;
    resp.data_template.push_back(kv);
  }

  api::global_api_server->send_homeassistant_service_call(resp);
}

void NSPanelLovelace::on_entity_state_update_(std::string entity_id, std::string state) {
  this->on_entity_attribute_update_(entity_id, to_string(ha_attr_type::state), state);
}
void NSPanelLovelace::on_entity_attribute_update_(std::string entity_id, std::string attr, std::string attr_value) {
  auto entity = this->get_entity_(entity_id);
  if (entity == nullptr) return;
  auto ha_attr = to_ha_attr(attr);
  if (ha_attr == ha_attr_type::unknown) return;

  if (ha_attr == ha_attr_type::state) {
    entity->set_state(attr_value);
  } else {
    entity->set_attribute(ha_attr, attr_value);
  }

  ESP_LOGD(TAG, "HA update: %s %s='%s'",
    entity_id.c_str(), attr.c_str(), 
    ha_attr == ha_attr_type::state
      ? entity->get_state().c_str()
      : entity->get_attribute(ha_attr).c_str());

  // if (this->force_current_page_update_) return;

  // If there are lots of entity attributes that update within a short time
  // then this will queue lots of commands unnecessarily.
  // This re-schedules updates every time one happens within a 200ms period.
  this->set_timeout(entity_id, 200, [this, entity_id] () {
    if (this->force_current_page_update_) return;
    if (this->current_page_ == nullptr) return;

    if (this->screensaver_ != nullptr && 
        this->current_page_->is_type(page_type::screensaver)) {
      force_current_page_update_ = 
        this->screensaver_->should_render_status_update(entity_id);
      return;
    }

    // re-render only if the entity is on the currently active card
    // todo: this doesnt account for popup pages
    for (auto &item : this->current_page_->get_items()) {
      auto stateful_item = page_item_cast<StatefulPageItem>(item.get());
      if (stateful_item == nullptr) continue;
      
      if (stateful_item->get_entity_id() == entity_id) {
        force_current_page_update_ = true;
        return;
      }
    }

    auto entity_type = get_entity_type(entity_id);
    // Thermo cards don't have items to check, only a single thermo entity
    // render updates when climate entitites are updated
    if (entity_type == entity_type::climate &&
        this->current_page_->is_type(page_type::cardThermo)) {
      force_current_page_update_ = true;
      return;
    }
    else if (entity_type == entity_type::media_player &&
        this->current_page_->is_type(page_type::cardMedia)) {
      force_current_page_update_ = true;
      return;
    }
    else if (entity_type == entity_type::alarm_control_panel &&
        this->current_page_->is_type(page_type::cardAlarm)) {
      force_current_page_update_ = true;
      return;
    }
    
    // todo: implement popup page checks too
    // if (this->popup_page_current_uuid_ == item->get_uuid()) {
    //   this->render_popup_page_update_(item);
    // } else if (this->popup_page_current_uuid_.empty()) {
    //   this->render_item_update_(this->current_page_);
    // }
  });
}

void NSPanelLovelace::send_weather_update_command_() {
  if (this->current_page_ != this->screensaver_)
    return;
  this->screensaver_->render(this->command_buffer_);
  this->send_buffered_command_();
}

void NSPanelLovelace::on_weather_state_update_(std::string entity_id, std::string state) {
  if (this->screensaver_ == nullptr) return;
  auto item = this->screensaver_->get_item<WeatherItem>(0);
  if (item == nullptr) return;
  item->set_icon_by_weather_condition(state);
  this->send_weather_update_command_();
}

void NSPanelLovelace::on_weather_temperature_update_(std::string entity_id, std::string temperature) {
  if (this->screensaver_ == nullptr) return;
  auto item = this->screensaver_->get_item<WeatherItem>(0);
  if (item == nullptr) return;
  item->set_value(std::move(temperature));
  this->send_weather_update_command_();
}

void NSPanelLovelace::on_weather_temperature_unit_update_(std::string entity_id, std::string temperature_unit) {
  if (this->screensaver_ == nullptr) return;
  WeatherItem::temperature_unit = std::move(temperature_unit);
  this->screensaver_->set_items_render_invalid();
  this->send_weather_update_command_();
}

void NSPanelLovelace::on_weather_forecast_update_(std::string entity_id, std::string forecast_json) {
  if (this->screensaver_ == nullptr) return;
  // todo: check if we are on the screensaver otherwise don't update
  // todo: implement color updates: "color~background~tTime~timeAMPM~tDate~tMainText~tForecast1~tForecast2~tForecast3~tForecast4~tForecast1Val~tForecast2Val~tForecast3Val~tForecast4Val~bar~tMainTextAlt2~tTimeAdd"

  ArduinoJson::StaticJsonDocument<200> filter;
  filter[0]["datetime"] = true;
  filter[0]["condition"] = true;
  filter[0]["temperature"] = true;

  if (filter.overflowed()) {
    ESP_LOGW(TAG, "Weather unparsable: filter overflowed");
    return;
  }
  
  // Note: Unfortunately the json received is nearly 6KB!
  //       We filter the variables to consume less but it is still a lot,
  //       so we need to allocate an appropriate amount of memory to read it.
  SpiRamJsonDocument doc(psram_available() ? 7680 : 6144);
  ArduinoJson::DeserializationError error = ArduinoJson::deserializeJson(
    doc, (char *)forecast_json.data(), DeserializationOption::Filter(filter));
  App.feed_wdt();

  if (error || doc.overflowed()) {
    ESP_LOGW(TAG, "Weather unparsable: %s", error ? error.c_str() : "doc overflow");
    return;
  }

  this->command_buffer_.clear();

  // check if forecast is hourly or daily
  auto weather_entity_is_hourly = false;
  if (doc.size() > 1) {
    auto date1 = doc[0]["datetime"].as<const char *>();
    auto date2 = doc[1]["datetime"].as<const char *>();
    tm t{};
    if (iso8601_to_tm(date1, t)) {
      uint8_t hr = t.tm_hour;
      if (iso8601_to_tm(date2, t) && t.tm_hour != hr) {
        weather_entity_is_hourly = true;
      }
    }
  }

  char buff[16] = {};
  uint8_t index = 1, item_count = this->screensaver_->get_items().size();

  for (const ArduinoJson::JsonObject &item : doc.as<ArduinoJson::JsonArray>()) {
    // can only display the first 4 items (minus 1 for the current weather)
    if (index == item_count)
      break;

    auto weatherItem = this->screensaver_->get_item<WeatherItem>(index);
    if (weatherItem == nullptr)
      continue;

    weatherItem->set_icon_by_weather_condition(
        item["condition"].as<std::string>());

    // icon displayName
    // todo: import temperature symbol from config
    tm t{};
    // Parse date e.g. 2023-08-22T21:00:00+00:00
    if (!iso8601_to_tm(item["datetime"], t)) {
      ESP_LOGW(TAG, "Weather 'datetime' unparsable: %s", item["datetime"].as<const char *>());
      // return;
      t = { 
        // second, minute, hour
        0,0,0,
        // monthday, month, year
        0,0,100,
        // weekday, yearday, isdst
        0,0,0
      };
    }

    if (weather_entity_is_hourly) {
      // ESPTime now; now.strftime(datefmt);
      strftime(buff, sizeof(buff), this->time_format_.c_str(), &t);
      weatherItem->set_display_name(buff);
    } else {
      weatherItem->set_display_name(this->day_of_week_map_.at(t.tm_wday).at(0));
    }
    
    snprintf(buff, sizeof(buff), "%.1f", item["temperature"].as<float>());
    weatherItem->set_value(buff);

    ++index;
  }
  this->send_weather_update_command_();
}

} // namespace nspanel_lovelace
} // namespace esphome