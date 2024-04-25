#include "card_items.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include "card_base.h"

#include <type_traits>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCardEntityItem ===============
 */

GridCardEntityItem::GridCardEntityItem(
    const std::string &uuid, std::shared_ptr<Entity> entity) : 
    CardItem(uuid, std::move(entity)) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

GridCardEntityItem::GridCardEntityItem(
    const std::string &uuid, std::shared_ptr<Entity> entity, 
    const std::string &display_name) : 
    CardItem(uuid, std::move(entity), display_name) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

void GridCardEntityItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== EntitiesCardEntityItem ===============
 */

EntitiesCardEntityItem::EntitiesCardEntityItem(
    const std::string &uuid, std::shared_ptr<Entity> entity) :
    CardItem(uuid, std::move(entity)), PageItem_Value(this) {
  // todo: fix this - needs to be called to ensure overloaded set_on_state_callback_ is called
  this->on_entity_type_change(this->entity_->get_type());
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

EntitiesCardEntityItem::EntitiesCardEntityItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &display_name) :
    CardItem(uuid, std::move(entity), display_name),
    PageItem_Value(this) {
  // todo: fix this - needs to be called to ensure overloaded set_on_state_callback_ is called
  this->on_entity_type_change(this->entity_->get_type());
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

void EntitiesCardEntityItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

void EntitiesCardEntityItem::on_entity_attribute_change(
    const char *attr, const std::string &value) {
  StatefulPageItem::on_entity_attribute_change(attr, value);

  // this class only needs to react to the following attributes
  if (attr == ha_attr_type::unit_of_measurement) {
    this->set_value_postfix(value);
  }
}

void EntitiesCardEntityItem::state_generic_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = me_->entity_->get_state();
}
void EntitiesCardEntityItem::state_on_off_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = (me_->entity_->is_state(generic_type::on) ? "1" : "0");
  StatefulPageItem::state_on_off_fn(me);
}
void EntitiesCardEntityItem::state_button_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  // frontend.ui.card.button.press
  me_->value_ = "Press";
}
void EntitiesCardEntityItem::state_scene_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = "Activate";
}
void EntitiesCardEntityItem::state_script_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = "Run";
}
void EntitiesCardEntityItem::state_timer_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  auto &state = me_->get_entity()->get_state();
  // backend.component.timer.state
  if (state == "idle")
    me_->value_ = "Idle";
  else if (state == "paused")
    me_->value_ = "Paused";
  else if (state == "active")
    me_->value_ = "Active";
  else
    me_->value_ = state;
}

void EntitiesCardEntityItem::set_on_state_callback_(const char *type) {
  if (type == entity_type::light ||
      type == entity_type::switch_ ||
      type == entity_type::input_boolean ||
      type == entity_type::automation ||
      type == entity_type::fan) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_on_off_fn;
  } else if (
      type == entity_type::button ||
      type == entity_type::input_button ||
      type == entity_type::navigate) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_button_fn;
  } else if (type == entity_type::scene) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_scene_fn;
  } else if (type == entity_type::script) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_script_fn;
  } else if (type == entity_type::timer) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_timer_fn;
  } else {
    this->on_state_callback_ = EntitiesCardEntityItem::state_generic_fn;
  }
}

std::string &EntitiesCardEntityItem::render_(std::string &buffer) {
  CardItem::render_(buffer);
  return PageItem_Value::render_(buffer);
}

uint16_t EntitiesCardEntityItem::get_render_buffer_reserve_() const {
  // try to guess the required size of the buffer to reduce heap fragmentation
  return CardItem::get_render_buffer_reserve_() +
         this->value_.length() + this->value_postfix_.length() + 2;
}

} // namespace nspanel_lovelace
} // namespace esphome