#pragma once

#include <array>
#include <cassert>
#include <map>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>

#include "helpers.h"

namespace esphome {
namespace nspanel_lovelace {

enum class render_page_option : uint8_t { prev, next, screensaver, default_page };

enum class alarm_arm_action : uint8_t { arm_home, arm_away, arm_night, arm_vacation };
struct alarm_entity_state {
  static constexpr const char* disarmed = "disarmed";
  static constexpr const char* arming = "arming";
  static constexpr const char* pending = "pending";
  static constexpr const char* triggered = "triggered";
  static constexpr const char* armed_home = "armed_home";
  static constexpr const char* armed_away = "armed_away";
  static constexpr const char* armed_night = "armed_night";
  static constexpr const char* armed_vacation = "armed_vacation";
  static constexpr const char* armed_custom_bypass = "armed_custom_bypass";
};

static const std::array<std::array<const char *, 2>, 7> dow_names = {
    {{"Sun", "Sunday"},
     {"Mon", "Monday"},
     {"Tue", "Tuesday"},
     {"Wed", "Wednesday"},
     {"Thu", "Thursday"},
     {"Fri", "Friday"},
     {"Sat", "Saturday"}}};

class DayOfWeekMap {
public:
  enum dow : uint8_t { sunday, monday, tuesday, wednesday, thursday, friday, saturday };
  enum dow_mode : uint8_t { 
    none = 0,
    short_dow = 1 << 0,
    long_dow = 1 << 1,
    both = short_dow | long_dow
  };

  DayOfWeekMap() :
      sunday_overridden_(false),
      monday_overridden_(false),
      tuesday_overridden_(false),
      wednesday_overridden_(false),
      thursday_overridden_(false),
      friday_overridden_(false),
      saturday_overridden_(false),
      sunday_(dow_names.at(0)),
      monday_(dow_names.at(1)),
      tuesday_(dow_names.at(2)),
      wednesday_(dow_names.at(3)),
      thursday_(dow_names.at(4)),
      friday_(dow_names.at(5)),
      saturday_(dow_names.at(6)) {}

  void set_sunday(const std::array<const char *, 2> &sunday) {
    this->sunday_ = sunday;
    this->sunday_overridden_ = true;
  }
  void set_monday(const std::array<const char *, 2> &monday) {
    this->monday_ = monday;
    this->monday_overridden_ = true;
  }
  void set_tuesday(const std::array<const char *, 2> &tuesday) {
    this->tuesday_ = tuesday;
    this->tuesday_overridden_ = true;
  }
  void set_wednesday(const std::array<const char *, 2> &wednesday) {
    this->wednesday_ = wednesday;
    this->wednesday_overridden_ = true;
  }
  void set_thursday(const std::array<const char *, 2> &thursday) {
    this->thursday_ = thursday;
    this->thursday_overridden_ = true;
  }
  void set_friday(const std::array<const char *, 2> &friday) {
    this->friday_ = friday;
    this->friday_overridden_ = true;
  }
  void set_saturday(const std::array<const char *, 2> &saturday) {
    this->saturday_ = saturday;
    this->saturday_overridden_ = true;
  }

  std::string &replace(std::string &str, dow_mode mode) {
    if (mode == dow_mode::none)
      return str;

    if ((mode & dow_mode::long_dow) == dow_mode::long_dow) {
      if (sunday_overridden_)
        replace_first(str, dow_names.at(0).at(1), sunday_.at(1));
      if (monday_overridden_)
        replace_first(str, dow_names.at(1).at(1), monday_.at(1));
      if (tuesday_overridden_)
        replace_first(str, dow_names.at(2).at(1), tuesday_.at(1));
      if (wednesday_overridden_)
        replace_first(str, dow_names.at(3).at(1), wednesday_.at(1));
      if (thursday_overridden_)
        replace_first(str, dow_names.at(4).at(1), thursday_.at(1));
      if (friday_overridden_)
        replace_first(str, dow_names.at(5).at(1), friday_.at(1));
      if (saturday_overridden_)
        replace_first(str, dow_names.at(6).at(1), saturday_.at(1));
    }
    if ((mode & dow_mode::short_dow) == dow_mode::short_dow) {
      if (sunday_overridden_)
        replace_first(str, dow_names.at(0).at(0), sunday_.at(0));
      if (monday_overridden_)
        replace_first(str, dow_names.at(1).at(0), monday_.at(0));
      if (tuesday_overridden_)
        replace_first(str, dow_names.at(2).at(0), tuesday_.at(0));
      if (wednesday_overridden_)
        replace_first(str, dow_names.at(3).at(0), wednesday_.at(0));
      if (thursday_overridden_)
        replace_first(str, dow_names.at(4).at(0), thursday_.at(0));
      if (friday_overridden_)
        replace_first(str, dow_names.at(5).at(0), friday_.at(0));
      if (saturday_overridden_)
        replace_first(str, dow_names.at(6).at(0), saturday_.at(0));
    }
    return str;
  }

  const std::array<const char *, 2> &at(size_t index) const {
    return operator[](index);
  }

