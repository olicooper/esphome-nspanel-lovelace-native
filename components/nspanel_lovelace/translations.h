#pragma once

#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

static constexpr FrozenCharMap<const char *, 26> TRANSLATION_MAP {{
  {"none", "None"},
  {"preset_mode", "Preset mode"},
  {"swing_mode", "Swing mode"},
  {"fan_mode", "Fan mode"},
  {"activity", "Activity"},
  {"away", "Away"},
  {"boost", "Boost"},
  {"comfort", "Comfort"},
  {"eco", "Eco"},
  {"home", "Home"},
  {"sleep", "Sleep"},
  {"cool", "Cool"},
  {"cooling", "Cooling"},
  {"dry", "Dry"},
  {"drying", "Drying"},
  {"fan", "Fan"},
  {"heat", "Heat"},
  {"heating", "Heating"},
  {"heat_cool", "Heat/Cool"},
  {"idle", "Idle"},
  {"auto", "Auto"},
  {"fan_only", "Fan only"},
  {"off", "Off"},
  {"currently", "Currently"},
  {"state", "State"},
  {"action", "Action"},
}};

// inline const std::string &get_translation(const std::string &key) {
//   auto key_cstr = key.c_str();
//   return get_translation(key_cstr);
// }

static inline const char *get_translation(const char *key) {
  auto ret = key;
  try_get_value(TRANSLATION_MAP, ret, key);
  return ret;
}

} // namespace nspanel_lovelace
} // namespace esphome