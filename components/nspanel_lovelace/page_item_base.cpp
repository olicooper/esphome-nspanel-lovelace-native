#include "page_item_base.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include <array>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== PageItem ===============
 */

const uint32_t PageItem::this_class_type_ = 1<<0;

PageItem::PageItem(const std::string &uuid) :
    uuid_(uuid) {}

// Copy constructor overridden so the uuid and render_buffer is cleared
PageItem::PageItem(const PageItem &other) :
    uuid_(""), render_buffer_(""), render_invalid_(true) {}

const std::string &PageItem::render() {
  // only re-render if values have changed
  if (this->render_invalid_) {
    this->render_(this->render_buffer_);
    this->render_invalid_ = false;
  }
  return this->render_buffer_;
}

std::string &PageItem::render_(std::string &buffer) {
  return buffer.append("uuid.").append(this->uuid_);
}

/*
 * =============== PageItem_Type ===============
 */

PageItem_Type::PageItem_Type(
    IHaveRenderInvalid *const child, const char *type) :
    ISetRenderInvalid(child),
    type_(type), render_type_(type) {}

bool PageItem_Type::set_type(const char *type) {
  if (type == nullptr || type[0] == '\0') {
    return false;
  }
  this->type_ = type;
  this->render_type_ = this->type_;
  set_render_invalid_();
  return true;
}

std::string &PageItem_Type::render_(std::string &buffer) {
  if (this->render_type_ == nullptr)
    return buffer;
  return buffer.append(this->render_type_);
}

/*
 * =============== PageItem_EntityId ===============
 */

PageItem_EntityId::PageItem_EntityId(IHaveRenderInvalid *const child) :
    ISetRenderInvalid(child), entity_id_("delete") {}

PageItem_EntityId::PageItem_EntityId(
    IHaveRenderInvalid *const child, const std::string &entity_id) :
    ISetRenderInvalid(child), entity_id_(entity_id) {}

void PageItem_EntityId::set_entity_id(const std::string &entity_id) {
  this->entity_id_ = entity_id;
  set_render_invalid_();
}

std::string &PageItem_EntityId::render_(std::string &buffer) {
  return buffer.append(this->entity_id_);
}

/*
 * =============== PageItem_Icon ===============
 */

PageItem_Icon::PageItem_Icon(IHaveRenderInvalid *const child) :
    ISetRenderInvalid(child), 
    icon_default_value_(u8"\uE624"), icon_default_color_(17299u),
    icon_value_(icon_default_value_), icon_color_(icon_default_color_),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    IHaveRenderInvalid *const child, const std::string &icon_default_value) :
    ISetRenderInvalid(child),
    icon_default_value_(icon_default_value), icon_default_color_(17299u),
    icon_value_(icon_default_value), icon_color_(icon_default_color_),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    IHaveRenderInvalid *const child, const uint16_t icon_default_color) :
    ISetRenderInvalid(child),
    icon_default_value_(u8"\uE624"), icon_default_color_(icon_default_color),
    icon_value_(icon_default_value_), icon_color_(icon_default_color),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    IHaveRenderInvalid *const child, const std::string &icon_default_value, const uint16_t icon_default_color) :
    ISetRenderInvalid(child), 
    icon_default_value_(icon_default_value), icon_default_color_(icon_default_color),
    icon_value_(icon_default_value), icon_color_(icon_default_color),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

void PageItem_Icon::set_icon_value(const std::string &value) {
  this->icon_value_ = value;
  this->icon_value_overridden_ = true;
  set_render_invalid_();
}

void PageItem_Icon::reset_icon_value() {
  this->icon_value_ = this->icon_default_value_;
  this->icon_value_overridden_ = false;
}

void PageItem_Icon::set_icon_color(const uint16_t color) {
  this->icon_color_ = color;
  this->icon_color_overridden_ = true;
  set_render_invalid_();
}

void PageItem_Icon::set_icon_color(const std::array<uint8_t, 3> rgb) {
  this->icon_color_ = rgb_dec565(rgb[0], rgb[1], rgb[2]);
  this->icon_color_overridden_ = true;
  set_render_invalid_();
}

void PageItem_Icon::reset_icon_color() {
  this->icon_color_ = this->icon_default_color_;
  this->icon_color_overridden_ = false;
}

std::string &PageItem_Icon::render_(std::string &buffer) {
  return this->icon_value_.empty() ? buffer.append(1, SEPARATOR)
                                   : buffer.append(this->icon_value_)
                                         .append(1, SEPARATOR)
                                         .append(this->get_icon_color_str());
}

/*
 * =============== PageItem_DisplayName ===============
 */

PageItem_DisplayName::PageItem_DisplayName(
    IHaveRenderInvalid *const child, const std::string &display_name) :
    ISetRenderInvalid(child),
    display_name_(display_name) {}

void PageItem_DisplayName::set_display_name(const std::string &display_name) {
  this->display_name_ = display_name;
  set_render_invalid_();
}

std::string &PageItem_DisplayName::render_(std::string &buffer) {
  return buffer.append(this->display_name_);
}

/*
 * =============== PageItem_Value ===============
 */

PageItem_Value::PageItem_Value(
    IHaveRenderInvalid *const child, const std::string &value) :
    ISetRenderInvalid(child),
    value_(value) {}

bool PageItem_Value::set_value(const std::string &value) {
  this->value_ = value;
  set_render_invalid_();
  return true;
}

void PageItem_Value::set_value_postfix(const std::string &value_postfix) {
  this->value_postfix_ = value_postfix;
  set_render_invalid_();
}

std::string &PageItem_Value::render_(std::string &buffer) {
  return buffer
      .append(this->value_)
      .append(this->value_postfix_);
}

/*
 * =============== PageItem_State ===============
 */

PageItem_State::PageItem_State(
    IHaveRenderInvalid *const child, const std::string &state) :
    ISetRenderInvalid(child),
    state_(state) {}

void PageItem_State::set_state(const std::string &state) {
  this->state_ = state;
  set_render_invalid_();
}

void PageItem_State::set_condition_state(const std::string &condition) {
  this->condition_state_ = condition;
  set_render_invalid_();
}

void PageItem_State::set_condition_not_state(const std::string &condition) {
  this->condition_not_state_ = condition;
  set_render_invalid_();
}

std::string &PageItem_State::render_(std::string &buffer) {
  // Do not render if the state does not match the condition
  if (condition_state_ != state_) {
    return buffer;
  }
  // Do not render if the state matches the condition
  if (condition_not_state_ == state_) {
    return buffer;
  }
  return buffer.append(this->state_);
}

} // namespace nspanel_lovelace
} // namespace esphome