  const std::array<const char *, 2> &operator[](size_t index) const {
    assert(index < 7);
    switch (index) {
    case dow::sunday:
      return sunday_;
    case dow::monday:
      return monday_;
    case dow::tuesday:
      return tuesday_;
    case dow::wednesday:
      return wednesday_;
    case dow::thursday:
      return thursday_;
    case dow::friday:
      return friday_;
    case dow::saturday:
      return saturday_;
    }
    // should never reach here
    return sunday_;
  }

private:
  bool sunday_overridden_, monday_overridden_, tuesday_overridden_,
    wednesday_overridden_, thursday_overridden_, friday_overridden_, 
    saturday_overridden_;

  std::array<const char *, 2> sunday_;
  std::array<const char *, 2> monday_;
  std::array<const char *, 2> tuesday_;
  std::array<const char *, 2> wednesday_;
  std::array<const char *, 2> thursday_;
  std::array<const char *, 2> friday_;
  std::array<const char *, 2> saturday_;
};

// todo: populate a list of all used icons
struct icon_t {
  static constexpr const char* alert_circle_outline = u8"\uE5D5";
  static constexpr const char* pause = u8"\uE3E3";
  static constexpr const char* play = u8"\uE409";
  static constexpr const char* shuffle = u8"\uE49C";
  static constexpr const char* shuffle_disable = u8"\uE49D";
};

struct Icon {
  // The codepoint value
  // default: mdi:05D6 (alert-circle-outline)
  std::string value;
  // The rgb565 color
  // default: #ff3131 (red)
  uint16_t color;
  // The string representation of the rgb565 color
  const std::string color_str() const { return std::to_string(color); }

  Icon() : Icon(icon_t::alert_circle_outline, 63878u) { }
  Icon(const std::string value, const uint16_t color) : value(value), color(color) { }
};

struct generic_type {
  static constexpr const char* enable = "enable";
  static constexpr const char* disable = "disable";
  static constexpr const char* unknown = "unknown";
  static constexpr const char* unavailable = "unavailable";
  static constexpr const char* on = "on";
  static constexpr const char* off = "off";
  static constexpr const char* empty = "";
};

typedef std::map<const char*, const char*, compare_char_str> char_map;
typedef std::map<const char*, Icon, compare_char_str> char_icon_map;
typedef std::map<const char*, std::vector<const char *>, compare_char_str> char_list_map;

struct weather_type {
  static constexpr const char* sunny = "sunny";
  static constexpr const char* windy = "windy";
  static constexpr const char* windy_variant = "windy-variant";
  static constexpr const char* cloudy = "cloudy";
  static constexpr const char* partlycloudy = "partlycloudy";
  static constexpr const char* clear_night = "clear-night";
  static constexpr const char* exceptional = "exceptional";
  static constexpr const char* rainy = "rainy";
  static constexpr const char* pouring = "pouring";
  static constexpr const char* snowy = "snowy";
  static constexpr const char* snowy_rainy = "snowy-rainy";
  static constexpr const char* fog = "fog";
  static constexpr const char* hail = "hail";
  static constexpr const char* lightning = "lightning";
  static constexpr const char* lightning_rainy = "lightning-rainy";
};

struct button_type {
  static constexpr const char* bExit = "bExit";
  static constexpr const char* sleepReached = "sleepReached";
  static constexpr const char* onOff = "OnOff";
  static constexpr const char* numberSet = "number-set";
  static constexpr const char* button = "button";

  // shutters and covers
  static constexpr const char* up = "up";
  static constexpr const char* stop = "stop";
  static constexpr const char* down = "down";
  static constexpr const char* positionSlider = "positionSlider";
  static constexpr const char* tiltOpen = "tiltOpen";
  static constexpr const char* tiltStop = "tiltStop";
  static constexpr const char* tiltClose = "tiltClose";
  static constexpr const char* tiltSlider = "tiltSlider";

  // media page
  static constexpr const char* mediaNext = "media-next";
  static constexpr const char* mediaBack = "media-back";
  static constexpr const char* mediaPause = "media-pause";
  static constexpr const char* mediaOnOff = "media-OnOff";
  static constexpr const char* mediaShuffle = "media-shuffle";
  static constexpr const char* volumeSlider = "volumeSlider";
  static constexpr const char* speakerSel = "speaker-sel";

  // light page
  static constexpr const char* brightnessSlider = "brightnessSlider";
  static constexpr const char* colorTempSlider = "colorTempSlider";
  static constexpr const char* colorWheel = "colorWheel";

  // climate page
  static constexpr const char* tempUpd = "tempUpd";
  static constexpr const char* tempUpdHighLow = "tempUpdHighLow";
  static constexpr const char* hvacAction = "hvac_action";

  // alarm page
  static constexpr const char* disarm = "disarm";
  static constexpr const char* armHome = "arm_home";
  static constexpr const char* armAway = "arm_away";
  static constexpr const char* armNight = "arm_night";
  static constexpr const char* armVacation = "arm_vacation";
  static constexpr const char* opnSensorNotify = "opnSensorNotify";

  // cardUnlock
  static constexpr const char* cardUnlockUnlock = "cardUnlock-unlock";
  static constexpr const char* modePresetModes = "mode-preset_modes";
  static constexpr const char* modeSwingModes = "mode-swing_modes";
  static constexpr const char* modeFanModes = "mode-fan_modes";
  static constexpr const char* modeInputSelect = "mode-input_select";
  static constexpr const char* modeLight = "mode-light";
  static constexpr const char* modeMediaPlayer = "mode-media_player";

