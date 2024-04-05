#include "page_items.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include <array>
#include <functional>
#include <string>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== NavigationItem ===============
 */

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid) : 
    PageItem(uuid), PageItem_EntityId(this, navigation_uuid), 
    PageItem_Icon(this, 65535u) {}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const std::string &icon_default_value) : 
    PageItem(uuid), PageItem_EntityId(this, navigation_uuid), 
    PageItem_Icon(this, icon_default_value, 65535u) {}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const uint16_t icon_default_color) : 
    PageItem(uuid), PageItem_EntityId(this, navigation_uuid), 
    PageItem_Icon(this, icon_default_color) {}

NavigationItem::NavigationItem(
    const std::string &uuid, const std::string &navigation_uuid, 
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_EntityId(this, navigation_uuid), 
    PageItem_Icon(this, icon_default_value, icon_default_color) {}

void NavigationItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &NavigationItem::render_(std::string &buffer) {
  // note: should be able to skip type field but it doesn't render if I do
  // type~
  buffer.assign(entity_type::button).append(1, SEPARATOR);
  // internalName(navigate.uuid.[page uuid])~
  buffer.append(entity_type::navigate_uuid)
    .append(1,'.').append(entity_id_).append(1, SEPARATOR);
  // icon~iconColor
  PageItem_Icon::render_(buffer);
  // skip: ~displayName~value
  return buffer.append(2, SEPARATOR);
}

/*
 * =============== IconItem ===============
 */

IconItem::IconItem(const std::string &uuid, const std::string &entity_id) : 
    PageItem(uuid), PageItem_EntityId(this, entity_id),
    PageItem_Icon(this), PageItem_DisplayName(this), PageItem_Value(this) {}
IconItem::IconItem(
    const std::string &uuid, const std::string &entity_id, 
    const std::string &icon_default_value) :
    PageItem(uuid), PageItem_EntityId(this, entity_id),
    PageItem_Icon(this, icon_default_value),
    PageItem_DisplayName(this), PageItem_Value(this) {}
IconItem::IconItem(
    const std::string &uuid, const std::string &entity_id, 
    const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_EntityId(this, entity_id), 
    PageItem_Icon(this, icon_default_color),
    PageItem_DisplayName(this), PageItem_Value(this) {}
IconItem::IconItem(
    const std::string &uuid, const std::string &entity_id, 
    const std::string &icon_default_value, const uint16_t icon_default_color) :
    PageItem(uuid), PageItem_EntityId(this, entity_id), 
    PageItem_Icon(this, icon_default_value, icon_default_color),
    PageItem_DisplayName(this), PageItem_Value(this) {}

void IconItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &IconItem::render_(std::string &buffer) {
  // skip: type~internalName~
  buffer.assign(2, SEPARATOR);
  // icon~iconColor~
  PageItem_Icon::render_(buffer).append(1, SEPARATOR);
  // displayName~
  PageItem_DisplayName::render_(buffer).append(1, SEPARATOR);
  // value
  return PageItem_Value::render_(buffer);
}

/*
 * =============== WeatherItem ===============
 */

WeatherItem::WeatherItem(const std::string &uuid) :
    PageItem(uuid), PageItem_Icon(this, 63878u), // change the default icon color: #ff3131 (red)
    PageItem_DisplayName(this), PageItem_Value(this, "0.0"), 
    float_value_(0.0f) {
}

WeatherItem::WeatherItem(
    const std::string &uuid, const std::string &display_name, 
    const std::string &value, const char *weather_condition) :
    PageItem(uuid), PageItem_Icon(this, 63878u), 
    PageItem_DisplayName(this, display_name), 
    PageItem_Value(this, value) {
  this->set_icon_by_weather_condition(weather_condition);
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
  buffer.assign(2, SEPARATOR);
  PageItem_Icon::render_(buffer).append(1, SEPARATOR);
  PageItem_DisplayName::render_(buffer).append(1, SEPARATOR);
  // allow the value to be fomatted based on locale instead of using the raw string value
  return buffer.append(string_sprintf("%.1f", this->float_value_))
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

} // namespace nspanel_lovelace
} // namespace esphome