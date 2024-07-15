#pragma once

#include <map>
#include <string>
#include "helpers.h"

namespace esphome {
namespace nspanel_lovelace {

const std::map<const char*, std::string, compare_char_str> TRANSLATION_MAP {
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
};

// inline const std::string &get_translation(const std::string &key) {
//   auto key_cstr = key.c_str();
//   return get_translation(key_cstr);
// }

inline std::string get_translation(const char *key) {
  if (key == nullptr) return "";
  auto it = TRANSLATION_MAP.find(key);
  if (it != TRANSLATION_MAP.end()) {
    return it->second;
  }
  return std::string(key);
}

} // namespace nspanel_lovelace
} // namespace esphome