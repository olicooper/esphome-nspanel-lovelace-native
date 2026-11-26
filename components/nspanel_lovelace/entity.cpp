#include "entity.h"

#include <algorithm>

namespace esphome {
namespace nspanel_lovelace {

ThermostatControls::ThermostatControls(const Entity *entity) {
  if (entity == nullptr) return;

  const auto bound = [](const auto &value, int fallback, bool lower, int &display) {
    double temp = 0.0;
    if (!str_to_double(value, temp)) return fallback;
    // Round toward the legal range; compare back in degrees to preserve exact tenths.
    if (temp < INT16_MIN / 10.0) return lower ? INT16_MIN : INT16_MIN - 1;
    if (temp > INT16_MAX / 10.0) return lower ? INT16_MAX + 1 : INT16_MAX;
    int tenths = static_cast<int>(temp * 10.0);
    if (lower && tenths / 10.0 < temp) ++tenths;
    if (!lower && tenths / 10.0 > temp) --tenths;
    return display = tenths;
  };
  this->min = bound(entity->get_attribute(ha_attr_type::min_temp), INT16_MIN, true, this->display_min);
  this->max = bound(entity->get_attribute(ha_attr_type::max_temp), INT16_MAX, false, this->display_max);

  float step = 0.0f;
  if (str_to_float(entity->get_attribute(ha_attr_type::target_temp_step), step) &&
      step > 0.0f && step <= INT16_MAX / 10.0) {
    this->step = std::max(1, static_cast<int>(step * 10.0f));
  }
}

bool ThermostatControls::normalize(int &low, int &high) const {
  if (low < INT16_MIN || high > INT16_MAX || low > high || this->min > this->max) return false;

  // v4.3.3 tests the displayed value before stepping. Only accept a crossing
  // if reversing one step lands on a value that could have crossed that bound.
  const auto accepts = [&](int value) {
    if (value < this->min) {
      return this->min == this->display_min && value + this->step > this->min && value + this->step <= this->max;
    }
    if (value > this->max) {
      return this->max == this->display_max && value - this->step < this->max && value - this->step >= this->min;
    }
    return true;
  };
  if (!accepts(low) || !accepts(high)) return false;
  low = std::clamp(low, this->min, this->max);
  high = std::clamp(high, this->min, this->max);
  return true;
}

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

void Entity::set_ha_service(
    const std::string &service,
    const std::map<std::string, std::string> &data,
    const std::map<std::string, std::string> &data_template) {
  this->ha_service_ = service;
  this->ha_service_data_ = data;
  this->ha_service_data_template_ = data_template;
}

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

static bool valid_effect_utf8(const std::string &effect) {
  for (size_t pos = 0; pos < effect.size();) {
    const auto len = utf8_scalar_size(effect, pos);
    if (len == 0) return false;
    pos += len;
  }
  return true;
}

// Python str repr uses fixed-width hex escapes for non-printable scalars.
static bool append_effect_escape(const std::string &value, size_t &pos, size_t digits, std::string &effect) {
  if (value.size() - pos < digits) return false;
  uint32_t scalar = 0;
  for (size_t i = 0; i < digits; ++i) {
    const char c = value[pos++];
    unsigned digit;
    if (c >= '0' && c <= '9') digit = c - '0';
    else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
    else return false;
    scalar = (scalar << 4) | digit;
  }
  if (scalar > 0x10ffff || (scalar >= 0xd800 && scalar <= 0xdfff)) return false;
  if (scalar < 0x80) {
    effect += static_cast<char>(scalar);
    return true;
  }
  const unsigned tail = scalar < 0x800 ? 1 : scalar < 0x10000 ? 2 : 3;
  const unsigned prefix = tail == 1 ? 0xc0 : tail == 2 ? 0xe0 : 0xf0;
  effect += static_cast<char>(prefix | (scalar >> (6 * tail)));
  for (unsigned i = tail; i > 0; --i)
    effect += static_cast<char>(0x80 | ((scalar >> (6 * (i - 1))) & 0x3f));
  return true;
}

// Quoted effect names can contain commas; keep their boundaries until rendering.
static std::vector<std::string> parse_effects(const std::string &value) {
  constexpr size_t max_effects = 15;
  std::vector<std::string> effects;
  if (value.empty() || value == "None" || value == "none") return effects;
  if (value.front() != '[') {
    split_str(',', value, effects);
    if (!std::all_of(effects.begin(), effects.end(), valid_effect_utf8)) return {};
    if (effects.size() > max_effects) effects.resize(max_effects);
    return effects;
  }
  size_t pos = value.find_first_not_of(" \t\r\n", 1);
  while (pos != std::string::npos) {
    const char quote = value[pos++];
    if (quote == ']') return value.find_first_not_of(" \t\r\n", pos) == std::string::npos ? effects : std::vector<std::string>{};
    if (quote != '\'' && quote != '"') return {};
    std::string effect;
    while (pos < value.size() && value[pos] != quote) {
      char c = value[pos++];
      if (c == '\n' || c == '\r') return {};
      if (c == '\\' && pos < value.size()) {
        c = value[pos++];
        if (c == 'x' || c == 'u' || c == 'U') {
          if (!append_effect_escape(value, pos, c == 'x' ? 2 : c == 'u' ? 4 : 8, effect)) return {};
          continue;
        }
        if (c == 'n') c = '\n';
        else if (c == 'r') c = '\r';
        else if (c == 't') c = '\t';
        else if (c != '\\' && c != '\'' && c != '"') return {};
      }
      effect += c;
    }
    if (pos == value.size() || !valid_effect_utf8(effect)) return {};
    if (!effect.empty() && effects.size() < max_effects) effects.push_back(std::move(effect));
    pos = value.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos || (value[pos] != ',' && value[pos] != ']')) return {};
    if (value[pos] == ',') pos = value.find_first_not_of(" \t\r\n", pos + 1);
  }
  return {};
}

void Entity::set_attribute(ha_attr_type attr, const std::string &value) {
  if (attr == ha_attr_type::effect_list) effects_ = parse_effects(value);
  auto &slot = attributes_[static_cast<size_t>(attr)];
  if (value.empty() || value == "None" || value == "none") {
    slot.clear();
    this->notify_attribute_change(attr, slot);
    return;
  }
  if (slot == value.c_str()) return;

  if (attr == ha_attr_type::brightness) {
    int brightness_val = 0;
    if (!str_to_int(value, brightness_val)) return;
    const int scaled = static_cast<int>(round(scale_value(brightness_val, {0, 255}, {0, 100})));
    slot.assign(std::to_string(scaled).c_str());
  } else if (attr == ha_attr_type::color_temp) {
    const auto range = mired_range(this->get_attribute(ha_attr_type::min_mireds),
                                   this->get_attribute(ha_attr_type::max_mireds));
    int temp_val = 0;
    if (!str_to_int(value, temp_val)) return;
    const double temp = std::clamp(static_cast<double>(temp_val), range[0], range[1]);
    const int scaled = static_cast<int>(round(scale_value(temp, range, {0, 100})));
    slot.assign(std::to_string(scaled).c_str());
  } else if (attr == ha_attr_type::supported_color_modes ||
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