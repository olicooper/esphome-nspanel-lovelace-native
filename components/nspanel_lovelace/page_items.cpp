#include "page_items.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include <array>
#include <functional>
#include <string>
#include <vector>
#include "esphome/core/helpers.h"

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== NavigationItem ===============
 */

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid) : 
    PageItem(uuid), PageItem_Icon(65535u),
    navigation_uuid_(navigation_uuid) {
}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const std::string &icon_default_value) : 
    PageItem(uuid), PageItem_Icon(icon_default_value, 65535u),
    navigation_uuid_(navigation_uuid) {
}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const uint16_t icon_default_color) : 
    PageItem(uuid), PageItem_Icon(icon_default_color),
    navigation_uuid_(navigation_uuid) {
}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_Icon(icon_default_value, icon_default_color),
    navigation_uuid_(navigation_uuid) {
}

void NavigationItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &NavigationItem::render_(std::string &buffer) {
  // note: should be able to skip type field but it doesn't render if I do
  // type~
  buffer.append(entity_type::button).append(1, SEPARATOR);
  // internalName(navigate.uuid.[page uuid])~
  // NOTE: navigation_uuid_ contains the uuid of the item to navigate to
  buffer.append(entity_type::navigate_uuid)
    .append(1,'.').append(navigation_uuid_).append(1, SEPARATOR);
  // icon~iconColor
  PageItem_Icon::render_(buffer);
  // skip: ~displayName~value
  return buffer.append(2, SEPARATOR);
}

/*
 * =============== StatusIconItem ===============
 */

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity) :
    StatefulPageItem(uuid, std::move(entity)), alt_font_(false) {
}

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &icon_default_value) :
    StatefulPageItem(uuid, std::move(entity), icon_default_value),
    alt_font_(false) {
}

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const uint16_t icon_default_color) :
    StatefulPageItem(uuid, std::move(entity), icon_default_color),
    alt_font_(false) {
}

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    StatefulPageItem(uuid, std::move(entity),
      icon_default_value, icon_default_color),
    alt_font_(false) {
}

void StatusIconItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &StatusIconItem::render_(std::string &buffer) {
  // icon~iconColor
  return PageItem_Icon::render_(buffer);
}

/*
 * =============== WeatherItem ===============
 */

WeatherItem::WeatherItem(const std::string &uuid) :
    PageItem(uuid), PageItem_Icon(63878u), // change the default icon color: #ff3131 (red)
    PageItem_Value("0.0"), float_value_(0.0f) {
}

WeatherItem::WeatherItem(
    const std::string &uuid, const std::string &display_name, 
    const std::string &value, const char *weather_condition) :
    PageItem(uuid), PageItem_Icon(63878u), 
    PageItem_DisplayName(display_name), 
    PageItem_Value(value), float_value_(0.0f) {
  this->set_icon_by_weather_condition(weather_condition);
}

void WeatherItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

// valid conditions are found in weather_type
void WeatherItem::set_icon_by_weather_condition(const std::string &condition) {
  Icon icon{};
  if (!try_get_value(WEATHER_ICON_MAP, icon, condition)) return;
  this->icon_color_ = icon.color;
  this->icon_value_ = icon.value;
}

bool WeatherItem::set_value(const std::string &value) {
  if (sscanf(value.c_str(), "%f", &this->float_value_) != 1)
    return false;
  this->value_ = value;
  return true;
}

std::string &WeatherItem::render_(std::string &buffer) {
  // skip: type,internalName
  buffer.append(2, SEPARATOR);
  PageItem_Icon::render_(buffer).append(1, SEPARATOR);
  PageItem_DisplayName::render_(buffer).append(1, SEPARATOR);
  // allow the value to be fomatted based on locale instead of using the raw string value
  return buffer.append(esphome::str_snprintf("%.1f", 6, this->float_value_))
      .append(WeatherItem::temperature_unit);
}

std::string WeatherItem::temperature_unit = "°C";

/*
 * =============== AlarmButtonItem ===============
 */

AlarmButtonItem::AlarmButtonItem(const std::string &uuid,
    const char *action_type, const std::string &display_name) :
    PageItem(uuid), PageItem_DisplayName(display_name),
    action_type_(action_type) {
}

void AlarmButtonItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &AlarmButtonItem::render_(std::string &buffer) {
  PageItem_DisplayName::render_(buffer).append(1, SEPARATOR);
  return buffer.append(this->action_type_);
}

/*
 * =============== AlarmIconItem ===============
 */

AlarmIconItem::AlarmIconItem(const std::string &uuid,
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_Icon(icon_default_value, icon_default_color) {}

void AlarmIconItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &AlarmIconItem::render_(std::string &buffer) {
  return PageItem_Icon::render_(buffer);
}

/*
 * =============== DeleteItem ===============
 */

DeleteItem::DeleteItem(page_type page_type) :
    PageItem(entity_type::delete_) {
  // Currently all page_types that accept delete entities
  // have the same separator quantity
  this->uuid_.append(5, SEPARATOR);
}

DeleteItem::DeleteItem(uint8_t separator_quantity) :
    PageItem(entity_type::delete_) {
  this->uuid_.append(separator_quantity, SEPARATOR);
}

void DeleteItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &DeleteItem::render_(std::string &buffer) {
  return buffer.append(this->uuid_);
}

} // namespace nspanel_lovelace
} // namespace esphome