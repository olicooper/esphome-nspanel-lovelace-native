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

/*
 * =============== PageItem_Icon ===============
 */

PageItem_Icon::PageItem_Icon(IHaveRenderInvalid *const parent) :
    ISetRenderInvalid(parent), 
    icon_default_value_(icon_t::help_circle_outline), icon_default_color_(17299u),
    icon_value_(icon_default_value_), icon_color_(icon_default_color_),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    IHaveRenderInvalid *const parent, const icon_char_t *icon_default_value) :
    ISetRenderInvalid(parent),
    icon_default_value_(icon_default_value), icon_default_color_(17299u),
    icon_value_(icon_default_value), icon_color_(icon_default_color_),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    IHaveRenderInvalid *const parent, const uint16_t icon_default_color) :
    ISetRenderInvalid(parent),
    icon_default_value_(icon_t::help_circle_outline), icon_default_color_(icon_default_color),
    icon_value_(icon_default_value_), icon_color_(icon_default_color),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

PageItem_Icon::PageItem_Icon(
    IHaveRenderInvalid *const parent, const icon_char_t *icon_default_value, const uint16_t icon_default_color) :
    ISetRenderInvalid(parent), 
    icon_default_value_(icon_default_value), icon_default_color_(icon_default_color),
    icon_value_(icon_default_value), icon_color_(icon_default_color),
    icon_value_overridden_(false), icon_color_overridden_(false) {}

void PageItem_Icon::set_icon_value(const icon_char_t *value) {
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
  if (this->icon_value_ == nullptr) { 
    return buffer.append(1, SEPARATOR);
  }
  return buffer
    .append(CHAR8_CAST(this->icon_value_))
    .append(1, SEPARATOR)
    .append(this->get_icon_color_str());
}

/*
 * =============== PageItem_DisplayName ===============
 */

PageItem_DisplayName::PageItem_DisplayName(
    IHaveRenderInvalid *const parent, const std::string &display_name) :
    ISetRenderInvalid(parent),
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
    IHaveRenderInvalid *const parent, const std::string &value) :
    ISetRenderInvalid(parent),
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
 * =============== StatefulPageItem ===============
 */

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity) :
    PageItem(uuid), PageItem_Icon(this),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const icon_char_t *icon_default_value) :
    PageItem(uuid), PageItem_Icon(this, icon_default_value),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_Icon(this, icon_default_color),
    entity_(std::move(entity)), render_type_(nullptr) {
  this->entity_->add_subscriber(this);
  this->on_entity_type_change(this->entity_->get_type());
}

StatefulPageItem::StatefulPageItem(
    const std::string &uuid, std::shared_ptr<Entity> entity, 
    const icon_char_t *icon_default_value, 
    const uint16_t icon_default_color) :
    PageItem(uuid),
    PageItem_Icon(this, icon_default_value, icon_default_color),
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
    const icon_char_t *icon;
    if (try_get_value(ENTITY_ICON_MAP, icon, type)) {
      this->icon_value_ = this->icon_default_value_ = icon;
    }
  }
  this->icon_value_overridden_ = false;

  this->set_on_state_callback_(type);

  this->set_render_invalid();

  // also need to make sure the state is updated based on the new 'type'
  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

void StatefulPageItem::on_entity_state_change(const std::string &state) {
  this->set_render_invalid();

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
    this->icon_value_ = get_icon(MEDIA_TYPE_ICON_MAP,
      this->entity_->get_attribute(ha_attr_type::media_content_type),
      entity_state::off);
  } else {
    return;
  }

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }

  this->set_render_invalid();
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
  } else if (type == entity_type::sun) {
    this->on_state_callback_ = StatefulPageItem::state_sun_fn;
  } else if (type == entity_type::alarm_control_panel) {
    this->on_state_callback_ = StatefulPageItem::state_alarm_fn;
  } else if (type == entity_type::lock) {
    this->on_state_callback_ = StatefulPageItem::state_lock_fn;
  } else if (type == entity_type::weather) {
    this->on_state_callback_ = StatefulPageItem::state_weather_fn;
  } else if (type == entity_type::timer) {
    this->on_state_callback_ = StatefulPageItem::state_timer_fn;
  }
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
  
uint16_t StatefulPageItem::get_render_buffer_reserve_() const {
  // try to guess the required size of the buffer to reduce heap fragmentation
  return strlen(this->render_type_) + 
      this->uuid_.length() + 6 +
      this->get_icon_color_str().length() + 
      // icon is 4 char long + separator chars
      9;
}

