#include "page_item_base.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include <algorithm>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== PageItem ===============
 */

PageItem::PageItem(const std::string &uuid) :
    uuid_(uuid) {}

// Copy constructor overridden so the uuid and render_buffer is cleared
PageItem::PageItem(const PageItem &other) :
    uuid_(""), render_buffer_(""), render_invalid_(true) {}

void PageItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

const std::string &PageItem::render() {
  // only re-render if values have changed
  if (this->render_invalid_) {
    this->render_buffer_.clear();
    this->render_(this->render_buffer_);
    this->render_invalid_ = false;
  }
  return this->render_buffer_;
}

std::string &PageItem::render_(std::string &buffer) {
  return buffer.append("uuid.").append(this->uuid_);
}

bool PageItem::has_page(Page *page) const {
  return this->find_page(page) != nullptr;
}

const Page *PageItem::find_page(Page *page) const {
  for (auto& p : this->pages_) {
    if (p != page)
      continue;
    return p;
  }
  return nullptr;
}

void PageItem::add_page(Page *page) {
  this->pages_.push_back(page);
}

void PageItem::remove_page(Page *page) {
  this->pages_.erase(std::remove(
    this->pages_.begin(), this->pages_.end(), page), 
    this->pages_.end());
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
  set_render_invalid_();
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
  set_render_invalid_();
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
    IHaveRenderInvalid *const child) :
    ISetRenderInvalid(child), state_("unknown") {}

PageItem_State::PageItem_State(
    IHaveRenderInvalid *const child, const std::string &state) :
    ISetRenderInvalid(child), state_(state) {}

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

/*
 * =============== StatefulPageItem ===============
 */

StatefulPageItem::StatefulPageItem(
    const std::string &uuid) :
    PageItem(uuid), PageItem_Type(this), PageItem_EntityId(this), 
    PageItem_Icon(this), PageItem_State(this) {}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, const std::string &icon_default_value) :
    PageItem(uuid), PageItem_Type(this), PageItem_EntityId(this), 
    PageItem_Icon(this, icon_default_value), PageItem_State(this) {}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_Type(this), PageItem_EntityId(this), 
    PageItem_Icon(this, icon_default_color), PageItem_State(this) {}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, 
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_Type(this), PageItem_EntityId(this), 
    PageItem_Icon(this, icon_default_value, icon_default_color),
    PageItem_State(this) {}

void StatefulPageItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

void StatefulPageItem::set_entity_id(const std::string &entity_id) {
  PageItem_EntityId::set_entity_id(entity_id);

  if (!this->entity_id_.empty() && 
      this->set_type(get_entity_type(this->entity_id_))) {

    // extract the text from iText entities
    if (this->is_type(entity_type::itext)) {
      auto pos = this->entity_id_.rfind('.', strlen(entity_type::itext) + 1);
      if (pos != std::string::npos && pos < this->entity_id_.length()) {
        this->set_state(this->entity_id_.substr(pos + 1));
      }
    }
  }
}

bool StatefulPageItem::set_type(const char *type) {
  if (type == nullptr || type[0] == '\0') {
    return false;
  }
  this->type_ = type;

  if (type == entity_type::delete_) {
    this->render_type_ = nullptr;
    return true;
  }

  auto it = ENTITY_RENDER_TYPE_MAP.find(this->type_);
  if (it != ENTITY_RENDER_TYPE_MAP.end()) {
    this->render_type_ = it->second;
  } else {
    this->render_type_ = entity_render_type::text;
  }

  if (this->type_ != entity_type::sensor) {
    auto icon_value = get_icon_by_name(ENTITY_ICON_MAP, this->type_);
    if (icon_value != nullptr) {
      this->icon_value_ = this->icon_default_value_ = icon_value;
    }
  }
  this->icon_value_overridden_ = false;

  if (this->type_ == entity_type::light ||
      this->type_ == entity_type::switch_ ||
      this->type_ == entity_type::input_boolean ||
      this->type_ == entity_type::automation ||
      this->type_ == entity_type::fan) {
    this->on_state_callback_ = StatefulPageItem::state_on_off_fn;
  } else if (this->type_ == entity_type::binary_sensor) {
    this->on_state_callback_ = StatefulPageItem::state_binary_sensor_fn;
  } else if (this->type_ == entity_type::cover) {
    this->on_state_callback_ = StatefulPageItem::state_cover_fn;
  } /* else if (
      this->type_ == entity_type::button ||
      this->type_ == entity_type::input_button) {
    this->on_state_callback_ = StatefulPageItem::state_button_fn;
  } else if (this->type_ == entity_type::scene) {
    this->on_state_callback_ = StatefulPageItem::state_scene_fn;
  } else if (this->type_ == entity_type::script) {
    this->on_state_callback_ = StatefulPageItem::state_script_fn;
  }*/

  this->set_render_invalid();

  // also need to make sure the state is updated based on the new 'type'
  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
  return true;
}

