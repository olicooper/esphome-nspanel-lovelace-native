#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "helpers.h"
#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

struct IEntitySubscriber {
public:
  virtual ~IEntitySubscriber() {}
  virtual void on_entity_type_change(const char *type) {}
  virtual void on_entity_state_change(const std::string &state) {}
  virtual void on_entity_attribute_change(ha_attr_type attr, const psram_string &value) {}
};

class Entity {
public:
  Entity(const std::string &entity_id);
  Entity(const std::string &entity_id, const char *type);

  void add_subscriber(IEntitySubscriber *const target);
  bool remove_subscriber(const IEntitySubscriber *const target);

  const std::string &get_entity_id() const;
  void set_entity_id(const std::string &entity_id);
  
  bool is_type(const char *type) const;
  const char *get_type() const;
  bool set_type(const char *type);

  bool is_state(const std::string &state) const;
  const std::string &get_state() const;
  void set_state(const std::string &state);

  bool has_attribute(ha_attr_type attr) const;
  // Returns either the entity's own attribute storage or a static empty string,
  // so the reference is always safe to hold.
  inline const psram_string &get_attribute(ha_attr_type attr) const {
    static const psram_string empty_value{};
    const auto &val = attributes_[static_cast<size_t>(attr)];
    return val.empty() ? empty_value : val;
  }

  // Returns by value: returning a reference here would dangle when the attribute
  // is unset and `default_value` is bound to a temporary (e.g. a string literal),
  // since that temporary only lives until the end of the caller's full-expression.
  inline psram_string get_attribute(ha_attr_type attr, const psram_string &default_value) const {
    const auto &val = attributes_[static_cast<size_t>(attr)];
    return val.empty() ? default_value : val;
  }
  void set_attribute(ha_attr_type attr, const std::string &value);

protected:
  std::string entity_id_;
  const char *type_ = nullptr;
  bool type_overridden_ = false;
  std::string state_;
  // One slot per attribute type. The slot table itself is allocated in PSRAM
  // (when available) so it doesn't take up internal RAM for every entity.
  psram_vector<psram_string> attributes_ =
      psram_vector<psram_string>(static_cast<size_t>(ha_attr_type::_count));
  std::vector<IEntitySubscriber*> targets_;
  bool enable_notifications_ = false;

  void notify_type_change(const char *type);
  void notify_state_change(const std::string &state);
  void notify_attribute_change(ha_attr_type attr, const psram_string &value);
};

}
}