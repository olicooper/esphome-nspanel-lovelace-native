#pragma once

#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

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
  static constexpr const char* sleep = "sleep";
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
  static constexpr const char* pause = ha_action_type::pause;
  static constexpr const char* cancel = ha_action_type::cancel;
  static constexpr const char* finish = ha_action_type::finish;
  // alarm_control_panel
  static constexpr const char* disarm = "disarm";
  // cover
  static constexpr const char* tilt_position = "tilt_pos";
  // sun (backend.component.sun.state)
  static constexpr const char* above_horizon = entity_state::above_horizon;
  static constexpr const char* below_horizon = entity_state::below_horizon;
  // person (backend.component.person.state)
  static constexpr const char* not_home = entity_state::not_home;
  // vacuum (frontend.ui.card.vacuum.actions)
  static constexpr const char* start_cleaning = "start_cleaning";
  static constexpr const char* resume_cleaning = "resume_cleaning";
  static constexpr const char* return_to_base = ha_action_type::return_to_base;
  static constexpr const char* docked = "docked";
  
  static constexpr const char* turn_on = ha_action_type::turn_on;
  static constexpr const char* turn_off = ha_action_type::turn_off;
};

static constexpr FrozenCharMap<const char *, 55> TRANSLATION_MAP {{
  {translation_item::none, "None"},
  {translation_item::unknown, "Unknown"},
  {translation_item::preset_mode, "Preset mode"},
  {translation_item::swing_mode, "Swing mode"},
  {translation_item::fan_mode, "Fan mode"},
  {translation_item::activity, "Activity"},
  {translation_item::away, "Away"},
  {translation_item::boost, "Boost"},
  {translation_item::comfort, "Comfort"},
  {translation_item::eco, "Eco"},
  {translation_item::home, "Home"},
  {translation_item::sleep, "Sleep"},
  {translation_item::cool, "Cool"},
  {translation_item::cooling, "Cooling"},
  {translation_item::dry, "Dry"},
  {translation_item::drying, "Drying"},
  {translation_item::fan, "Fan"},
  {translation_item::heat, "Heat"},
  {translation_item::heating, "Heating"},
  {translation_item::heat_cool, "Heat/Cool"},
  {translation_item::idle, "Idle"},
  {translation_item::auto_, "Auto"},
  {translation_item::fan_only, "Fan only"},
  {translation_item::off, "Off"},
  {translation_item::currently, "Currently"},
  {translation_item::state, "State"},
  {translation_item::action, "Action"},
  {translation_item::lock, "Lock"},
  {translation_item::unlock, "Unlock"},
  {translation_item::idle, "Idle"},
  {translation_item::paused, "Paused"},
  {translation_item::active, "Active"},
  {translation_item::activate, "Activate"},
  {translation_item::press, "Press"},
  {translation_item::run, "Run"},
  {translation_item::speed, "Speed"},
  {translation_item::brightness, "Brightness"},
  {translation_item::color, "Colour"},
  {translation_item::color_temp, "Colour temperature"},
  {translation_item::above_horizon, "Above Horizon"},
  {translation_item::below_horizon, "Below Horizon"},
  {translation_item::position, "Position"},
  {translation_item::tilt_position, "Tilt position"},
  {translation_item::start, "Start"},
  {translation_item::pause, "Pause"},
  {translation_item::cancel, "Cancel"},
  {translation_item::finish, "Finish"},
  {translation_item::disarm, "Disarm"},
  {translation_item::not_home, "Away"},
  {translation_item::start_cleaning, "Start cleaning"},
  {translation_item::return_to_base, "Return to dock"},
  {translation_item::resume_cleaning, "Resume dock"},
  {translation_item::docked, "Docked"},
  {translation_item::turn_on, "Turn on"},
  {translation_item::turn_off, "Turn off"},
}};

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