  // timer detail page
  static constexpr const char* timerStart = "timer-start";
  static constexpr const char* timerCancel = "timer-cancel";
  static constexpr const char* timerPause = "timer-pause";
  static constexpr const char* timerFinish = "timer-finish";
};

struct entity_type {
  static constexpr const char* scene = "scene";
  static constexpr const char* script = "script";
  static constexpr const char* light = "light";
  static constexpr const char* switch_ = "switch";
  static constexpr const char* input_boolean = "input_boolean";
  static constexpr const char* automation = "automation";
  static constexpr const char* fan = "fan";
  static constexpr const char* lock = "lock";
  static constexpr const char* button = "button";
  static constexpr const char* input_button = "input_button";
  static constexpr const char* input_select = "input_select";
  static constexpr const char* number = "number";
  static constexpr const char* input_number = "input_number";
  static constexpr const char* vacuum = "vacuum";
  static constexpr const char* timer = "timer";
  static constexpr const char* person = "person";
  static constexpr const char* service = "service";
  
  static constexpr const char* cover = "cover";
  static constexpr const char* sensor = "sensor";
  static constexpr const char* binary_sensor = "binary_sensor";
  static constexpr const char* input_text = "input_text";
  static constexpr const char* select = "select";
  static constexpr const char* alarm_control_panel = "alarm_control_panel";
  static constexpr const char* media_player = "media_player";
  static constexpr const char* sun = "sun";
  static constexpr const char* climate = "climate";
  static constexpr const char* weather = "weather";

  // internal (non HA) types
  static constexpr const char* nav_up = "navUp";
  static constexpr const char* nav_prev = "navPrev";
  static constexpr const char* nav_next = "navNext";
  static constexpr const char* uuid = "uuid";
  static constexpr const char* navigate = "navigate";
  static constexpr const char* navigate_uuid = "navigate.uuid";
  static constexpr const char* itext = "iText";
  static constexpr const char* delete_ = "delete";
};

struct entity_render_type {
  static constexpr const char* text = "text";
  static constexpr const char* shutter = "shutter";
  static constexpr const char* input_sel = "input_sel";
  static constexpr const char* media_pl = "media_pl";
};

struct entity_cover_type {
  static constexpr const char* awning = "awning";
  static constexpr const char* blind = "blind";
  static constexpr const char* curtain = "curtain";
  static constexpr const char* damper = "damper";
  static constexpr const char* door = "door";
  static constexpr const char* garage = "garage";
  static constexpr const char* gate = "gate";
  static constexpr const char* shade = "shade";
  static constexpr const char* shutter = "shutter";
  static constexpr const char* window = "window";
};

struct sensor_type {
  // sensor_mapping
  static constexpr const char* apparent_power = "apparent_power";
  static constexpr const char* aqi = "aqi";
  static constexpr const char* battery = "battery";
  static constexpr const char* carbon_dioxide = "carbon_dioxide";
  static constexpr const char* carbon_monoxide = "carbon_monoxide";
  static constexpr const char* current = "current";
  static constexpr const char* date = "date";
  static constexpr const char* duration = "duration";
  static constexpr const char* energy = "energy";
  static constexpr const char* frequency = "frequency";
  static constexpr const char* gas = "gas";
  static constexpr const char* humidity = "humidity";
  static constexpr const char* illuminance = "illuminance";
  static constexpr const char* monetary = "monetary";
  static constexpr const char* nitrogen_dioxide = "nitrogen_dioxide";
  static constexpr const char* nitrogen_monoxide = "nitrogen_monoxide";
  static constexpr const char* nitrous_oxide = "nitrous_oxide";
  static constexpr const char* ozone = "ozone";
  static constexpr const char* pm1 = "pm1";
  static constexpr const char* pm10 = "pm10";
  static constexpr const char* pm25 = "pm25";
  static constexpr const char* power_factor = "power_factor";
  static constexpr const char* power = "power";
  static constexpr const char* pressure = "pressure";
  static constexpr const char* reactive_power = "reactive_power";
  static constexpr const char* signal_strength = "signal_strength";
  static constexpr const char* sulphur_dioxide = "sulphur_dioxide";
  static constexpr const char* temperature = "temperature";
  static constexpr const char* timestamp = "timestamp";
  static constexpr const char* volatile_organic_compounds = "volatile_organic_compounds";
  static constexpr const char* voltage = "voltage";

