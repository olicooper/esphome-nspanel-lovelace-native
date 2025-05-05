#include "cards.h"

#include "card_base.h"
#include "config.h"
#include "entity.h"
#include "helpers.h"
#include "page_items.h"
#include "translations.h"
#include "types.h"
#include <string>
#include <memory>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCard ===============
 */

void GridCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== EntitiesCard ===============
 */

void EntitiesCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== QRCard ===============
 */

void QRCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

std::string &QRCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->qr_text_);

  for (auto& item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }

  return buffer;
}

/*
 * =============== AlarmCard ===============
 */

AlarmCard::AlarmCard(
  const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity) :
    Card(page_type::cardAlarm, uuid),
    alarm_entity_(alarm_entity),
    show_keypad_(true), status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), icon_t::shield_off, 0x0CE6)); //green
  this->info_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_i"), icon_t::progress_alert, 0xED80)); //orange
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"),
      button_type::disarm, get_translation(translation_item::disarm)));
}
AlarmCard::AlarmCard(
  const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity,
  const std::string &title) :
    Card(page_type::cardAlarm, uuid, title),
    alarm_entity_(alarm_entity),
    show_keypad_(true),status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), icon_t::shield_off, 0x0CE6)); //green
  this->info_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_i"), icon_t::progress_alert, 0xED80)); //orange
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"),
      button_type::disarm, get_translation(translation_item::disarm)));
}
AlarmCard::AlarmCard(
    const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity,
    const std::string &title, const uint16_t sleep_timeout) :
    Card(page_type::cardAlarm, uuid, title, sleep_timeout),
    alarm_entity_(alarm_entity),
    show_keypad_(true),status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), icon_t::shield_off, 0x0CE6)); //green
  this->info_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_i"), icon_t::progress_alert, 0xED80)); //orange
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"), 
      button_type::disarm, get_translation(translation_item::disarm)));
}

AlarmCard::~AlarmCard() {
  alarm_entity_->remove_subscriber(this);
}

void AlarmCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

bool AlarmCard::add_arm_button(alarm_arm_action action) {
  if (this->items_.size() >= 4) {
    return false;
  }

  const char *action_type = nullptr;
  switch(action) {
    case alarm_arm_action::arm_home:
      action_type = button_type::armHome;
      break;
    case alarm_arm_action::arm_away:
      action_type = button_type::armAway;
      break;
    case alarm_arm_action::arm_night:
      action_type = button_type::armNight;
      break;
    case alarm_arm_action::arm_vacation:
      action_type = button_type::armVacation;
      break;
    case alarm_arm_action::arm_custom_bypass:
      action_type = button_type::armCustomBypass;
      break;
  }

  this->items_.push_back(
    std::unique_ptr<AlarmButtonItem>(
      new AlarmButtonItem(
        std::string(this->uuid_).append(1, '_').append(action_type), 
        action_type, get_translation(action_type))));
  return true;
}

void AlarmCard::on_entity_state_change(const std::string &state) {
  this->status_icon_flashing_ = false;

  if (state == entity_state::triggered || 
      state == entity_state::arming || 
      state == entity_state::disarming || 
      state == entity_state::pending) {
    this->status_icon_flashing_ = true;
  }

  Icon icon{};
  icon.color = 38066u; //grey
  try_get_value(ALARM_ICON_MAP, icon, state);
  this->status_icon_->set_icon_color(icon.color);
  this->status_icon_->set_icon_value(icon.value);
}

void AlarmCard::on_entity_attribute_change(ha_attr_type attr, const std::string &value) {
  if (attr == ha_attr_type::code_arm_required) {
    this->set_show_keypad(value != entity_state::off);
  }
}

std::string &AlarmCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->alarm_entity_->get_entity_id());

  if (this->alarm_entity_->is_state(entity_state::unknown) ||
      this->alarm_entity_->is_state(entity_state::disarmed)) {
    for (auto& item : this->items_) {
      buffer.append(1, SEPARATOR).append(item->render());
    }
    if (this->items_.size() < 4) {
      buffer.append(2 * (4 - this->items_.size()), SEPARATOR);
    }
  } else {
    buffer.append(1, SEPARATOR).append(this->disarm_button_->render());
    buffer.append(2 * 3, SEPARATOR);
  }

  buffer.append(1, SEPARATOR).append(this->status_icon_->render());

  // Only disable keypad when alarm is disarmed, since arming always requires code
  if (this->alarm_entity_->is_state(entity_state::disarmed)) {
        buffer.append(1, SEPARATOR)
        .append(this->show_keypad_ ? 
          generic_type::enable : generic_type::disable);
  } else {
    buffer.append(1, SEPARATOR)
        .append(generic_type::enable);
  }

  buffer.append(1, SEPARATOR)
    .append(this->status_icon_flashing_ ? 
      generic_type::enable : generic_type::disable);
  
  // todo: not finished/tested
  auto &open_sensors = this->alarm_entity_->get_attribute(ha_attr_type::open_sensors);
  if (!open_sensors.empty()) {
    buffer.append(1, SEPARATOR).append(this->info_icon_->render());
  }

  return buffer;
}

