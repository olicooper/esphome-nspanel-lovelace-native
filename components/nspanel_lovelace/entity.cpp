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
    this->notify_type_change(type);
  }
  return true;
}

bool Entity::is_state(const std::string &state) const { return this->state_ == state; }

const std::string &Entity::get_state() const { return this->state_; }

void Entity::set_state(const std::string &state) {
  if (this->state_ == state) return;
  this->state_ = state;

  if (this->enable_notifications_) {
    this->notify_state_change(state);
  }
}

bool Entity::has_attribute(ha_attr_type attr) const {
  auto it = attributes_.find(attr);
  return it != attributes_.end();
}

const std::string &Entity::get_attribute(ha_attr_type attr, const std::string &default_value) const {
  auto it = attributes_.find(attr);
  return it == attributes_.end() ? default_value : it->second;
}

void Entity::set_attribute(ha_attr_type attr, const std::string &value) {
  if (value.empty() || value == "None" || value == "none") {
    attributes_.erase(attr);
    this->notify_attribute_change(attr, "");
    return;
  }
  if (this->attributes_[attr] == value) return;

  if (attr == ha_attr_type::brightness) {
    this->attributes_[attr] = std::to_string(static_cast<int>(round(
        scale_value(std::stoi(value), {0, 255}, {0, 100}))));
  } else if (attr == ha_attr_type::color_temp) {
    auto &minstr = this->get_attribute(ha_attr_type::min_mireds);
    auto &maxstr = this->get_attribute(ha_attr_type::max_mireds);
    uint16_t min_mireds = minstr.empty() ? 153 : std::stoi(minstr);
    uint16_t max_mireds = maxstr.empty() ? 500 : std::stoi(maxstr);
    this->attributes_[attr] = std::to_string(static_cast<int>(round(scale_value(
        std::stoi(value),
        {static_cast<double>(min_mireds), static_cast<double>(max_mireds)},
        {0, 100}))));
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
    this->attributes_[attr] = convert_python_arr_str(value);
    
    // only store the first 14 effects as additonal ones will never be rendered
    if (attr == ha_attr_type::effect_list) {
      auto split_pos = find_nth_of(',', 15, this->attributes_[attr]);
      if (split_pos != std::string::npos) {
        this->attributes_[attr] = this->attributes_[attr].substr(0, split_pos);
      }
    }
    this->attributes_[attr].shrink_to_fit();
  } else {
    this->attributes_[attr] = value;
  }

  if (this->enable_notifications_) {
    this->notify_attribute_change(attr, this->attributes_[attr]);
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

void Entity::notify_attribute_change(ha_attr_type attr, const std::string &value) {
  for (auto iter = this->targets_.begin(); iter != this->targets_.end(); ++iter) {
    (*iter)->on_entity_attribute_change(attr, value);
  }
}

}
}