void StatefulPageItem::state_on_off_fn(StatefulPageItem *me) {
  if (me->icon_color_overridden_) {
    return;
  }

  if (me->is_state(entity_state::on)) {
    me->icon_color_ = 64909u; // yellow
  } else if (me->is_state(entity_state::off)) {
    me->icon_color_ = 17299u; // blue
  } else {
    me->icon_color_ = 38066u; // grey
  }
}

void StatefulPageItem::state_binary_sensor_fn(StatefulPageItem *me) {
  if (me->is_state(entity_state::on)) {
    if (!me->icon_color_overridden_)
      me->icon_color_ = 64909u; // yellow
    if (!me->icon_value_overridden_) {
      me->icon_value_ = get_value_or_default(SENSOR_ON_ICON_MAP,
        me->get_attribute(ha_attr_type::device_class),
        static_cast<const icon_char_t *>(icon_t::checkbox_marked_circle));
    }
  } else {
    if (!me->icon_color_overridden_) {
      if (me->is_state(entity_state::off))
        me->icon_color_ = 17299u; // blue
      else
        me->icon_color_ = 38066u; // grey
    }
    if (!me->icon_value_overridden_) {
      me->icon_value_ = get_value_or_default(SENSOR_OFF_ICON_MAP,
        me->get_attribute(ha_attr_type::device_class),
        static_cast<const icon_char_t *>(icon_t::radiobox_blank));
    }
  }
}

void StatefulPageItem::state_cover_fn(StatefulPageItem *me) {
  if (!me->icon_color_overridden_) {
    if (me->is_state(entity_state::closed))
      me->icon_color_ = 17299u; // blue
    else if (me->is_state(entity_state::open))
      me->icon_color_ = 64909u; // yellow
    else 
      me->icon_color_ = 38066u; // grey
  }
  
  if (!me->icon_value_overridden_) {
    std::array<const icon_char_t *, 4> icons{};
    if (try_get_value(COVER_MAP, icons,
        me->get_attribute(ha_attr_type::device_class))) {
      if (me->is_state(entity_state::closed))
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
    if (state == entity_state::auto_ ||
        state == entity_state::heat_cool) {
      me->icon_color_ = 1024U;
    } else if (state == entity_state::off ||
        state == entity_state::fan_only) {
      me->icon_color_ = 35921U;
    } else if (state == entity_state::cool) {
      me->icon_color_ = 11487U;
    } else if (state == entity_state::dry) {
      me->icon_color_ = 60897U;
    }
  }
}

void StatefulPageItem::state_media_fn(StatefulPageItem *me) {
  if (me->icon_color_overridden_) {
    return;
  }

  if (me->is_state(entity_state::off)) {
    me->icon_color_ = 17299u; // blue
  } else if (!me->is_state(entity_state::unavailable)) {
    me->icon_color_ = 64909u; // yellow
  } else {
    me->icon_color_ = 38066u; // grey
  }
}

void StatefulPageItem::state_alarm_fn(StatefulPageItem *me) {
  if (me->icon_value_overridden_ && 
      me->icon_color_overridden_) return;
  
  Icon icon(icon_t::help_circle_outline, 38066u);
  try_get_value(ALARM_ICON_MAP, icon, me->get_state());

  if (!me->icon_value_overridden_)
    me->icon_value_ = icon.value;
  if (!me->icon_color_overridden_)
    me->icon_color_ = icon.color;
}
// todo: also change colour
void StatefulPageItem::state_sun_fn(StatefulPageItem *me) {
  if (me->icon_value_overridden_) return;
  if (me->is_state(entity_state::above_horizon))
    me->icon_value_ = icon_t::weather_sunset_up;
  else
    me->icon_value_ = icon_t::weather_sunset_down;
}
// todo: also change colour
void StatefulPageItem::state_lock_fn(StatefulPageItem *me) {
  if (me->icon_value_overridden_) return;
  if (me->is_state(entity_state::unlocked))
    me->icon_value_ = icon_t::lock_open;
  else
    me->icon_value_ = icon_t::lock;
}

void StatefulPageItem::state_weather_fn(StatefulPageItem *me) {
  if (me->icon_value_overridden_ && 
      me->icon_color_overridden_) return;

  Icon icon{};
  try_get_value(WEATHER_ICON_MAP, icon, me->get_state());

  if (!me->icon_value_overridden_)
    me->icon_value_ = icon.value;
  if (!me->icon_color_overridden_)
    me->icon_color_ = icon.color;
}

void StatefulPageItem::state_timer_fn(StatefulPageItem *me) {
  if (me->icon_value_overridden_ && 
      me->icon_color_overridden_) return;

  if (!me->icon_color_overridden_) {
    if (me->is_state(entity_state::active))
      me->icon_color_ = 64909u; // yellow
    else
      me->icon_color_ = 17299u; // blue
  }

}

} // namespace nspanel_lovelace
} // namespace esphome