/*
 * =============== ThermoCard ===============
 */

ThermoCard::ThermoCard(const std::string &uuid,
    const std::shared_ptr<Entity> &thermo_entity) :
    Card(page_type::cardThermo, uuid),
    thermo_entity_(thermo_entity) {
  this->configure_temperature_unit();
  thermo_entity->add_subscriber(this);
}

ThermoCard::ThermoCard(const std::string &uuid,
    const std::shared_ptr<Entity> &thermo_entity,
    const std::string &title) :
    Card(page_type::cardThermo, uuid, title),
    thermo_entity_(thermo_entity) {
  this->configure_temperature_unit();
  thermo_entity->add_subscriber(this);
}

ThermoCard::ThermoCard(
    const std::string &uuid,
    const std::shared_ptr<Entity> &thermo_entity,
    const std::string &title, const uint16_t sleep_timeout) :
    Card(page_type::cardThermo, uuid, title, sleep_timeout),
    thermo_entity_(thermo_entity) {
  this->configure_temperature_unit();
  thermo_entity->add_subscriber(this);
}

ThermoCard::~ThermoCard() {
  thermo_entity_->remove_subscriber(this);
}

void ThermoCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

void ThermoCard::configure_temperature_unit() {
  if (Configuration::get_temperature_unit() == temperature_unit_t::celcius) {
    this->temperature_unit_icon_ = icon_t::temperature_celsius;
  } else {
    this->temperature_unit_icon_ = icon_t::temperature_fahrenheit;
  }
}

std::string &ThermoCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->thermo_entity_->get_entity_id());
  buffer.append(1, SEPARATOR);

  buffer.append(this->thermo_entity_->get_attribute(
    ha_attr_type::current_temperature));
  buffer.append(1, ' ');
  
  buffer.append(Configuration::get_temperature_unit_str());
  buffer.append(1, SEPARATOR);

  std::string dest_temp_str = 
    this->thermo_entity_->get_attribute(ha_attr_type::temperature);
  std::string dest_temp2_str;

  if (dest_temp_str.empty()) {
    dest_temp_str = this->thermo_entity_->get_attribute(
      ha_attr_type::target_temp_high, "0");
    dest_temp2_str = this->thermo_entity_->get_attribute(
      ha_attr_type::target_temp_low);
    if (!dest_temp2_str.empty()) {
      dest_temp2_str = std::to_string(
        static_cast<int>(std::stof(dest_temp2_str) * 10));
    }
  }
  dest_temp_str = std::to_string(
    static_cast<int>(std::stof(dest_temp_str) * 10));

  buffer.append(dest_temp_str).append(1, SEPARATOR);

  auto hvac_action = this->thermo_entity_->get_attribute(
    ha_attr_type::hvac_action);
  if (!hvac_action.empty()) {
    buffer
      // frontend.state_attributes.climate.hvac_action
      .append(get_translation(hvac_action))
      .append("\r\n(");
  }
  // backend.component.climate.state
  buffer.append(get_translation(this->thermo_entity_->get_state()));
  if (!hvac_action.empty()) {
    buffer.append(1, ')');
  }
  buffer.append(1, SEPARATOR);

  buffer.append(std::to_string(static_cast<int>(
    std::stof(this->thermo_entity_->get_attribute(
      ha_attr_type::min_temp, "0")) * 10)));
  buffer.append(1, SEPARATOR);

  buffer.append(std::to_string(static_cast<int>(
    std::stof(this->thermo_entity_->get_attribute(
      ha_attr_type::max_temp, "0")) * 10)));
  buffer.append(1, SEPARATOR);

  buffer.append(std::to_string(static_cast<int>(
    std::stof(this->thermo_entity_->get_attribute(
      ha_attr_type::target_temp_step, "0.5")) * 10)));
  
  //TODO: add overwrite_supported_modes
  auto& hvac_modes_str = 
    this->thermo_entity_->get_attribute(ha_attr_type::hvac_modes);
  if (hvac_modes_str.empty()) {
    buffer.append(4 * 8, SEPARATOR);
  } else {
    std::vector<std::string> hvac_modes;
    hvac_modes.reserve(6);
    split_str(',', hvac_modes_str, hvac_modes);

    for (auto& mode : hvac_modes) {
      uint16_t active_colour = 64512U; //dark orange
      if (mode == entity_state::auto_ ||
          mode == entity_state::heat_cool) {
        active_colour = 1024U; //dark green
      } else if (mode == entity_state::off ||
          mode == entity_state::fan_only) {
        active_colour = 52857U; // light grey (was: muddy grey|35921)
      } else if (mode == entity_state::cool) {
        active_colour = 11487U; //light blue
      } else if (mode == entity_state::dry) {
        active_colour = 60897U; //light orange
      }
      buffer.append(1, SEPARATOR);
      buffer.append(CHAR8_CAST(get_icon(CLIMATE_ICON_MAP, mode))).append(1, SEPARATOR);
      buffer.append(std::to_string(active_colour)).append(1, SEPARATOR);
      buffer.append(1, this->thermo_entity_->is_state(mode) ? '1' : '0');
      buffer.append(1, SEPARATOR);
      buffer.append(mode);
    }
    
    // todo: disperse icons evenly based on size of hvac_modes
    buffer.append(4 * (8 - hvac_modes.size()), SEPARATOR);
  }

  buffer.append(1, SEPARATOR);

  buffer.append(get_translation(translation_item::currently)).append(1, SEPARATOR);
  buffer.append(get_translation(translation_item::state)).append(1, SEPARATOR);
  // buffer.append(get_translation(translation_item::action)).append(1, SEPARATOR); // depreciated
  buffer.append(1, SEPARATOR);
  buffer.append(CHAR8_CAST(this->temperature_unit_icon_)).append(1, SEPARATOR);
  buffer.append(dest_temp2_str).append(1, SEPARATOR);
  
  if (this->thermo_entity_->has_attribute(ha_attr_type::preset_modes) || 
      this->thermo_entity_->has_attribute(ha_attr_type::swing_modes) || 
      this->thermo_entity_->has_attribute(ha_attr_type::fan_modes)) {
    buffer.append(1, '0');
  } else {
    buffer.append(1, '1');
  }

  return buffer;
}

