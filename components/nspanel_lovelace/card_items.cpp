#include "card_items.h"

#include "config.h"
#include "card_base.h"
#include "helpers.h"
#include "translations.h"
#include "types.h"
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
  this->on_entity_type_change(this->get_type());
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

EntitiesCardEntityItem::EntitiesCardEntityItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &display_name) :
    CardItem(uuid, std::move(entity), display_name),
    PageItem_Value(this) {
  // todo: fix this - needs to be called to ensure overloaded set_on_state_callback_ is called
  this->on_entity_type_change(this->get_type());
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

void EntitiesCardEntityItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

void EntitiesCardEntityItem::on_entity_attribute_change(
    ha_attr_type attr, const std::string &value) {
  StatefulPageItem::on_entity_attribute_change(attr, value);

  if (attr == ha_attr_type::unit_of_measurement) {
    this->set_value_postfix(value);
    return;
  }

  // Sometimes attribute changes also require updating the state for certain
  // entity types seen below
  if (!this->on_state_callback_) return;

  if (this->is_type(entity_type::cover)) {
    if (attr == ha_attr_type::current_position) {
      // this is a cheat/shortcut to avoid state change spamming
      if (!value.empty() && value != "100" && value != "0") {
        return;
      }
    }
  } else if (this->is_type(entity_type::climate)) {
    // Only these two attributes affect the visual output
    // so avoid the state callback for anything else
    if (attr != ha_attr_type::temperature &&
        attr != ha_attr_type::current_temperature) {
      return;
    }
  } else if (
      this->is_type(entity_type::number) ||
      this->is_type(entity_type::input_number)) {
    if (attr != ha_attr_type::min &&
        attr != ha_attr_type::max) {
      return;
    }
  } else if (this->is_type(entity_type::weather)) {
    if (attr != ha_attr_type::temperature &&
        attr != ha_attr_type::temperature_unit) {
      return;
    }
  } else if (this->is_type(entity_type::media_player)) {
    // All attribute updates effect render output for this entity
  } else {
    // Any entity type not mentioned above doesn't need re-rendering
    return;
  }

  this->on_state_callback_(this);
  this->set_render_invalid();
}

void EntitiesCardEntityItem::state_generic_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = get_translation(me_->get_state());
}

void EntitiesCardEntityItem::state_on_off_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = (me_->is_state(entity_state::on) ? "1" : "0");
  StatefulPageItem::state_on_off_fn(me);
}

void EntitiesCardEntityItem::state_button_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  // frontend.ui.card.button.press
  me_->value_ = get_translation(translation_item::press);
}

void EntitiesCardEntityItem::state_scene_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = get_translation(translation_item::activate);
}

void EntitiesCardEntityItem::state_script_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = get_translation(translation_item::run);
}

void EntitiesCardEntityItem::state_timer_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  auto &state = me_->get_entity()->get_state();
  // backend.component.timer.state
  me_->value_ = get_translation(state);
}

void EntitiesCardEntityItem::state_cover_fn(StatefulPageItem *me) {
  StatefulPageItem::state_cover_fn(me);
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);

  std::array<const char *, 4> cover_icons{};
  bool cover_icons_found = try_get_value(COVER_MAP,
    cover_icons,
    me_->get_attribute(ha_attr_type::device_class),
    entity_cover_type::window);
  auto &position_str = me_->get_attribute(
    ha_attr_type::current_position);
  auto &supported_features_str = me_->get_attribute(
    ha_attr_type::supported_features);

  uint8_t position = 0;
  uint8_t supported_features = 0;
  bool icon_up_status = false;
  bool icon_stop_status = false;
  bool icon_down_status = false;

  me_->value_.clear();

  if (!position_str.empty()) {
    position = std::stoi(position_str);
  }
  if (!supported_features_str.empty()) {
    supported_features = std::stoi(supported_features_str);
  }

  // see: https://github.com/home-assistant/core/blob/dev/homeassistant/components/cover/__init__.py#L112
  // OPEN
  if (supported_features & 0b1) {
    if (position != 100 && !((me_->is_state(entity_state::open) ||
        me_->is_state(entity_state::unknown)) &&
        position_str.empty())) {
      icon_up_status = true;
    }
    if (cover_icons_found)
      me_->value_.append(cover_icons.at(2));
  }
  me_->value_.append(1, '|');
  // STOP
  if (supported_features & 0b1000) {
    icon_stop_status = !me_->is_state(entity_state::unknown);
    me_->value_.append(icon_t::stop);
  }
  me_->value_.append(1, '|');
  // CLOSE
  if (supported_features & 0b10) {
    if (position != 0 && !((me_->is_state(entity_state::closed) ||
        me_->is_state(entity_state::unknown)) &&
        position_str.empty())) {
      icon_down_status = true;
    }
    if (cover_icons_found)
      me_->value_.append(cover_icons.at(3));
  }
  me_->value_
    .append(1, '|')
    .append(icon_up_status ? generic_type::enable : generic_type::disable)
    .append(1, '|')
    .append(icon_stop_status ? generic_type::enable : generic_type::disable)
    .append(1, '|')
    .append(icon_down_status ? generic_type::enable : generic_type::disable);
}

