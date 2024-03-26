#include "card_items.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include "card_base.h"
#include <stdint.h>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCardEntityItem ===============
 */

const uint32_t GridCardEntityItem::this_class_type_ = 
    // PageItem | CardItem | StatefulCardItem | GridCardEntityItem
    (1<<0) | (1<<1) | (1<<2) | (1<<6);

/*
 * =============== EntitiesCardEntityItem ===============
 */

const uint32_t EntitiesCardEntityItem::this_class_type_ =
    // PageItem | CardItem | StatefulCardItem | EntitiesCardEntityItem
    (1<<0) | (1<<1) | (1<<2) | (1<<7);

EntitiesCardEntityItem::EntitiesCardEntityItem(
    const std::string &uuid, const std::string &entity_id) :
    StatefulCardItem(uuid), PageItem_Value(this) {
  this->set_entity_id(entity_id);
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

EntitiesCardEntityItem::EntitiesCardEntityItem(
    const std::string &uuid, const std::string &entity_id,
    const std::string &display_name) :
    StatefulCardItem(uuid, display_name),
    PageItem_Value(this) {
  this->set_entity_id(entity_id);
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

void EntitiesCardEntityItem::state_generic_fn(StatefulCardItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = me_->state_;
}
void EntitiesCardEntityItem::state_on_off_fn(StatefulCardItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = (me_->state_ == "on" ? "1" : "0");
  StatefulCardItem::state_on_off_fn(me);
}
void EntitiesCardEntityItem::state_button_fn(StatefulCardItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = "Press";
  // value_ = get_translation(self._locale, "frontend.ui.card.button.press")
}
void EntitiesCardEntityItem::state_scene_fn(StatefulCardItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = "Activate";
}
void EntitiesCardEntityItem::state_script_fn(StatefulCardItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = "Run";
}

bool EntitiesCardEntityItem::set_type(const char *type) {
  if (!StatefulCardItem::set_type(type))
    return false;

  if (this->type_ == entity_type::light ||
      this->type_ == entity_type::switch_ ||
      this->type_ == entity_type::input_boolean ||
      this->type_ == entity_type::automation ||
      this->type_ == entity_type::fan) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_on_off_fn;
  } else if (
      this->type_ == entity_type::button ||
      this->type_ == entity_type::input_button ||
      this->type_ == entity_type::navigate) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_button_fn;
  } else if (this->type_ == entity_type::scene) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_scene_fn;
  } else if (this->type_ == entity_type::script) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_script_fn;
  } else {
    this->on_state_callback_ = EntitiesCardEntityItem::state_generic_fn;
  }

  this->set_render_invalid();

  // also need to make sure the state is updated based on the new 'type'
  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }

  return true;
}

void EntitiesCardEntityItem::set_state(const std::string &state) {
  this->state_ = state;

  this->set_render_invalid();

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

std::string &EntitiesCardEntityItem::render_(std::string &buffer) {
  StatefulCardItem::render_(buffer);
  return PageItem_Value::render_(buffer);
}

uint16_t EntitiesCardEntityItem::get_render_buffer_reserve_() const {
  // try to guess the required size of the buffer to reduce heap fragmentation
  return StatefulCardItem::get_render_buffer_reserve_() +
         this->value_.length() + this->value_postfix_.length() + 1;
}

} // namespace nspanel_lovelace
} // namespace esphome