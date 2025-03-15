#pragma once

#include "defines.h"
#include "entity.h"
#include "helpers.h"
#include "page_item_visitor.h"
#include "types.h"
#include <array>
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

class Page;

struct IRender {
protected:
  virtual std::string &render_(std::string &buffer) = 0;
};

struct IHaveRenderInvalid {
public:
  virtual bool get_render_invalid() = 0;
  // Invalidates the render buffer and forces a re-build of the string.
  // Note: Call this manually if static members are modified
  //       as the instance won't know about those changes.
  virtual void set_render_invalid() = 0;
};

struct ISetRenderInvalid {
public:
  ISetRenderInvalid(IHaveRenderInvalid *const parent) : parent_(parent) {}
  virtual ~ISetRenderInvalid() {}

  virtual void set_render_invalid_() {
    if (parent_ == nullptr)
      return;
    parent_->set_render_invalid();
  }

private:
  IHaveRenderInvalid *const parent_{nullptr};
};

/*
 * =============== PageItem ===============
 */

class PageItem : public IRender, public IHaveRenderInvalid {
public:
  PageItem(const std::string &uuid);
  PageItem(const PageItem &other);
  virtual ~PageItem() {}

  virtual void accept(PageItemVisitor& visitor);
  
  const std::string &get_uuid() const { return this->uuid_; }
  virtual void set_uuid(const std::string &uuid) { this->uuid_ = uuid; }
  
  bool get_render_invalid() { return this->render_invalid_; }
  virtual void set_render_invalid() { this->render_invalid_ = true; }
  virtual const std::string &render();

protected:
  std::string uuid_;
  std::string render_buffer_;
  bool render_invalid_ = true;

  virtual uint16_t get_render_buffer_reserve_() const { return 5; }
  
  // output: internalName (uuid)
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== PageItem_Icon ===============
 */

class PageItem_Icon : public IRender, public ISetRenderInvalid {
public:
  PageItem_Icon(IHaveRenderInvalid *const parent);
  PageItem_Icon(IHaveRenderInvalid *const parent, const icon_char_t *icon_default_value);
  PageItem_Icon(IHaveRenderInvalid *const parent, const uint16_t icon_default_color);
  PageItem_Icon(IHaveRenderInvalid *const parent, const icon_char_t *icon_default_value, const uint16_t icon_default_color);
  virtual ~PageItem_Icon() {}

  const icon_char_t *get_icon_value() const { return this->icon_value_; }
  bool is_icon_value_overridden() const { return this->icon_value_overridden_; }
  uint16_t get_icon_color() const { return this->icon_color_; }
  std::string get_icon_color_str() const {
    return std::to_string(this->icon_color_);
  }

  virtual void set_icon_value(const icon_char_t *value);
  virtual void reset_icon_value();
  virtual void set_icon_color(const uint16_t color);
  virtual void set_icon_color(const std::array<uint8_t, 3> rgb);
  virtual void reset_icon_color();

protected:
  // mdi:0625 (help-circle-outline)
  const icon_char_t *icon_default_value_;
  // default HA blue
  uint16_t icon_default_color_;
  const icon_char_t *icon_value_;
  uint16_t icon_color_;
  bool icon_value_overridden_;
  bool icon_color_overridden_;

  // output: icon~iconColor
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== PageItem_DisplayName ===============
 */

class PageItem_DisplayName : public IRender, public ISetRenderInvalid {
public:
  PageItem_DisplayName(IHaveRenderInvalid *const parent) : ISetRenderInvalid(parent) {}
  PageItem_DisplayName(IHaveRenderInvalid *const parent, const std::string &display_name);
  virtual ~PageItem_DisplayName() {}

  const std::string &get_display_name() const {
    return this->display_name_;
  }

  virtual void set_display_name(const std::string &display_name);

protected:
  std::string display_name_;

  // output: displayName
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== PageItem_Value ===============
 */

class PageItem_Value : public IRender, public ISetRenderInvalid {
public:
  PageItem_Value(IHaveRenderInvalid *const parent) : ISetRenderInvalid(parent) {}
  PageItem_Value(IHaveRenderInvalid *const parent, const std::string &value);
  virtual ~PageItem_Value() {}

  const std::string &get_value() const { return this->value_; }
  const std::string &get_value_postfix() const { return this->value_postfix_; }

  virtual bool set_value(const std::string &value);
  virtual void set_value_postfix(const std::string &value_postfix);

protected:
  std::string value_;
  std::string value_postfix_;

  // output: value
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== StatefulPageItem ===============
 */

class StatefulPageItem :
    public PageItem,
    public PageItem_Icon,
    public IEntitySubscriber {
public:
  StatefulPageItem(
      const std::string &uuid, std::shared_ptr<Entity> entity);
  StatefulPageItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const icon_char_t *icon_default_value);
  StatefulPageItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const uint16_t icon_default_color);
  StatefulPageItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const icon_char_t *icon_default_value, 
      const uint16_t icon_default_color);
  virtual ~StatefulPageItem();

  void accept(PageItemVisitor& visitor) override;

  void on_entity_type_change(const char *type) override;
  void on_entity_state_change(const std::string &state) override;
  void on_entity_attribute_change(ha_attr_type attr, const std::string &value) override;

  bool is_type(const char *type) const { return this->entity_->is_type(type); }
  const char *get_type() const { return this ->entity_->get_type(); }
  const std::string &get_entity_id() const { return this->entity_->get_entity_id(); }
  bool is_state(const std::string &state) const { return this->entity_->is_state(state); }
  const std::string &get_state() const { return this->entity_->get_state(); }
  const std::string &get_attribute(
      ha_attr_type attr, const std::string &default_value = "") const {
    return this->entity_->get_attribute(attr, default_value);
  }
  Entity* get_entity() const { return this->entity_.get(); }

protected:
  const std::shared_ptr<Entity> entity_;
  // A function which modifies the entity when the state changes
  std::function<void(StatefulPageItem *)> on_state_callback_;
  const char *render_type_;

  virtual void set_on_state_callback_(const char *type);

  static void state_on_off_fn(StatefulPageItem *me);
  static void state_binary_sensor_fn(StatefulPageItem *me);
  static void state_cover_fn(StatefulPageItem *me);
  static void state_climate_fn(StatefulPageItem *me);
  static void state_media_fn(StatefulPageItem *me);
  static void state_sun_fn(StatefulPageItem *me);
  static void state_alarm_fn(StatefulPageItem *me);
  static void state_lock_fn(StatefulPageItem *me);
  static void state_weather_fn(StatefulPageItem *me);
  static void state_timer_fn(StatefulPageItem *me);

  // output: type~internalName~icon~iconColor~
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;
};

} // namespace nspanel_lovelace
} // namespace esphome