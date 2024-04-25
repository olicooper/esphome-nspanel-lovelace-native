#pragma once

#include "config.h"
#include "entity.h"
#include "helpers.h"
#include "page_item_base.h"
#include "page_item_visitor.h"
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

class NavigationItem :
    public PageItem,
    public PageItem_Icon {
public:
  NavigationItem(
      const std::string &uuid, const std::string &navigation_uuid);
  NavigationItem(
      const std::string &uuid, const std::string &navigation_uuid, 
      const std::string &icon_default_value);
  NavigationItem(
      const std::string &uuid, const std::string &navigation_uuid, 
      const uint16_t icon_default_color);
  NavigationItem(
      const std::string &uuid, const std::string &navigation_uuid, 
      const std::string &icon_default_value, const uint16_t icon_default_color);
  // virtual ~NavigationItem() {}

  void accept(PageItemVisitor& visitor) override;

protected:
  std::string navigation_uuid_;
  // output: ~internalName~icon~iconColor~~
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== StatusIconItem ===============
 */

class StatusIconItem : public StatefulPageItem {
public:
  StatusIconItem(const std::string &uuid, std::shared_ptr<Entity> entity);
  StatusIconItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const std::string &icon_default_value);
  StatusIconItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const uint16_t icon_default_color);
  StatusIconItem(
      const std::string &uuid, std::shared_ptr<Entity> entity,
      const std::string &icon_default_value,
      const uint16_t icon_default_color);
  // virtual ~StatusIconItem() {}

  void accept(PageItemVisitor& visitor) override;

  bool get_alt_font() const { return this->alt_font_; }
  
  void set_alt_font(bool large) { this->alt_font_ = large; }

protected:
  bool alt_font_;

  // output: icon~iconColor
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== WeatherItem ===============
 */

class WeatherItem :
    public PageItem,
    public PageItem_Icon,
    public PageItem_DisplayName,
    public PageItem_Value {
public:
  WeatherItem(const std::string &uuid);
  WeatherItem(
      const std::string &uuid, const std::string &display_name, 
      const std::string &value, const char *weather_condition);
  // virtual ~WeatherItem() {}

  void accept(PageItemVisitor& visitor) override;

  void set_icon_by_weather_condition(const std::string &condition);
  bool set_value(const std::string &value) override;

  // A map of icons and their respective color for each weather condition
  // see:
  //  - https://www.home-assistant.io/integrations/weather/
  //  - 'get_entity_color' function in:
  //  https://github.com/joBr99/nspanel-lovelace-ui/blob/main/apps/nspanel-lovelace-ui/luibackend/pages.py
  //  - icon lookup:
  //      - codepoint values: https://docs.nspanel.pky.eu/icon-cheatsheet.html
  //      - icon mapping:
  //      https://github.com/joBr99/nspanel-lovelace-ui/blob/main/apps/nspanel-lovelace-ui/luibackend/icon_mapping.py
  //      - mdi icons: https://pictogrammers.com/library/mdi/
  //  - color lookup:
  //      - https://rgbcolorpicker.com/565
  static char_icon_map icon_color_map;
  // The temperature unit all weather items will use
  static std::string temperature_unit;

protected:
  float float_value_;

  // output: ~~icon~iconColor~displayName~value
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== AlarmButtonItem ===============
 */

class AlarmButtonItem :
    public PageItem,
    public PageItem_DisplayName {
public:
  AlarmButtonItem(const std::string &uuid,
      const char *action_type, const std::string &display_name);
  // virtual ~AlarmButtonItem() {}

  void accept(PageItemVisitor& visitor) override;

protected:
  const char *action_type_;
  // output: displayName~action
  std::string &render_(std::string &buffer) override;
};

/*
 * =============== AlarmIconItem ===============
 */

class AlarmIconItem :
    public PageItem,
    public PageItem_Icon {
public:
  AlarmIconItem(const std::string &uuid,
      const std::string &icon_default_value, const uint16_t icon_default_color);
  // virtual ~AlarmIconItem() {}

  void accept(PageItemVisitor& visitor) override;

protected:
  // output: icon~iconColor
  std::string &render_(std::string &buffer) override;
};

} // namespace nspanel_lovelace
} // namespace esphome