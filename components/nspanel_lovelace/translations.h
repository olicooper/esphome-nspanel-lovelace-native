// NOTE: Due to memory constraints a few design decisions have been made to preserve resources...
//  - Keep the translation keys short with no nesting (besides using the 'key.subkey' format in select places).
//  - Use constexpr to ensure strings are stored in flash and re-use matching keys from other places where possible.
//  - No unnecessary translations.
// **Translations (keys and values) may change in the future!** This is an imperfect solution which may be inadequate
// for the task, but reducing memory consumption is also important.

#pragma once

#include "esphome/core/defines.h"
#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

// NOTE: If keys are added to this list, the REQUIRED_TRANSLATION_KEYS
//       list in __init__.py will need updating
struct translation_item {
  static constexpr const char* none = "none";
  static constexpr const char* unknown = entity_state::unknown;
  static constexpr const char* preset_mode = "preset_mode";
  static constexpr const char* swing_mode = "swing_mode";
  static constexpr const char* fan_mode = "fan_mode";
  static constexpr const char* activity = "activity";
  static constexpr const char* away = "away";
  static constexpr const char* boost = "boost";
  static constexpr const char* comfort = "comfort";
  static constexpr const char* eco = "eco";
  static constexpr const char* home = entity_state::home;
  static constexpr const char* sleep_ = "sleep";
  static constexpr const char* cool = entity_state::cool;
  static constexpr const char* cooling = "cooling";
  static constexpr const char* dry = entity_state::dry;
  static constexpr const char* drying = "drying";
  static constexpr const char* fan = "fan";
  static constexpr const char* heat = entity_state::heat;
  static constexpr const char* heating = "heating";
  static constexpr const char* heat_cool = entity_state::heat_cool;
  static constexpr const char* idle = entity_state::idle;
  static constexpr const char* auto_ = entity_state::auto_;
  static constexpr const char* fan_only = entity_state::fan_only;
  static constexpr const char* on = entity_state::on;
  static constexpr const char* off = entity_state::off;
  static constexpr const char* currently = "currently";
  static constexpr const char* state = "state";
  static constexpr const char* action = "action";
  static constexpr const char* lock = ha_action_type::lock;
  static constexpr const char* unlock = ha_action_type::unlock;
  static constexpr const char* paused = entity_state::paused;
  static constexpr const char* active = "active";
  static constexpr const char* activate = "activate";
  static constexpr const char* press = ha_action_type::press;
  static constexpr const char* run = "run";
  static constexpr const char* speed = "speed";
  static constexpr const char* brightness = "brightness";
  static constexpr const char* color = "color";
  static constexpr const char* color_temp = "color_temp";
  static constexpr const char* position = "position";
  // timer (frontend.ui.card.timer.actions)
  static constexpr const char* start = ha_action_type::start;
  static constexpr const char* pause_ = ha_action_type::pause;
  static constexpr const char* cancel = ha_action_type::cancel;
  static constexpr const char* finish = ha_action_type::finish;
  // alarm_control_panel
  static constexpr const char* arm_home = button_type::armHome;
  static constexpr const char* arm_away = button_type::armAway;
  static constexpr const char* arm_night = button_type::armNight;
  static constexpr const char* arm_vacation = button_type::armVacation;
  static constexpr const char* arm_custom_bypass = button_type::armCustomBypass;
  static constexpr const char* disarm = button_type::disarm;
  static constexpr const char* disarmed = entity_state::disarmed;
  static constexpr const char* arming = entity_state::arming;
  static constexpr const char* pending = entity_state::pending;
  static constexpr const char* triggered = entity_state::triggered;
  static constexpr const char* armed_home = entity_state::armed_home;
  static constexpr const char* armed_away = entity_state::armed_away;
  static constexpr const char* armed_night = entity_state::armed_night;
  static constexpr const char* armed_vacation = entity_state::armed_vacation;
  static constexpr const char* armed_custom_bypass = entity_state::armed_custom_bypass;
  // cover
  static constexpr const char* tilt_position = "tilt_pos";
  // sun (backend.component.sun.state)
  static constexpr const char* above_horizon = entity_state::above_horizon;
  static constexpr const char* below_horizon = entity_state::below_horizon;
  // person (backend.component.person.state)
  static constexpr const char* not_home = entity_state::not_home;
  // vacuum (frontend.ui.card.vacuum.actions)
  static constexpr const char* start_cleaning = "start_cleaning";
  static constexpr const char* return_to_base = ha_action_type::return_to_base;
  static constexpr const char* docked = "docked";
  
  static constexpr const char* turn_on = ha_action_type::turn_on;
  static constexpr const char* turn_off = ha_action_type::turn_off;
};

// NOTE: This map is dynamically generated by the esphome build script from a
//       json file based on the users selected language (default 'en')
extern FrozenCharMap<const char *, TRANSLATION_MAP_SIZE> TRANSLATION_MAP;

static inline const char *get_translation(const char *key) {
  auto ret = key;
  try_get_value(TRANSLATION_MAP, ret, key);
  return ret;
}

static inline const char *get_translation(const std::string &key) {
  if (key.empty()) return key.c_str();
  return get_translation(key.c_str());
}

} // namespace nspanel_lovelace
} // namespace esphome