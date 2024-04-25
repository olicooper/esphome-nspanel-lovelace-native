#include "cards.h"

#include "config.h"
#include "entity.h"
#include "helpers.h"
#include "types.h"
#include "card_base.h"
#include "page_items.h"
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
    new AlarmIconItem(std::string(uuid).append("_s"), u8"\uE99D", 3302)); //shield-off, green
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"),
      button_type::disarm, "Disarm"));
}
AlarmCard::AlarmCard(
  const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity,
  const std::string &title) :
    Card(page_type::cardAlarm, uuid, title),
    alarm_entity_(alarm_entity),
    show_keypad_(true),status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), u8"\uE99D", 3302)); //shield-off, green
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"),
      button_type::disarm, "Disarm"));
}
AlarmCard::AlarmCard(
    const std::string &uuid, const std::shared_ptr<Entity> &alarm_entity,
    const std::string &title, const uint16_t sleep_timeout) :
    Card(page_type::cardAlarm, uuid, title, sleep_timeout),
    alarm_entity_(alarm_entity),
    show_keypad_(true),status_icon_flashing_(false) {
  alarm_entity_->add_subscriber(this);
  this->status_icon_ = std::unique_ptr<AlarmIconItem>(
    new AlarmIconItem(std::string(uuid).append("_s"), u8"\uE99D", 3302)); //shield-off, green
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(uuid).append("_d"), 
      button_type::disarm, "Disarm"));
}

AlarmCard::~AlarmCard() {
  alarm_entity_->remove_subscriber(this);
}

void AlarmCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

bool AlarmCard::set_arm_button(
    alarm_arm_action action, const std::string &display_name) {
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
  }

  this->items_.push_back(
    std::unique_ptr<AlarmButtonItem>(
      new AlarmButtonItem(
        std::string(this->uuid_).append(1, '_').append(action_type), 
        action_type, display_name)));
  return true;
}

void AlarmCard::set_disarm_button(const std::string &display_name) {
  this->disarm_button_.reset();
  this->disarm_button_ = std::unique_ptr<AlarmButtonItem>(
    new AlarmButtonItem(std::string(this->uuid_).append("_d"),
      button_type::disarm, display_name));
}

void AlarmCard::on_entity_state_change(const std::string &state) {
  this->status_icon_flashing_ = false;

  if (state == alarm_entity_state::disarmed || 
      state == generic_type::unknown) {
    this->status_icon_->reset_icon_color(); //green
    this->status_icon_->reset_icon_value(); //shield-off
  } else if (state == alarm_entity_state::triggered) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE09D"); //bell-ring
    this->status_icon_flashing_ = true;
  } else if (state == alarm_entity_state::armed_home) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE689"); //shield-home
  } else if (state == alarm_entity_state::armed_away) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE99C"); //shield-lock
  } else if (state == alarm_entity_state::armed_night) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uF827"); //shield-moon (was E593:weather-night)
  } else if (state == alarm_entity_state::armed_vacation) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE6BA"); //shield-airplane
  } else if (state == alarm_entity_state::armed_custom_bypass) {
    this->status_icon_->set_icon_color(0xE243); //red
    this->status_icon_->set_icon_value(u8"\uE497"); //shield
  } else if (state == alarm_entity_state::arming || 
      state == alarm_entity_state::pending) {
    this->status_icon_->set_icon_color(0xED80); //orange
    this->status_icon_->set_icon_value(u8"\uE497"); //shield
    this->status_icon_flashing_ = true;
  } else {
    this->status_icon_->set_icon_color(38066u); //grey
    this->status_icon_->set_icon_value(u8"\uE624"); //help-circle-outline
  }
}

void AlarmCard::on_entity_attribute_change(const char *attr, const std::string &value) {
  if (attr == ha_attr_type::code_arm_required) {
    this->set_show_keypad(value != generic_type::off);
  }
}

std::string &AlarmCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer).append(1, SEPARATOR);

  buffer.append(this->alarm_entity_->get_entity_id());

  if (this->alarm_entity_->is_state(generic_type::unknown) ||
      this->alarm_entity_->is_state(alarm_entity_state::disarmed)) {
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

  buffer.append(1, SEPARATOR)
    .append(this->show_keypad_ ? 
      generic_type::enable : generic_type::disable);

  buffer.append(1, SEPARATOR)
    .append(this->status_icon_flashing_ ? 
      generic_type::enable : generic_type::disable);
  
  // todo: 
  //   if "open_sensors" in entity.attributes and entity.attributes.open_sensors is not None:
  // if (this->info_icon_ && /* ?? */) {
  //   buffer.append(1, SEPARATOR).append(this->info_icon_->render());
  // }

  return buffer;
}

} // namespace nspanel_lovelace
} // namespace esphome