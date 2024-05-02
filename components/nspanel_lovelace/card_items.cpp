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
    ha_attr_type attr, const std::string &value) {
  StatefulPageItem::on_entity_attribute_change(attr, value);

  // this class only needs to react to the following attributes
  if (attr == ha_attr_type::unit_of_measurement) {
    this->set_value_postfix(value);
  } else if (this->is_type(entity_type::cover)) {
    if (attr == ha_attr_type::current_position) {
      // this is a cheat/shortcut to avoid state change spamming,
      // remove this if it doesn't work
      if (!value.empty() && value != "100" && value != "0") return;
    }
    // any attribute change requires re-evaluating other attributes too,
    // so it is easier to execute the state callback to handle all changes
    if (this->on_state_callback_) {
      this->on_state_callback_(this);
      this->set_render_invalid();
    }
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

void EntitiesCardEntityItem::state_cover_fn(StatefulPageItem *me) {
  StatefulPageItem::state_cover_fn(me);
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);

  auto cover_icons = get_icon_by_name(
    COVER_MAP,
    me_->entity_->get_attribute(ha_attr_type::device_class),
    entity_cover_type::window);
  auto &position_str = me_->entity_->
    get_attribute(ha_attr_type::current_position);
  auto &supported_features_str = me_->entity_->
    get_attribute(ha_attr_type::supported_features);

  uint8_t position = 0;
  uint8_t supported_features = 0;
  const char* icon_up   = generic_type::disable;
  const char* icon_stop = generic_type::disable;
  const char* icon_down = generic_type::disable;
  bool icon_up_status = false;
  bool icon_stop_status = false;
  bool icon_down_status = false;

  if (!position_str.empty()) {
    position = std::stoi(position_str);
  }
  if (!supported_features_str.empty()) {
    supported_features = std::stoi(supported_features_str);
  }

  // see: https://github.com/home-assistant/core/blob/dev/homeassistant/components/cover/__init__.py#L112
  // OPEN
  if (supported_features & 0b1) {
    if (position != 100 && !((me_->entity_->is_state("open") ||
        me_->entity_->is_state(generic_type::unknown)) &&
        position_str.empty())) {
      icon_up_status = true;
    }
    icon_up = cover_icons->at(2);
  }
  // CLOSE
  if (supported_features & 0b10) {
    if (position != 0 && !((me_->entity_->is_state("closed") ||
        me_->entity_->is_state(generic_type::unknown)) &&
        position_str.empty())) {
      icon_down_status = true;
    }
    icon_down = cover_icons->at(3);
  }
  // STOP
  if (supported_features & 0b1000) {
    icon_stop_status = !me_->entity_->is_state(generic_type::unknown);
    icon_stop = u8"\uE4DA"; // stop
  }
  me_->value_
    .assign(icon_up).append(1, '|')
    .append(icon_stop).append(1, '|')
    .append(icon_down).append(1, '|')
    .append(icon_up_status ? generic_type::enable : generic_type::disable)
    .append(1, '|')
    .append(icon_stop_status ? generic_type::enable : generic_type::disable)
    .append(1, '|')
    .append(icon_down_status ? generic_type::enable : generic_type::disable);
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
  } else if (type == entity_type::cover) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_cover_fn;
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