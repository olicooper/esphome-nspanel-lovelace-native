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
PageItem::PageItem(const PageItem &other) : uuid_("") {}

void PageItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &PageItem::render(std::string &buffer) {
  return this->render_(buffer);
}

std::string &PageItem::render_(std::string &buffer) {
  return buffer.append("uuid.").append(this->uuid_);
}

/*
 * =============== PageItem_Icon ===============
 */

PageItem_Icon::PageItem_Icon() :
    icon_default_value_(icon_t::help_circle_outline), icon_default_color_(17299u),
    icon_value_(icon_default_value_), icon_color_(icon_default_color_),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(const std::string &icon_default_value) :
    icon_default_value_(icon_default_value), icon_default_color_(17299u),
    icon_value_(icon_default_value), icon_color_(icon_default_color_),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(const uint16_t icon_default_color) :
    icon_default_value_(icon_t::help_circle_outline), icon_default_color_(icon_default_color),
    icon_value_(icon_default_value_), icon_color_(icon_default_color),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    icon_default_value_(icon_default_value), icon_default_color_(icon_default_color),
    icon_value_(icon_default_value), icon_color_(icon_default_color),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

void PageItem_Icon::set_icon_value(const std::string &value) {
  this->icon_value_ = value;
  this->icon_value_overridden_ = true;
}

void PageItem_Icon::reset_icon_value() {
  this->icon_value_ = this->icon_default_value_;
  this->icon_value_overridden_ = false;
}

void PageItem_Icon::set_icon_color(const uint16_t color) {
  this->icon_color_ = color;
  this->icon_color_overridden_ = true;
}

void PageItem_Icon::set_icon_color(const std::array<uint8_t, 3> rgb) {
  this->icon_color_ = rgb_dec565(rgb[0], rgb[1], rgb[2]);
  this->icon_color_overridden_ = true;
}

void PageItem_Icon::reset_icon_color() {
  this->icon_color_ = this->icon_default_color_;
  this->icon_color_overridden_ = false;
}

std::string &PageItem_Icon::render_(std::string &buffer) {
  if (this->icon_value_.empty()) { 
    return buffer.append(1, SEPARATOR);
  }
  return buffer
    .append(this->icon_value_)
    .append(1, SEPARATOR)
    .append(this->get_icon_color_str());
}

/*
 * =============== PageItem_DisplayName ===============
 */

PageItem_DisplayName::PageItem_DisplayName(const std::string &display_name) :
    display_name_(display_name) {}

void PageItem_DisplayName::set_display_name(const std::string &display_name) {
  this->display_name_ = display_name;
}

std::string &PageItem_DisplayName::render_(std::string &buffer) {
  return buffer.append(this->display_name_);
}

/*
 * =============== PageItem_Value ===============
 */

PageItem_Value::PageItem_Value(const std::string &value) :
    value_(value) {}

bool PageItem_Value::set_value(const std::string &value) {
  this->value_ = value;
  return true;
}

void PageItem_Value::set_value_postfix(const std::string &value_postfix) {
  this->value_postfix_ = value_postfix;
}

std::string &PageItem_Value::render_(std::string &buffer) {
  return buffer
      .append(this->value_)
      .append(this->value_postfix_);
}

/*
 * =============== StatefulPageItem ===============
 */

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity) :
    PageItem(uuid),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &icon_default_value) :
    PageItem(uuid), PageItem_Icon(icon_default_value),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_Icon(icon_default_color),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity, 
    const std::string &icon_default_value, 
    const uint16_t icon_default_color) :
    PageItem(uuid),
    PageItem_Icon(icon_default_value, icon_default_color),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::~StatefulPageItem() {
  this->entity_->remove_subscriber(this);
}

void StatefulPageItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