/*
 * =============== MediaCard ===============
 */

MediaCard::MediaCard(const std::string &uuid,
    const std::shared_ptr<Entity> &media_entity) :
    Card(page_type::cardMedia, uuid),
    media_entity_(media_entity) {
  media_entity->add_subscriber(this);
}

MediaCard::MediaCard(const std::string &uuid,
    const std::shared_ptr<Entity> &media_entity,
    const std::string &title) :
    Card(page_type::cardMedia, uuid, title),
    media_entity_(media_entity) {
  media_entity->add_subscriber(this);
}

MediaCard::MediaCard(const std::string &uuid,
    const std::shared_ptr<Entity> &media_entity,
    const std::string &title, const uint16_t sleep_timeout) :
    Card(page_type::cardMedia, uuid, title, sleep_timeout),
    media_entity_(media_entity) {
  media_entity->add_subscriber(this);
}

MediaCard::~MediaCard() {
  media_entity_->remove_subscriber(this);
}

void MediaCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

// entityUpd~{heading}~{navigation}~{entityId}~{title}~~{author}~~{volume}~{iconplaypause}~{onoffbutton}~{shuffleBtn}{media_icon}{item_str}
std::string &MediaCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->media_entity_->get_entity_id());
  buffer.append(1, SEPARATOR);

  buffer.append(this->media_entity_->get_attribute(
    ha_attr_type::media_title).substr(0, 40));
  buffer.append(2, SEPARATOR);

  buffer.append(this->media_entity_->get_attribute(
    ha_attr_type::media_artist).substr(0, 40));
  buffer.append(2, SEPARATOR);

  buffer.append(std::to_string(
    static_cast<uint8_t>(std::stof(this->media_entity_->get_attribute(
      ha_attr_type::volume_level, "0")) * 100.0f)));
  buffer.append(1, SEPARATOR);

  auto icon = this->media_entity_->is_state(entity_state::playing)
    ? icon_t::pause : icon_t::play;
  buffer.append(CHAR8_CAST(icon)).append(1, SEPARATOR);

  uint32_t supported_features = value_or_default(this->media_entity_->
    get_attribute(ha_attr_type::supported_features), 0UL);

  // on/off button colour
  if (supported_features & 0b10000000) {
    if (this->media_entity_->is_state(entity_state::off))
      buffer.append(std::to_string(1374)); // light blue
    else
      buffer.append(std::to_string(64704)); // orange
  } else {
    buffer.append(generic_type::disable);
  }
  buffer.append(1, SEPARATOR);
  
  // shuffle button icon
  if (supported_features & 0b100000000000000) {
    if (this->media_entity_->
        get_attribute(ha_attr_type::shuffle) == entity_state::on)
      buffer.append(CHAR8_CAST(icon_t::shuffle));
    else
      buffer.append(CHAR8_CAST(icon_t::shuffle_disable));
  } else {
    buffer.append(generic_type::disable);
  }
  buffer.append(1, SEPARATOR);

  // todo: create PageItem for this?
  // media icon/button: type~internalName~icon~iconColor~displayName~
  //      on btn press: event,buttonPress2,{entity_id},button
  buffer.append(entity_render_type::media_pl).append(1, SEPARATOR);
  buffer.append(this->media_entity_->get_entity_id()).append(1, SEPARATOR);
  auto media_icon = get_value_or_default(MEDIA_TYPE_ICON_MAP, 
    this->media_entity_->get_attribute(ha_attr_type::media_content_type),
    icon_t::speaker_off);
  buffer.append(CHAR8_CAST(media_icon)).append(1, SEPARATOR);
  buffer.append(std::to_string(17299U)).append(2, SEPARATOR);
  
  for (auto& item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }
  if (this->items_.size() > 0) buffer.append(1, SEPARATOR);

  return buffer;
}

} // namespace nspanel_lovelace
} // namespace esphome