#pragma once

#include <stdint.h>
#include <string>
#include <map>
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
  virtual void on_entity_attribute_change(ha_attr_type attr, const std::string &value) {}
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
  const std::string &get_attribute(ha_attr_type attr, const std::string &default_value = "") const;
  void set_attribute(ha_attr_type attr, const std::string &value);

protected:
  std::string entity_id_;
  const char *type_;
  bool type_overridden_ = false;
  std::string state_;
  std::map<ha_attr_type, std::string> attributes_;
  std::vector<IEntitySubscriber*> targets_;
  bool enable_notifications_ = false;

  void notify_type_change(const char *type);
  void notify_state_change(const std::string &state);
  void notify_attribute_change(ha_attr_type attr, const std::string &value);
};

}
}