void StatefulPageItem::set_state(const std::string &state) {
  this->state_ = state;

  this->set_render_invalid();

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

void StatefulPageItem::set_attribute(const char *attr, const std::string &value) {
  if (value.empty() || value == "None" || value == "none") {
    attributes_.erase(attr);
    return;
  }
  if (attr == ha_attr_type::brightness) {
    attributes_[attr] = std::to_string(static_cast<int>(round(
        scale_value(std::stoi(value), {0, 255}, {0, 100}))));
  } else if (attr == ha_attr_type::color_temp) {
    auto minstr = this->get_attribute(ha_attr_type::min_mireds);
    auto maxstr = this->get_attribute(ha_attr_type::max_mireds);
    uint16_t min_mireds = minstr.empty() ? 153 : std::stoi(minstr);
    uint16_t max_mireds = maxstr.empty() ? 500 : std::stoi(maxstr);
    attributes_[attr] = std::to_string(static_cast<int>(round(scale_value(
        std::stoi(value),
        {static_cast<double>(min_mireds), static_cast<double>(max_mireds)},
        {0, 100}))));
  } else {
    attributes_[attr] = value;
  }
}

void StatefulPageItem::set_device_class(const std::string &device_class) {
  this->device_class_ = device_class;

  this->set_render_invalid();

  if (!this->icon_value_overridden_) {
    if (this->type_ == entity_type::sensor) {
      this->icon_default_value_ = u8"\uE5D5"; // default: alert-circle-outline
      auto icon = get_icon_by_name(SENSOR_ICON_MAP, this->device_class_);
      this->icon_value_ = (icon == nullptr ? this->icon_default_value_ : icon);
    }
  }

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

std::string &StatefulPageItem::render_(std::string &buffer) {
  // type~
  PageItem_Type::render_(buffer).append(1, SEPARATOR);
  if (this->entity_id_ == entity_type::delete_)
    // internalName(delete)~
    buffer.append(this->entity_id_).append(1, SEPARATOR);
  else
    // internalName(uuid)~
    PageItem::render_(buffer).append(1, SEPARATOR);
  // iconValue~iconColor~
  return PageItem_Icon::render_(buffer).append(1, SEPARATOR);
}
  
uint16_t StatefulPageItem::get_render_buffer_reserve_() const {
  // try to guess the required size of the buffer to reduce heap fragmentation
  return (this->type_ == nullptr ? 0 : strlen(this->type_)) + 
      this->uuid_.length() + 6 +
      this->get_icon_color_str().length() + 
      // icon is 4 char long + separator chars
      9;
}

void StatefulPageItem::state_on_off_fn(StatefulPageItem *me) {
  if (me->icon_color_overridden_) {
    return;
  }

  if (me->state_ == generic_type::on) {
    me->icon_color_ = 64909u; // yellow
  } else if (me->state_ == generic_type::off) {
    me->icon_color_ = 17299u; // blue
  } else {
    me->icon_color_ = 38066u; // grey
  }
}

void StatefulPageItem::state_binary_sensor_fn(StatefulPageItem *me) {
  const char *icon = nullptr;
  if (me->state_ == generic_type::on) {
    if (!me->icon_color_overridden_)
      me->icon_color_ = 64909u; // yellow
    if (!me->icon_value_overridden_) {
      icon = get_icon_by_name(SENSOR_ON_ICON_MAP, me->device_class_);
      me->icon_value_ = icon == nullptr ? u8"\uE132" : icon; // default: checkbox-marked-circle
    }
  } else {
    if (!me->icon_color_overridden_) {
      if (me->state_ == generic_type::off)
        me->icon_color_ = 17299u; // blue
      else
        me->icon_color_ = 38066u; // grey
    }
    if (!me->icon_value_overridden_) {
      icon = get_icon_by_name(SENSOR_OFF_ICON_MAP, me->device_class_);
      me->icon_value_ = icon == nullptr ? u8"\uE43C" : icon; // default: radiobox-blank
    }
  }
}

void StatefulPageItem::state_cover_fn(StatefulPageItem *me) {
  if (!me->icon_color_overridden_) {
    if (me->state_ == "closed")
      me->icon_color_ = 17299u; // blue
    else if (me->state_ == "open")
      me->icon_color_ = 64909u; // yellow
    else 
      me->icon_color_ = 38066u; // grey
  }
  
  if (!me->icon_value_overridden_) {
    auto icons = get_icon_by_name(COVER_MAP, me->device_class_);
    if (icons != nullptr) {
      if (me->state_ == "closed")
        me->icon_value_ = icons->at(1);
      else
        me->icon_value_ = icons->at(0);
    }
  }
}

// clang-format off
// void StatefulPageItem::state_button_fn(StatefulPageItem *me) {}
// void StatefulPageItem::state_scene_fn(StatefulPageItem *me) {}
// void StatefulPageItem::state_script_fn(StatefulPageItem *me) {}
// clang-format on

} // namespace nspanel_lovelace
} // namespace esphome