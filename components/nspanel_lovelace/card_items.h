#pragma once

#include "card_base.h"
#include "page_item_visitor.h"
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

  void accept(PageItemVisitor& visitor) override;
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

  void accept(PageItemVisitor& visitor) override;

  const std::string &get_value() const { return this->value_; }
  bool set_type(const char *type) override;
  void set_state(const std::string &state) override;

protected:
  static void state_generic_fn(StatefulCardItem *me);
  static void state_on_off_fn(StatefulCardItem *me);
  static void state_button_fn(StatefulCardItem *me);
  static void state_scene_fn(StatefulCardItem *me);
  static void state_script_fn(StatefulCardItem *me);

  // output: type~internalName~icon~iconColor~displayName~value
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;
};

} // namespace nspanel_lovelace
} // namespace esphome