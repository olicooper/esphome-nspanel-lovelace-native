#pragma once

#include "card_base.h"
#include "entity.h"
#include "page_item_visitor.h"
#include <memory>
#include <stdint.h>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCardEntityItem ===============
 */

class GridCardEntityItem : public CardItem {
public:
  GridCardEntityItem(const std::string &uuid, std::shared_ptr<Entity> entity);
  GridCardEntityItem(
      const std::string &uuid, std::shared_ptr<Entity> entity, 
      const std::string &display_name);
  // virtual ~GridCardEntityItem() {}

  void accept(PageItemVisitor& visitor) override;
};

/*
 * =============== EntitiesCardEntityItem ===============
 */

class EntitiesCardEntityItem :
    public CardItem,
    public PageItem_Value {
public:
  EntitiesCardEntityItem(const std::string &uuid, std::shared_ptr<Entity> entity);
  EntitiesCardEntityItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const std::string &display_name);
  // virtual ~EntitiesCardEntityItem() {}

  void accept(PageItemVisitor& visitor) override;
  
  void on_entity_attribute_change(ha_attr_type attr, const psram_string &value) override;

  const std::string &get_value() const { return this->value_; }

protected:
  static void state_generic_fn(StatefulPageItem *me);
  static void state_on_off_fn(StatefulPageItem *me);
  static void state_button_fn(StatefulPageItem *me);
  static void state_scene_fn(StatefulPageItem *me);
  static void state_script_fn(StatefulPageItem *me);
  static void state_timer_fn(StatefulPageItem *me);
  static void state_cover_fn(StatefulPageItem *me);
  static void state_climate_fn(StatefulPageItem *me);
  static void state_number_fn(StatefulPageItem *me);
  static void state_lock_fn(StatefulPageItem *me);
  static void state_weather_fn(StatefulPageItem *me);
  static void state_sun_fn(StatefulPageItem *me);
  static void state_vacuum_fn(StatefulPageItem *me);
  static void state_translate_fn(StatefulPageItem *me);

  void set_on_state_callback_(const char *type) override;

  // output: type~internalName~icon~iconColor~displayName~value
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;
};

/*
 * =============== PowerCardItem ===============
 */

class PowerCardItem : public CardItem {
public:
  PowerCardItem(const std::string &uuid, std::shared_ptr<Entity> entity);
  PowerCardItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const std::string &display_name);

  void accept(PageItemVisitor& visitor) override;

  void on_entity_attribute_change(ha_attr_type attr, const psram_string &value) override;

  int16_t get_speed() const { return this->speed_; }

  // rgb565 grey shown when power == 0; active color comes from icon_color (set via YAML)
  static constexpr uint16_t COLOR_INACTIVE = 33808u; // #808080

protected:
  std::string value_str_;
  std::string ha_unit_;
  int16_t speed_ = 0;
  bool is_active_ = false;

  static void state_power_fn(StatefulPageItem *me);
  void set_on_state_callback_(const char *type) override;

  // output: text~entityId~icon~iconColor~displayName~valueStr~speed
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;
};

} // namespace nspanel_lovelace
} // namespace esphome