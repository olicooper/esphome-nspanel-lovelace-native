#pragma once

#include "card_base.h"
#include <stdint.h>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCardEntityItem ===============
 */

class GridCardEntityItem : public StatefulCardItem {
public:
  GridCardEntityItem(
      const std::string &uuid, const std::string &entity_id) : 
      StatefulCardItem(uuid) {
    this->set_entity_id(entity_id);
    this->render_buffer_.reserve(this->get_render_buffer_reserve_());
  }
  GridCardEntityItem(
      const std::string &uuid, const std::string &entity_id, 
      const std::string &display_name) : 
      StatefulCardItem(uuid, display_name) {
    this->set_entity_id(entity_id);
    this->render_buffer_.reserve(this->get_render_buffer_reserve_());
  }
  
  const uint32_t class_type() const override { return this->this_class_type_; }
  static const bool is_instance_of(PageItem *item) {
    return (item->class_type() & GridCardEntityItem::this_class_type_) == GridCardEntityItem::this_class_type_;
  }

protected:
  static const uint32_t static_class_type_() { return GridCardEntityItem::this_class_type_; }

private:
  static const uint32_t this_class_type_;
};

/*
 * =============== EntitiesCardEntityItem ===============
 */

class EntitiesCardEntityItem :
    public StatefulCardItem,
    public PageItem_Value {
public:
  EntitiesCardEntityItem(const std::string &uuid, const std::string &entity_id);
  EntitiesCardEntityItem(
      const std::string &uuid, const std::string &entity_id,
      const std::string &display_name);

  const uint32_t class_type() const override { return this->this_class_type_; }
  static const bool is_instance_of(PageItem *item) {
    return (item->class_type() & EntitiesCardEntityItem::this_class_type_) == EntitiesCardEntityItem::this_class_type_;
  }

  const std::string &get_value() const { return this->value_; }
  bool set_type(const char *type) override;
  void set_state(const std::string &state) override;

protected:
  static const uint32_t static_class_type_() { return EntitiesCardEntityItem::this_class_type_; }

  static void state_generic_fn(StatefulCardItem *me);
  static void state_on_off_fn(StatefulCardItem *me);
  static void state_button_fn(StatefulCardItem *me);
  static void state_scene_fn(StatefulCardItem *me);
  static void state_script_fn(StatefulCardItem *me);

  // output: type~internalName~icon~iconColor~displayName~value
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;

private:
  static const uint32_t this_class_type_;
};

} // namespace nspanel_lovelace
} // namespace esphome