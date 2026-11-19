#include "entity.h"

namespace esphome {
namespace nspanel_lovelace {

Entity::Entity(const std::string &entity_id) :
    state_(entity_state::unknown) {
  assert(!entity_id.empty());
  this->set_entity_id(entity_id);
  enable_notifications_ = true;
}
Entity::Entity(const std::string &entity_id, const char *type) : 
    type_(type), type_overridden_(true),
    state_(entity_state::unknown) {
  assert(!entity_id.empty() && type != nullptr);
  this->set_entity_id(entity_id);
  enable_notifications_ = true;
}

void Entity::add_subscriber(IEntitySubscriber *const target) {
  // for (auto t : this->targets_) {
  //     if (t == target) return;
  // }
  this->targets_.push_back(target);
}

bool Entity::remove_subscriber(const IEntitySubscriber *const target) {
  for (auto iter = this->targets_.begin(); iter != this->targets_.end(); ++iter) {
    if (*iter == target) {
      this->targets_.erase(iter);
      return true;
    }
  }
  // this->targets_.erase(
  //     std::remove(this->targets_.begin(), this->targets_.end(), target),
  //     this->targets_.end());
  return false;
}

const std::string &Entity::get_entity_id() const { return this->entity_id_; }

void Entity::set_entity_id(const std::string &entity_id) {
  if (entity_id.empty()) return;

  this->entity_id_ = entity_id;

  if (!this->type_overridden_) {
    if (!this->set_type(get_entity_type(this->entity_id_))) {
      // todo: should we be setting a fallback type?
      this->type_ = entity_type::text;
    }
  }

  // extract the text from iText entities
  // todo: remove this after creating a StaticTextItem
  if (this->is_type(entity_type::itext)) {
    auto pos = this->entity_id_.rfind('.', strlen(entity_type::itext) + 1);
    if (pos != std::string::npos && pos < this->entity_id_.length()) {
      this->set_state(this->entity_id_.substr(pos + 1));
    }
  }
}

const char *Entity::get_type() const { return this->type_; }

bool Entity::is_type(const char *type) const {
  if (type == nullptr || this->type_ == nullptr) return false;
  if (type == this->type_) return true;
  return std::strcmp(this->type_, type) == 0;
}

bool Entity::set_type(const char *type) {
  if (type == nullptr || type[0] == '\0') {
    return false;
  }
  if (this->type_ == type) return true;
  this->type_ = type;

  if (this->enable_notifications_) {
    this->notify_type_change(this->type_);
  }
  return true;
}

bool Entity::is_state(const std::string &state) const { return this->state_ == state; }

const std::string &Entity::get_state() const { return this->state_; }

void Entity::set_state(const std::string &state) {
  if (this->state_ == state) return;
  this->state_ = state;

  if (this->enable_notifications_) {
    this->notify_state_change(this->state_);
  }
}

bool Entity::has_attribute(ha_attr_type attr) const {
  return !attributes_[static_cast<size_t>(attr)].empty();
}

void Entity::set_attribute(ha_attr_type attr, const std::string &value) {
  auto &slot = attributes_[static_cast<size_t>(attr)];
  if (value.empty() || value == "None" || value == "none") {
    slot.clear();
    this->notify_attribute_change(attr, slot);
    return;
  }
  if (slot == value.c_str()) return;

  if (attr == ha_attr_type::brightness) {
    slot.assign(std::to_string(static_cast<int>(round(
        scale_value(str_to_int(value), {0, 255}, {0, 100})))));
  } else if (attr == ha_attr_type::color_temp) {
    auto minstr = this->get_attribute(ha_attr_type::min_mireds);
    auto maxstr = this->get_attribute(ha_attr_type::max_mireds);
    uint16_t min_mireds = minstr.empty() ? 153 : static_cast<uint16_t>(str_to_int(minstr));
    uint16_t max_mireds = maxstr.empty() ? 500 : static_cast<uint16_t>(str_to_int(maxstr));
    slot.assign(std::to_string(static_cast<int>(round(scale_value(
        str_to_int(value),
        {static_cast<double>(min_mireds), static_cast<double>(max_mireds)},
        {0, 100})))));
  } else if (attr == ha_attr_type::supported_color_modes ||
      attr == ha_attr_type::effect_list ||
      attr == ha_attr_type::preset_modes ||
      attr == ha_attr_type::swing_modes ||
      attr == ha_attr_type::fan_modes ||
      attr == ha_attr_type::hvac_modes ||
      // todo: this list can contain any value (including ones with commas),
      //       convert_python_arr_str does not support this scenario!
      attr == ha_attr_type::source_list ||
      attr == ha_attr_type::options) {
    // todo: remove this when esphome starts sending properly formatted array strings
    auto converted = convert_python_arr_str(value);

    // only store the first 14 effects as additional ones will never be rendered
    if (attr == ha_attr_type::effect_list) {
      auto split_pos = find_nth_of(',', 15, converted);
      if (split_pos != std::string::npos) {
        converted = converted.substr(0, split_pos);
      }
    }
    slot.assign(converted.c_str(), converted.size());
    slot.shrink_to_fit();
  } else {
    slot.assign(value.c_str(), value.size());
  }

  if (this->enable_notifications_) {
    this->notify_attribute_change(attr, slot);
  }
}

void Entity::notify_type_change(const char *type) {
  for (auto iter = this->targets_.begin(); iter != this->targets_.end(); ++iter) {
    (*iter)->on_entity_type_change(type);
  }
}

void Entity::notify_state_change(const std::string &state) {
  for (auto iter = this->targets_.begin(); iter != this->targets_.end(); ++iter) {
    (*iter)->on_entity_state_change(state);
  }
}

void Entity::notify_attribute_change(ha_attr_type attr, const psram_string &value) {
  for (auto iter = this->targets_.begin(); iter != this->targets_.end(); ++iter) {
    (*iter)->on_entity_attribute_change(attr, value);
  }
}

}
}