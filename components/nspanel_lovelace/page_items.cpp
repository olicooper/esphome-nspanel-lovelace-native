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
    PageItem(uuid), PageItem_Icon(this, 65535u),
    navigation_uuid_(navigation_uuid) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const std::string &icon_default_value) : 
    PageItem(uuid), PageItem_Icon(this, icon_default_value, 65535u),
    navigation_uuid_(navigation_uuid) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const uint16_t icon_default_color) : 
    PageItem(uuid), PageItem_Icon(this, icon_default_color),
    navigation_uuid_(navigation_uuid) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    PageItem(uuid),
    PageItem_Icon(this, icon_default_value, icon_default_color),
    navigation_uuid_(navigation_uuid) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
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
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &icon_default_value) :
    StatefulPageItem(uuid, std::move(entity), icon_default_value),
    alt_font_(false) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const uint16_t icon_default_color) :
    StatefulPageItem(uuid, std::move(entity), icon_default_color),
    alt_font_(false) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

StatusIconItem::StatusIconItem(
    const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    StatefulPageItem(uuid, std::move(entity),
      icon_default_value, icon_default_color),
    alt_font_(false) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
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
    PageItem(uuid), PageItem_Icon(this, 63878u), // change the default icon color: #ff3131 (red)
    PageItem_DisplayName(this),
    PageItem_Value(this, "0.0"), float_value_(0.0f) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

WeatherItem::WeatherItem(
    const std::string &uuid, const std::string &display_name, 
    const std::string &value, const char *weather_condition) :
    PageItem(uuid), PageItem_Icon(this, 63878u), 
    PageItem_DisplayName(this, display_name), 
    PageItem_Value(this, value), float_value_(0.0f) {
  this->set_icon_by_weather_condition(weather_condition);
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
}

void WeatherItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

// valid conditions are found in weather_type
void WeatherItem::set_icon_by_weather_condition(const std::string &condition) {
  auto icon = get_icon_by_name(WeatherItem::icon_color_map, condition);
  if (icon == nullptr)
    return;
  this->icon_color_ = icon->color;
  this->icon_value_ = icon->value;
  this->set_render_invalid();
}

bool WeatherItem::set_value(const std::string &value) {
  if (sscanf(value.c_str(), "%f", &this->float_value_) != 1)
    return false;
  this->value_ = value;
  this->render_invalid_ = true;
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

// clang-format off
char_icon_map WeatherItem::icon_color_map = {
  {weather_type::sunny,           {u8"\uE598", 65504u}}, // mdi:0599,#ffff00
  {weather_type::windy,           {u8"\uE59C", 38066u}}, // mdi:059D,#949694
  {weather_type::windy_variant,   {u8"\uE59D", 64495u}}, // mdi:059E,#ff7d7b
  {weather_type::cloudy,          {u8"\uE58F", 31728u}}, // mdi:0590,#7b7d84
  {weather_type::partlycloudy,    {u8"\uE594", 38066u}}, // mdi:0595,#949694
  {weather_type::clear_night,     {u8"\uE593", 38060u}}, // mdi:0594,#949663 // weather-night
  {weather_type::exceptional,     {u8"\uE5D5", 63878u}}, // mdi:05D6,#ff3131 // alert-circle-outline
  {weather_type::rainy,           {u8"\uE596", 25375u}}, // mdi:0597,#6361ff
  {weather_type::pouring,         {u8"\uE595", 12703u}}, // mdi:0596,#3131ff
  {weather_type::snowy,           {u8"\uE597", 65535u}}, // mdi:E598,#ffffff
  {weather_type::snowy_rainy,     {u8"\uEF34", 38079u}}, // mdi:067F,#9496ff
  {weather_type::fog,             {u8"\uE590", 38066u}}, // mdi:0591,#949694
  {weather_type::hail,            {u8"\uE591", 65535u}}, // mdi:0592,#ffffff
  {weather_type::lightning,       {u8"\uE592", 65120u}}, // mdi:0593,#ffce00
  {weather_type::lightning_rainy, {u8"\uE67D", 50400u}}  // mdi:067E,#c59e00
};
// clang-format on

std::string WeatherItem::temperature_unit = "°C";

/*
 * =============== AlarmButtonItem ===============
 */

AlarmButtonItem::AlarmButtonItem(const std::string &uuid,
    const char *action_type, const std::string &display_name) :
    PageItem(uuid), PageItem_DisplayName(this, display_name),
    action_type_(action_type) {
  this->render_buffer_.reserve(this->get_render_buffer_reserve_());
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
    PageItem(uuid), PageItem_Icon(this, icon_default_value, icon_default_color) {}

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