void StatefulPageItem::on_entity_type_change(const char *type) {
  if (type == entity_type::delete_) {
    this->render_type_ = "";
  }

  this->render_type_ = get_value_or_default(ENTITY_RENDER_TYPE_MAP,
    type, entity_render_type::text);

  if (type != entity_type::sensor) {
    const char *icon;
    if (try_get_value(ENTITY_ICON_MAP, icon, type)) {
      this->icon_value_ = this->icon_default_value_ = icon;
    }
  }
  this->icon_value_overridden_ = false;

  this->set_on_state_callback_(type);

  // also need to make sure the state is updated based on the new 'type'
  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

void StatefulPageItem::on_entity_state_change(const std::string &state) {
  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

void StatefulPageItem::on_entity_attribute_change(ha_attr_type attr, const std::string &value) {
  // this class only needs to react to the following attributes
  if (attr == ha_attr_type::device_class) {
    if (!this->icon_value_overridden_) {
      if (this->entity_->is_type(entity_type::sensor)) {
        this->icon_default_value_ = this->icon_value_ =
          get_icon(SENSOR_ICON_MAP, value);
      }
    }
  } else if (attr == ha_attr_type::media_content_type) {
    if (this->icon_value_overridden_) return;
    this->icon_value_ = get_icon(MEDIA_TYPE_MAP,
      this->entity_->get_attribute(ha_attr_type::media_content_type),
      generic_type::off);
  } else {
    return;
  }

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

void StatefulPageItem::set_on_state_callback_(const char *type) {
  if (type == entity_type::light ||
      type == entity_type::switch_ ||
      type == entity_type::input_boolean ||
      type == entity_type::automation ||
      type == entity_type::fan) {
    this->on_state_callback_ = StatefulPageItem::state_on_off_fn;
  } else if (type == entity_type::binary_sensor) {
    this->on_state_callback_ = StatefulPageItem::state_binary_sensor_fn;
  } else if (type == entity_type::cover) {
    this->on_state_callback_ = StatefulPageItem::state_cover_fn;
  } else if (type == entity_type::climate) {
    this->on_state_callback_ = StatefulPageItem::state_climate_fn;
  } else if (type == entity_type::media_player) {
    this->on_state_callback_ = StatefulPageItem::state_media_fn;
  } /* else if (
      type == entity_type::button ||
      type == entity_type::input_button) {
    this->on_state_callback_ = StatefulPageItem::state_button_fn;
  } else if (type == entity_type::scene) {
    this->on_state_callback_ = StatefulPageItem::state_scene_fn;
  } else if (type == entity_type::script) {
    this->on_state_callback_ = StatefulPageItem::state_script_fn;
  }*/
}

std::string &StatefulPageItem::render_(std::string &buffer) {
  // type~
  buffer.append(this->render_type_).append(1, SEPARATOR);
  if (this->entity_->is_type(entity_type::delete_))
    // internalName(delete)~
    buffer.append(entity_type::delete_).append(1, SEPARATOR);
  else
    // internalName(uuid)~
    PageItem::render_(buffer).append(1, SEPARATOR);
  // iconValue~iconColor~
  return PageItem_Icon::render_(buffer).append(1, SEPARATOR);
}

void StatefulPageItem::state_on_off_fn(StatefulPageItem *me) {
  if (me->icon_color_overridden_) {
    return;
  }

  if (me->entity_->is_state(generic_type::on)) {
    me->icon_color_ = 64909u; // yellow
  } else if (me->entity_->is_state(generic_type::off)) {
    me->icon_color_ = 17299u; // blue
  } else {
    me->icon_color_ = 38066u; // grey
  }
}

void StatefulPageItem::state_binary_sensor_fn(StatefulPageItem *me) {
  if (me->entity_->is_state(generic_type::on)) {
    if (!me->icon_color_overridden_)
      me->icon_color_ = 64909u; // yellow
    if (!me->icon_value_overridden_) {
      me->icon_value_ = get_value_or_default(SENSOR_ON_ICON_MAP,
        me->entity_->get_attribute(ha_attr_type::device_class),
        static_cast<const char *>(icon_t::checkbox_marked_circle));
    }
  } else {
    if (!me->icon_color_overridden_) {
      if (me->entity_->is_state(generic_type::off))
        me->icon_color_ = 17299u; // blue
      else
        me->icon_color_ = 38066u; // grey
    }
    if (!me->icon_value_overridden_) {
      me->icon_value_ = get_value_or_default(SENSOR_OFF_ICON_MAP,
        me->entity_->get_attribute(ha_attr_type::device_class),
        static_cast<const char *>(icon_t::radiobox_blank));
    }
  }
}

void StatefulPageItem::state_cover_fn(StatefulPageItem *me) {
  if (!me->icon_color_overridden_) {
    if (me->entity_->is_state("closed"))
      me->icon_color_ = 17299u; // blue
    else if (me->entity_->is_state("open"))
      me->icon_color_ = 64909u; // yellow
    else 
      me->icon_color_ = 38066u; // grey
  }
  
  if (!me->icon_value_overridden_) {
    std::array<const char *, 4> icons{};
    if (try_get_value(COVER_MAP, icons,
        me->entity_->get_attribute(ha_attr_type::device_class))) {
      if (me->entity_->is_state("closed"))
        me->icon_value_ = icons.at(1);
      else
        me->icon_value_ = icons.at(0);
    }
  }
}

void StatefulPageItem::state_climate_fn(StatefulPageItem *me) {
  auto &state = me->get_state();

  if (!me->icon_value_overridden_) {
    me->icon_value_ = get_value_or_default(CLIMATE_ICON_MAP,
      state, icon_t::checkbox_marked_circle);
  }

  if (!me->icon_color_overridden_) {
    me->icon_color_ = 64512U;
    if (state == ha_attr_hvac_mode::auto_ ||
        state == ha_attr_hvac_mode::heat_cool) {
      me->icon_color_ = 1024U;
    } else if (state == ha_attr_hvac_mode::off ||
        state == ha_attr_hvac_mode::fan_only) {
      me->icon_color_ = 35921U;
    } else if (state == ha_attr_hvac_mode::cool) {
      me->icon_color_ = 11487U;
    } else if (state == ha_attr_hvac_mode::dry) {
      me->icon_color_ = 60897U;
    }
  }
}

void StatefulPageItem::state_media_fn(StatefulPageItem *me) {
  if (me->icon_color_overridden_) {
    return;
  }

  if (me->entity_->is_state(generic_type::off)) {
    me->icon_color_ = 17299u; // blue
  } else if (!me->entity_->is_state(generic_type::unavailable)) {
    me->icon_color_ = 64909u; // yellow
  } else {
    me->icon_color_ = 38066u; // grey
  }
}

// clang-format off
// void StatefulPageItem::state_button_fn(StatefulPageItem *me) {}
// void StatefulPageItem::state_scene_fn(StatefulPageItem *me) {}
// void StatefulPageItem::state_script_fn(StatefulPageItem *me) {}
// clang-format on

} // namespace nspanel_lovelace
} // namespace esphome