void EntitiesCardEntityItem::state_climate_fn(StatefulPageItem *me) {
  StatefulPageItem::state_climate_fn(me);
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);

  // backend.component.climate.state
  me_->value_.assign(get_translation(me_->get_state()));
  if (me_->is_state(entity_state::unknown)) return;
  
  auto temp_unit = Configuration::get_temperature_unit_str();
  auto temp = me_->get_attribute(ha_attr_type::temperature);
  if (!temp.empty()) {
    me_->value_.append(1, ' ').append(temp).append(temp_unit);
  }
  me_->value_.append("\r\n")
    .append(get_translation(translation_item::currently)).append(": ")
    .append(me_->get_attribute(ha_attr_type::current_temperature))
    .append(temp_unit);
}

void EntitiesCardEntityItem::state_number_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  auto &state = me_->get_entity()->get_state();
  auto min = me_->get_attribute(ha_attr_type::min, "0");
  auto max = me_->get_attribute(ha_attr_type::max, "100");
  
  me_->value_.assign(state)
    .append(1, '|').append(min)
    .append(1, '|').append(max);
}

void EntitiesCardEntityItem::state_lock_fn(StatefulPageItem *me) {
  StatefulPageItem::state_lock_fn(me);
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = get_translation(me_->is_state(entity_state::unlocked) ?
    translation_item::lock : translation_item::unlock);
}

void EntitiesCardEntityItem::state_weather_fn(StatefulPageItem *me) {
  StatefulPageItem::state_weather_fn(me);
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  auto temperature = me_->get_attribute(
    ha_attr_type::temperature, "0");
  auto temperature_unit = me_->get_attribute(
    ha_attr_type::temperature_unit,
    Configuration::get_temperature_unit_str());
  
  me_->value_.assign(temperature).append(temperature_unit);
}

void EntitiesCardEntityItem::state_sun_fn(StatefulPageItem *me) {
  StatefulPageItem::state_sun_fn(me);
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  // backend.component.sun.state
  me_->value_ = get_translation(me_->get_state());
}

void EntitiesCardEntityItem::state_vacuum_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  me_->value_ = get_translation(me_->is_state(entity_state::docked) ?
    translation_item::start_cleaning : translation_item::return_to_base);
}

void EntitiesCardEntityItem::state_translate_fn(StatefulPageItem *me) {
  auto me_ = static_cast<EntitiesCardEntityItem*>(me);
  // Firstly try to find a match for a specific entity type, then
  // find a match for the generic state, otherwise use the raw state value
  const char *ret;
  std::string key = me->get_type();
  key.append(1, '.').append(me_->get_state());
  if (!try_get_value(TRANSLATION_MAP, ret, key)) {
    if (!try_get_value(TRANSLATION_MAP, ret, me_->get_state())) {
      me_->value_ = me_->get_state();
      return;
    }
  }
  me_->value_ = ret;
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
  } else if (
      type == entity_type::script ||
      type == entity_type::service) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_script_fn;
  } else if (type == entity_type::timer) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_timer_fn;
  } else if (type == entity_type::cover) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_cover_fn;
  } else if (type == entity_type::climate) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_climate_fn;
  } else if (
      type == entity_type::number ||
      type == entity_type::input_number) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_number_fn;
  } else if (type == entity_type::lock) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_lock_fn;
  } else if (type == entity_type::weather) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_weather_fn;
  } else if (type == entity_type::sun) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_sun_fn;
  } else if (type == entity_type::vacuum) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_vacuum_fn;
  } else if (
      type == entity_type::person ||
      type == entity_type::alarm_control_panel ||
      type == entity_type::binary_sensor) {
    this->on_state_callback_ = EntitiesCardEntityItem::state_translate_fn;
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