  // sensor_mapping_on
  static constexpr const char* battery_charging = "battery_charging";
  static constexpr const char* cold = "cold";
  static constexpr const char* connectivity = "connectivity";
  static constexpr const char* door = "door";
  static constexpr const char* garage_door = "garage_door";
  static constexpr const char* problem = "problem";
  static constexpr const char* safety = "safety";
  static constexpr const char* tamper = "tamper";
  static constexpr const char* smoke = "smoke";
  static constexpr const char* heat = "heat";
  static constexpr const char* light = "light";
  static constexpr const char* lock = "lock";
  static constexpr const char* moisture = "moisture";
  static constexpr const char* motion = "motion";
  static constexpr const char* occupancy = "occupancy";
  static constexpr const char* opening = "opening";
  static constexpr const char* plug = "plug";
  static constexpr const char* presence = "presence";
  static constexpr const char* running = "running";
  static constexpr const char* sound = "sound";
  static constexpr const char* update = "update";
  static constexpr const char* vibration = "vibration";
  static constexpr const char* window = "window";
};

enum class page_type : uint8_t {
  unknown,
  screensaver,
  screensaver2,
  cardGrid,
  cardGrid2,
  cardEntities,
  cardThermo,
  cardMedia,
  cardUnlock,
  cardQR,
  cardPower,
  cardAlarm,
  popupLight,
};
static constexpr const char* page_type_names [] = {
  "",
  "screensaver",
  "screensaver2",
  "cardGrid",
  "cardGrid2",
  "cardEntities",
  "cardThermo",
  "cardMedia",
  "cardUnlock",
  "cardQR",
  "cardPower",
  "cardAlarm",
  "popupLight",
};

inline const char *to_string(page_type type) {
  if ((size_t)type >= (sizeof(page_type_names) / sizeof(*page_type_names)))
    return nullptr;
  return page_type_names[(uint8_t)type];
}

struct action_type {
  static constexpr const char* buttonPress2 = "buttonPress2";
  static constexpr const char* pageOpenDetail = "pageOpenDetail";
  static constexpr const char* sleepReached = "sleepReached";
  static constexpr const char* startup = "startup";
};

struct ha_action_type {
  static constexpr const char* turn_on = "turn_on";
  static constexpr const char* turn_off = "turn_off";
  static constexpr const char* press = "press";
  static constexpr const char* toggle = "toggle";
  static constexpr const char* open_cover = "open_cover";
  static constexpr const char* close_cover = "close_cover";
  static constexpr const char* stop_cover = "stop_cover";
  static constexpr const char* set_cover_position = "set_cover_position";
  static constexpr const char* open_cover_tilt = "open_cover_tilt";
  static constexpr const char* close_cover_tilt = "close_cover_tilt";
  static constexpr const char* stop_cover_tilt = "stop_cover_tilt";
  static constexpr const char* set_cover_tilt_position = "set_cover_tilt_position";
  static constexpr const char* set_temperature = "set_temperature";
  static constexpr const char* set_hvac_mode = "set_hvac_mode";
  static constexpr const char* set_preset_mode = "set_preset_mode";
  static constexpr const char* set_swing_mode = "set_swing_mode";
  static constexpr const char* set_fan_mode = "set_fan_mode";
  static constexpr const char* media_next_track = "media_next_track";
  static constexpr const char* media_previous_track = "media_previous_track";
  static constexpr const char* media_play_pause = "media_play_pause";
  static constexpr const char* shuffle_set = "shuffle_set";
  static constexpr const char* volume_set = "volume_set";
  static constexpr const char* select_source = "select_source";
};

enum class ha_attr_type : uint8_t {
  unknown,
  // general
  entity_id,
  state,
  device_class,
  supported_features,
  unit_of_measurement,
  // light
  brightness,
  min_mireds,
  max_mireds,
  supported_color_modes,
  color_mode,
  color_temp,
  rgb_color,
  // alarm_control_panel
  code,
  code_arm_required,
  // cover
  current_position,
  position,
  current_tilt_position,
  tilt_position,
  // weather & climate
  temperature,
  temperature_unit,
  // timer
  duration,
  remaining,
  editable,
  finishes_at,
  // climate
  current_temperature,
  target_temp_high,
  target_temp_low,
  target_temp_step,
  min_temp,
  max_temp,
  hvac_action,
  preset_mode,
  preset_modes,
  swing_mode,
  swing_modes,
  fan_mode,
  fan_modes,
  hvac_mode,
  hvac_modes,
  // media
  media_title,
  media_artist,
  volume_level,
  shuffle,
  media_content_type,
  source,
  source_list,
};

static constexpr const char* ha_attr_names [] = {
  "",
  // general
  "entity_id",
  "state",
  "device_class",
  "supported_features",
  "unit_of_measurement",
  // light
  "brightness",
  "min_mireds",
  "max_mireds",
  "supported_color_modes",
  "color_mode",
  "color_temp",
  "rgb_color",
  // alarm_control_panel
  "code",
  "code_arm_required",
  // cover
  "current_position",
  "position",
  "current_tilt_position",
  "tilt_position",
  // weather & climate
  "temperature",
  "temperature_unit",
  // timer
  "duration",
  "remaining",
  "editable",
  "finishes_at",
  // climate
  "current_temperature",
  "target_temp_high",
  "target_temp_low",
  "target_temp_step",
  "min_temp",
  "max_temp",
  "hvac_action",
  "preset_mode",
  "preset_modes",
  "swing_mode",
  "swing_modes",
  "fan_mode",
  "fan_modes",
  "hvac_mode",
  "hvac_modes",
  // media
  "media_title",
  "media_artist",
  "volume_level",
  "shuffle",
  "media_content_type",
  "source",
  "source_list",
};

inline const char *to_string(ha_attr_type attr) {
  if ((size_t)attr >= (sizeof(ha_attr_names) / sizeof(*ha_attr_names)))
    return nullptr;
  return ha_attr_names[(uint8_t)attr];
}

inline ha_attr_type to_ha_attr(const std::string &attr) {
  for (uint8_t i = 0; i < (sizeof(ha_attr_names) / sizeof(*ha_attr_names)); i++) {
    if (attr == ha_attr_names[i]) return static_cast<ha_attr_type>(i);
  }
  return ha_attr_type::unknown;
}

struct ha_attr_color_mode {
  static constexpr const char* onoff = "onoff";
  static constexpr const char* color_temp = "color_temp";
  static constexpr const char* xy = "xy";
  static constexpr const char* hs = "hs";
  static constexpr const char* rgb = "rgb";
  static constexpr const char* rgbw = "rgbw";
  static constexpr const char* rgbww = "rgbww";
};

struct ha_attr_hvac_mode {
  static constexpr const char* auto_ = "auto";
  static constexpr const char* heat_cool = "heat_cool";
  static constexpr const char* heat = "heat";
  static constexpr const char* off = "off";
  static constexpr const char* cool = "cool";
  static constexpr const char* dry = "dry";
  static constexpr const char* fan_only = "fan_only";
};

struct ha_attr_media_content_type {
  static constexpr const char* music = "music";
  static constexpr const char* tvshow = "tvshow";
  static constexpr const char* video = "video";
  static constexpr const char* episode = "episode";
  static constexpr const char* channel = "channel";
  static constexpr const char* playlist = "playlist";
  static constexpr const char* app = "app";
  static constexpr const char* url = "url";
};

enum class datetime_mode : uint8_t {
  date = 1<<0,
  time = 1<<1,
  both = date | time
};
inline datetime_mode operator|(datetime_mode a, datetime_mode b) {
  return static_cast<datetime_mode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
inline datetime_mode operator&(datetime_mode a, datetime_mode b) {
  return static_cast<datetime_mode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline const char *get_icon_by_name(
    const char_map &map,
    const std::string& icon_name,
    const std::string& fallback_icon_name = "",
    const char *default_icon = nullptr) {
  if (map.size() > 0) {
    if (icon_name.empty()) {
      if (!fallback_icon_name.empty()) {
        auto icon_it = map.find(fallback_icon_name.c_str());
        if (icon_it != map.end()) return icon_it->second;
      }
    } else {
      auto icon_it = map.find(icon_name.c_str());
      if (icon_it != map.end()) return icon_it->second;
    }
  }
  return default_icon == nullptr 
    ? icon_t::alert_circle_outline
    : default_icon;
}

inline const std::vector<const char*> *get_icon_by_name(
    const char_list_map &map,
    const std::string &icon_name,
    const std::string& fallback_icon_name = "",
    const std::vector<const char*> *default_icon_list = nullptr) {
  if (map.size() > 0) {
    if (icon_name.empty()) {
      if (!fallback_icon_name.empty()) {
        auto icon_it = map.find(fallback_icon_name.c_str());
        if (icon_it != map.end()) return &icon_it->second;
      }
    } else {
      auto icon_it = map.find(icon_name.c_str());
      if (icon_it != map.end()) return &icon_it->second;
    }
  }
  return default_icon_list;
}

inline const Icon *get_icon_by_name(
    const char_icon_map &map,
    const std::string &icon_name,
    const std::string& fallback_icon_name = "",
    const Icon *default_icon = nullptr) {
  if (map.size() > 0) {
    if (icon_name.empty()) {
      if (!fallback_icon_name.empty()) {
        auto icon_it = map.find(fallback_icon_name.c_str());
        if (icon_it != map.end()) return &icon_it->second;
      }
    } else {
      auto icon_it = map.find(icon_name.c_str());
      if (icon_it != map.end()) return &icon_it->second;
    }
  }
  static const Icon default_icon_obj;
  return default_icon == nullptr 
    ? &default_icon_obj
    : default_icon;
}

// simple_type_mapping
const char_map ENTITY_ICON_MAP {
  {entity_type::button, u8"\uF2A7"}, // gesture-tap-button
  {entity_type::navigate, u8"\uF2A7"}, // gesture-tap-button
  {entity_type::input_button, u8"\uF2A7"}, // gesture-tap-button
  {entity_type::input_select, u8"\uF2A7"}, // gesture-tap-button
  {entity_type::scene, u8"\uE3D7"}, // palette
  {entity_type::script, u8"\uEBC1"}, // script-text
  {entity_type::switch_, u8"\uE97D"}, // light-switch
  {entity_type::automation, u8"\uE6A8"}, // robot
  {entity_type::number, u8"\uE444"}, // ray-vertex
  {entity_type::input_number, u8"\uE444"}, // ray-vertex
  {entity_type::light, u8"\uE334"}, // lightbulb
  {entity_type::fan, u8"\uE20F"}, // fan
  {entity_type::person, u8"\uE003"}, // account
  {entity_type::vacuum, u8"\uE70C"}, // robot-vacuum
  {entity_type::timer, u8"\uE51A"}, // timer-outline

  {entity_type::nav_up, u8"\uE736"}, // arrow-up-bold
  {entity_type::nav_prev, u8"\uE730"}, // arrow-left-bold
  {entity_type::nav_next, u8"\uE733"}, //arrow-right-bold
  {entity_type::itext, u8"\uE69D"}, // format-color-text
  {entity_type::input_text, u8"\uE5E6"}, // cursor-text
};

// sensor_mapping_on
const char_map SENSOR_ON_ICON_MAP {
  {sensor_type::battery, u8"\uE08D"}, // battery-outline
  {sensor_type::battery_charging, u8"\uE083"}, // battery-charging
  {sensor_type::carbon_monoxide, u8"\uF92D"}, // smoke-detector-alert
  {sensor_type::cold, u8"\uE716"}, // snowflake
  {sensor_type::connectivity, u8"\uEC53"}, // check-network-outline
  {sensor_type::door, u8"\uE81B"}, // door-open
  {sensor_type::garage_door, u8"\uE6D9"}, // garage-open
  {sensor_type::power, u8"\uE6A4"}, // power-plug
  {sensor_type::gas, u8"\uE027"}, // alert-circle
  {sensor_type::problem, u8"\uE027"}, // alert-circle
  {sensor_type::safety, u8"\uE027"}, // alert-circle
  {sensor_type::tamper, u8"\uE027"}, // alert-circle
  {sensor_type::smoke, u8"\uF92F"}, // smoke-detector-variant-alert
  {sensor_type::heat, u8"\uE237"}, // fire
  {sensor_type::light, u8"\uE0DF"}, // brightness-7
  {sensor_type::lock, u8"\uE33E"}, // lock-open
  {sensor_type::moisture, u8"\uE58B"}, // water
  {sensor_type::motion, u8"\uED90"}, // motion-sensor
  {sensor_type::occupancy, u8"\uE2DB"}, // home
  {sensor_type::opening, u8"\uE762"}, // square-outline
  {sensor_type::plug, u8"\uE6A4"}, // power-plug
  {sensor_type::presence, u8"\uE2DB"}, // home
  {sensor_type::running, u8"\uE409"}, // play
  {sensor_type::sound, u8"\uE386"}, // music-note
  {sensor_type::update, u8"\uE3D4"}, // package-up
  {sensor_type::vibration, u8"\uE565"}, // vibrate
  {sensor_type::window, u8"\uE5B0"} // window-open
};

// sensor_mapping_off
const char_map SENSOR_OFF_ICON_MAP {
  {sensor_type::battery, u8"\uE078"}, // battery
  {sensor_type::battery_charging, u8"\uE078"}, // battery
  {sensor_type::carbon_monoxide, u8"\uE391"}, // smoke-detector
  {sensor_type::cold, u8"\uE50E"}, // thermometer
  {sensor_type::connectivity, u8"\uEC5E"}, // close-network-outline
  {sensor_type::door, u8"\uE81A"}, // door-closed
  {sensor_type::garage_door, u8"\uE6D8"}, // garage
  {sensor_type::power, u8"\uE6A5"}, // power-plug-off
  {sensor_type::gas, u8"\uE132"}, // checkbox-marked-circle
  {sensor_type::problem, u8"\uE132"}, // checkbox-marked-circle
  {sensor_type::safety, u8"\uE132"}, // checkbox-marked-circle
  {sensor_type::tamper, u8"\uE5DF"}, // check-circle
  {sensor_type::smoke, u8"\uF80A"}, // smoke-detector-variant
  {sensor_type::heat, u8"\uE50E"}, // thermometer
  {sensor_type::light, u8"\uE0DD"}, // brightness-5
  {sensor_type::lock, u8"\uE33D"}, // lock
  {sensor_type::moisture, u8"\uE58C"}, // water-off
  {sensor_type::motion, u8"\uF434"}, // motion-sensor-off
  {sensor_type::occupancy, u8"\uE6A0"}, // home-outline
  {sensor_type::opening, u8"\uE763"}, // square
  {sensor_type::plug, u8"\uE6A5"}, // power-plug-off
  {sensor_type::presence, u8"\uE6A0"}, // home-outline
  {sensor_type::running, u8"\uE4DA"}, // stop
  {sensor_type::sound, u8"\uE389"}, // music-note-off
  {sensor_type::update, u8"\uE3D2"}, // package
  {sensor_type::vibration, u8"\uE1A0"}, // crop-portrait
  {sensor_type::window, u8"\uE5AD"}, // window-closed
};

// sensor_mapping
const char_map SENSOR_ICON_MAP {
  {sensor_type::apparent_power, u8"\uE240"}, // flash
  {sensor_type::aqi, u8"\uEA70"}, // smog
  {sensor_type::battery, u8"\uE078"}, // battery
  {sensor_type::carbon_dioxide, u8"\uEA70"}, // smog
  {sensor_type::carbon_monoxide, u8"\uEA70"}, // smog
  {sensor_type::current, u8"\uE240"}, // flash
  {sensor_type::date, u8"\uE0EC"}, // calendar
  {sensor_type::duration, u8"\uF3AA"}, // timer
  {sensor_type::energy, u8"\uE240"}, // flash
  {sensor_type::frequency, u8"\uEC4F"}, // chart-bell-curve
  {sensor_type::gas, u8"\uE646"}, // gas-cylinder
  {sensor_type::humidity, u8"\uF098"}, // air-humidifier
  {sensor_type::illuminance, u8"\uE334"}, // lightbulb
  {sensor_type::monetary, u8"\uE113"}, // cash
  {sensor_type::nitrogen_dioxide, u8"\uEA70"}, // smog
  {sensor_type::nitrogen_monoxide, u8"\uEA70"}, // smog
  {sensor_type::nitrous_oxide, u8"\uEA70"}, // smog
  {sensor_type::ozone, u8"\uEA70"}, // smog
  {sensor_type::pm1, u8"\uEA70"}, // smog
  {sensor_type::pm10, u8"\uEA70"}, // smog
  {sensor_type::pm25, u8"\uEA70"}, // smog
  {sensor_type::power_factor, u8"\uE240"}, // flash
  {sensor_type::power, u8"\uE240"}, // flash
  {sensor_type::pressure, u8"\uE299"}, // gauge
  {sensor_type::reactive_power, u8"\uE240"}, // flash
  {sensor_type::signal_strength, u8"\uE4A1"}, // signal
  {sensor_type::sulphur_dioxide, u8"\uEA70"}, // smog
  {sensor_type::temperature, u8"\uE50E"}, // thermometer
  {sensor_type::timestamp, u8"\uE0EF"}, // calendar-clock
  {sensor_type::volatile_organic_compounds, u8"\uEA70"}, // smog
  {sensor_type::voltage, u8"\uE240"} // flash
};

// climate_mapping
const char_map CLIMATE_ICON_MAP {
  {ha_attr_hvac_mode::auto_, u8"\uEE8D"}, // calendar-sync
  {ha_attr_hvac_mode::heat_cool, u8"\uEE8D"}, // calendar-sync
  {ha_attr_hvac_mode::heat, u8"\uE237"}, // fire
  {ha_attr_hvac_mode::off, u8"\uE424"}, // power
  {ha_attr_hvac_mode::cool, u8"\uE716"}, // snowflake
  {ha_attr_hvac_mode::dry, u8"\uE58D"}, // water-percent
  {ha_attr_hvac_mode::fan_only, u8"\uE20F"}, // fan
};

const char_map MEDIA_TYPE_MAP {
  {generic_type::off, u8"\uE4C3"}, // speaker-off
  {ha_attr_media_content_type::music, u8"\uE759"}, // music
  {ha_attr_media_content_type::tvshow, u8"\uE380"}, // movie
  {ha_attr_media_content_type::video, u8"\uE566"}, // video
  {ha_attr_media_content_type::episode, u8"\uE410"}, // playlist-play (originally: icon_t::alert_circle_outline)
  {ha_attr_media_content_type::channel, u8"\uEDF1"}, //  playlist-star (OR radio-tower E43A / broadcast F71F?) (originally: icon_t::alert_circle_outline)
  {ha_attr_media_content_type::playlist, u8"\uECB7"}, // playlist-music (originally: icon_t::alert_circle_outline)
  {ha_attr_media_content_type::app, u8"\uE3CA"}, // newly added! open-in-app
  {ha_attr_media_content_type::url, u8"\uED1A"}, // newly added! link-box-outline (OR cast E117?)
};

// cover_mapping
const char_list_map COVER_MAP {
  // "device_class": ("icon-open", "icon-closed", "icon-cover-open", "icon-cover-close")
  {entity_cover_type::awning, {u8"\uE5B0", u8"\uE5AD", u8"\uE05C", u8"\uE044"}}, // window-open, window-closed, arrow-up, arrow-down
  {entity_cover_type::blind, {u8"\uF010", u8"\uE0AB", u8"\uE05C", u8"\uE044"}}, // blinds-open, blinds, arrow-up
  {entity_cover_type::curtain, {u8"\uF845", u8"\uF846", u8"\uE84D", u8"\uE84B"}}, // curtains, curtains-closed, arrow-expand-horizontal, arrow-collapse-horizontal
  {entity_cover_type::damper, {u8"\uE12E", u8"\uEAA4", u8"\uE05C", u8"\uE044"}}, // checkbox-blank-circle, circle-slice-8, arrow-up
  {entity_cover_type::door, {u8"\uE81B", u8"\uE81A", u8"\uE84D", u8"\uE84B"}}, // door-open, door-closed, arrow-expand-horizontal, arrow-collapse-horizontal
  {entity_cover_type::garage, {u8"\uE6D9", u8"\uE6D8", u8"\uE05C", u8"\uE044"}}, // garage-open, garage, arrow-up
  {entity_cover_type::gate, {u8"\uF169", u8"\uE298", u8"\uE84D", u8"\uE84B"}}, // gate-open, gate, arrow-expand-horizontal, arrow-collapse-horizontal
  {entity_cover_type::shade, {u8"\uF010", u8"\uE0AB", u8"\uE05C", u8"\uE044"}}, // blinds-open, blinds, arrow-up
  {entity_cover_type::shutter, {u8"\uF11D", u8"\uF11B", u8"\uE05C", u8"\uE044"}}, // window-shutter-open, window-shutter, arrow-up
  {entity_cover_type::window, {u8"\uE5B0", u8"\uE5AD", u8"\uE05C", u8"\uE044"}}, // window-open, window-closed, arrow-up
};

// see: 
//  - https://www.home-assistant.io/integrations/weather/
//  - 'get_entity_color' function in: https://github.com/joBr99/nspanel-lovelace-ui/blob/main/apps/nspanel-lovelace-ui/luibackend/pages.py
//  - icon lookup:
//      - codepoint values: https://docs.nspanel.pky.eu/icon-cheatsheet.html
//      - icon mapping: https://github.com/joBr99/nspanel-lovelace-ui/blob/main/apps/nspanel-lovelace-ui/luibackend/icon_mapping.py
//      - mdi icons: https://pictogrammers.com/library/mdi/
//  - color lookup:
//      - https://rgbcolorpicker.com/565
const char_icon_map WEATHER_ICON_MAP {
  {weather_type::sunny, {u8"\uE598", 65504u}}, //mdi:0599,#ffff00
  {weather_type::windy, {u8"\uE59C", 38066u}}, //mdi:059D,#949694
  {weather_type::windy_variant, {u8"\uE59D", 64495u}}, //mdi:059E,#ff7d7b
  {weather_type::cloudy, {u8"\uE58F", 31728u}}, //mdi:0590,#7b7d84
  {weather_type::partlycloudy, {u8"\uE594", 38066u}}, //mdi:0595,#949694
  {weather_type::clear_night, {u8"\uE593", 38060u}}, //mdi:0594,#949663 // weather-night
  {weather_type::exceptional, {icon_t::alert_circle_outline, 63878u}}, //mdi:05D6,#ff3131
  {weather_type::rainy, {u8"\uE596", 25375u}}, //mdi:0597,#6361ff
  {weather_type::pouring, {u8"\uE595", 12703u}}, //mdi:0596,#3131ff
  {weather_type::snowy, {u8"\uE597", 65535u}}, //mdi:E598,#ffffff
  {weather_type::snowy_rainy, {u8"\uEF34", 38079u}}, //mdi:067F,#9496ff
  {weather_type::fog, {u8"\uE590", 38066u}}, //mdi:0591,#949694
  {weather_type::hail, {u8"\uE591", 65535u}}, //mdi:0592,#ffffff
  {weather_type::lightning, {u8"\uE592", 65120u}}, //mdi:0593,#ffce00
  {weather_type::lightning_rainy, {u8"\uE67D", 50400u}} //mdi:067E,#c59e00
};

const char_map ENTITY_RENDER_TYPE_MAP {
  {entity_type::cover, entity_render_type::shutter},
  {entity_type::light, entity_type::light},

  {entity_type::switch_, entity_type::switch_},
  {entity_type::input_boolean, entity_type::switch_},
  {entity_type::automation, entity_type::switch_},

  {entity_type::fan, entity_type::fan},
  
  {entity_type::button, entity_type::button},
  {entity_type::input_button, entity_type::button},
  {entity_type::scene, entity_type::button},
  {entity_type::script, entity_type::button},
  {entity_type::lock, entity_type::button},
  {entity_type::vacuum, entity_type::button},
  {entity_type::navigate, entity_type::button},
  {entity_type::service, entity_type::button},

  {entity_type::number, entity_type::number},
  {entity_type::input_number, entity_type::number},

  {entity_type::input_select, entity_render_type::input_sel},
  {entity_type::select, entity_render_type::input_sel},
  
  {entity_type::itext, entity_render_type::text},
  {entity_type::sensor, entity_render_type::text},
  {entity_type::binary_sensor, entity_render_type::text},
  {entity_type::input_text, entity_render_type::text},
  {entity_type::alarm_control_panel, entity_render_type::text},
  {entity_type::sun, entity_render_type::text},
  {entity_type::person, entity_render_type::text},
  {entity_type::climate, entity_render_type::text},
  {entity_type::weather, entity_render_type::text},

  {entity_type::timer, entity_type::timer},
  {entity_type::media_player, entity_render_type::media_pl},
};

inline const char *get_entity_type(const std::string &entity_id) {
  auto pos = entity_id.find('.');
  if (pos == std::string::npos) {
    if (entity_id == entity_type::delete_)
      return entity_type::delete_;
    return nullptr;
  }
  
	auto type = entity_id.substr(0, pos);

	if (type == entity_type::light) return entity_type::light;
  else if (type == entity_type::switch_) return entity_type::switch_;
  else if (type == entity_type::input_boolean) return entity_type::input_boolean;
  else if (type == entity_type::automation) return entity_type::automation;
  else if (type == entity_type::fan) return entity_type::fan;
  else if (type == entity_type::lock) return entity_type::lock;
  else if (type == entity_type::button) return entity_type::button;
  else if (type == entity_type::input_button) return entity_type::input_button;
  else if (type == entity_type::input_select) return entity_type::input_select;
  else if (type == entity_type::number) return entity_type::number;
  else if (type == entity_type::input_number) return entity_type::input_number;
  else if (type == entity_type::vacuum) return entity_type::vacuum;
  else if (type == entity_type::timer) return entity_type::timer;
  else if (type == entity_type::person) return entity_type::person;
  else if (type == entity_type::service) return entity_type::service;
  else if (type == entity_type::scene) return entity_type::scene;
  else if (type == entity_type::script) return entity_type::script;

  else if (type == entity_type::cover) return entity_type::cover;
  else if (type == entity_type::sensor) return entity_type::sensor;
  else if (type == entity_type::binary_sensor) return entity_type::binary_sensor;
  else if (type == entity_type::input_text) return entity_type::input_text;
  else if (type == entity_type::select) return entity_type::select;
  else if (type == entity_type::alarm_control_panel) return entity_type::alarm_control_panel;
  else if (type == entity_type::media_player) return entity_type::media_player;
  else if (type == entity_type::sun) return entity_type::sun;
  else if (type == entity_type::climate) return entity_type::climate;
  else if (type == entity_type::weather) return entity_type::weather;

  // internal (non HA) types
  else if (type == entity_type::nav_up) return entity_type::nav_up;
  else if (type == entity_type::nav_prev) return entity_type::nav_prev;
  else if (type == entity_type::nav_next) return entity_type::nav_next;
  else if (type == entity_type::uuid) return entity_type::uuid;
  else if (type == entity_type::navigate) {
    if (entity_id.length() > (pos + 5) &&
      entity_id.substr(0, pos + 5) == entity_type::navigate_uuid)
      return entity_type::navigate_uuid;
    return entity_type::navigate;
  }
  else if (type == entity_type::itext) return entity_type::itext;
  
  else return nullptr;
}

} // namespace nspanel_lovelace
} // namespace esphome