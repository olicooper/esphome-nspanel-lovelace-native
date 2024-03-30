#pragma once

#include "config.h"
#include "types.h"
#include "helpers.h"
#include <array>
#include <map>
#include <functional>
#include <string>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

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
  ISetRenderInvalid(IHaveRenderInvalid *const child) : child_(child) {}

  virtual void set_render_invalid_() {
    if (child_ == nullptr)
      return;
    child_->set_render_invalid();
  }

private:
  IHaveRenderInvalid *const child_{nullptr};
};

/*
 * =============== PageItem ===============
 */

class PageItem : public IRender, public IHaveRenderInvalid {
public:
  PageItem(const std::string &uuid);
  PageItem(const PageItem &other);
  virtual ~PageItem() {}

  virtual uint32_t class_type() const { return this->this_class_type_; }
  static bool is_instance_of(PageItem *item) {
    return (item->class_type() & PageItem::this_class_type_) == PageItem::this_class_type_;
  }
  
  const std::string &get_uuid() const { return this->uuid_; }
  virtual void set_uuid(const std::string &uuid) { this->uuid_ = uuid; }
  
  bool get_render_invalid() { return this->render_invalid_; }
  virtual void set_render_invalid() { this->render_invalid_ = true; }
  virtual const std::string &render();

protected:
  // dynamic_cast is not available, so this is used to determine the type instead
  static uint32_t static_class_type_() { return PageItem::this_class_type_; }

  virtual uint16_t get_render_buffer_reserve_() const { return 15; }
  
  // output: internalName (uuid)
  std::string &render_(std::string &buffer) override;

  std::string uuid_;
  std::string render_buffer_;
  bool render_invalid_ = true;

private:
  static const uint32_t this_class_type_;
};

/*
 * =============== PageItem_Type ===============
 */

class PageItem_Type : public IRender, public ISetRenderInvalid {
public:
  PageItem_Type(IHaveRenderInvalid *const child) : 
      ISetRenderInvalid(child), type_(nullptr), render_type_(nullptr) {}
  PageItem_Type(IHaveRenderInvalid *const child, const char *type);
  
  bool is_type(const char *type) const {
    if (type == this->type_) return true;
    if (type == nullptr || this->type_ == nullptr) return false;
    return std::strcmp(this->type_, type) == 0;
  }
  const char *get_type() const { return value_or_empty(this->type_); }
  const char *get_render_type() const {
    return value_or_empty(this->render_type_);
  }

  virtual bool set_type(const char *type);

protected:
  const char *type_;
  // todo: should this be moved to WeatherItem instead?
  const char *render_type_;

  // output: type
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== PageItem_EntityId ===============
 */

class PageItem_EntityId : public IRender, public ISetRenderInvalid {
public:
  PageItem_EntityId(IHaveRenderInvalid *const child);
  PageItem_EntityId(IHaveRenderInvalid *const child, const std::string &entity_id);

  const std::string &get_entity_id() const { return this->entity_id_; }

  virtual void set_entity_id(const std::string &entity_id);

protected:
  std::string entity_id_;

  // output: entity_id
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== PageItem_Icon ===============
 */

class PageItem_Icon : public IRender, public ISetRenderInvalid {
public:
  PageItem_Icon(IHaveRenderInvalid *const child);
  PageItem_Icon(IHaveRenderInvalid *const child, const std::string &icon_default_value);
  PageItem_Icon(IHaveRenderInvalid *const child, const uint16_t icon_default_color);
  PageItem_Icon(IHaveRenderInvalid *const child, const std::string &icon_default_value, const uint16_t icon_default_color);

  const std::string &get_icon_value() const { return this->icon_value_; }
  bool is_icon_value_overridden() const { return this->icon_value_overridden_; }
  uint16_t get_icon_color() const { return this->icon_color_; }
  std::string get_icon_color_str() const {
    return std::to_string(this->icon_color_);
  }

  virtual void set_icon_value(const std::string &value);
  virtual void reset_icon_value();
  virtual void set_icon_color(const uint16_t color);
  virtual void set_icon_color(const std::array<uint8_t, 3> rgb);
  virtual void reset_icon_color();

protected:
  // mdi:0625 (help-circle-outline)
  std::string icon_default_value_;
  // default HA blue
  uint16_t icon_default_color_;
  std::string icon_value_;
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
  PageItem_DisplayName(IHaveRenderInvalid *const child) : ISetRenderInvalid(child) {}
  PageItem_DisplayName(IHaveRenderInvalid *const child, const std::string &display_name);

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
  PageItem_Value(IHaveRenderInvalid *const child) : ISetRenderInvalid(child) {}
  PageItem_Value(IHaveRenderInvalid *const child, const std::string &value);

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
 * =============== PageItem_State ===============
 */

class PageItem_State : public IRender, public ISetRenderInvalid {
public:
  PageItem_State(IHaveRenderInvalid *const child) : ISetRenderInvalid(child) {}
  PageItem_State(IHaveRenderInvalid *const child, const std::string &state);

  const std::string &get_state() const { return this->state_; }
  const std::string &get_condition_state() const {
    return this->condition_state_;
  }
  const std::string &get_condition_not_state() const {
    return this->condition_not_state_;
  }
  std::string get_attribute(const char *attr, std::string default_value = "") const {
    auto it = attributes_.find(attr);
    return it == attributes_.end() ? default_value : it->second;
  }

  virtual void set_state(const std::string &state);
  virtual void set_condition_state(const std::string &condition);
  virtual void set_condition_not_state(const std::string &condition);
  virtual void set_attribute(const char *attr, const std::string &value) { attributes_[attr] = value; }

protected:
  std::string state_;
  std::string condition_state_;
  std::string condition_not_state_;
  std::map<std::string, std::string> attributes_;

  // output: state
  std::string &render_(std::string &buffer) override;
};

} // namespace nspanel_lovelace
} // namespace esphome