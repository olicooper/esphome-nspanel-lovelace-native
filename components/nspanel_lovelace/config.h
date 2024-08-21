#pragma once

#include <stdint.h>
#include <string>
#include <memory>

#define NSPANEL_LOVELACE_BUILD_VERSION "0.1.0 (beta)"

namespace esphome {
namespace nspanel_lovelace {

enum class temperature_unit_t : uint8_t { celcius, fahrenheit };
enum class nspanel_model_t : uint8_t { unknown, eu, us_l, us_p };

constexpr char SEPARATOR = '~';
// workaround for https://github.com/sairon/esphome-nspanel-lovelace-ui/issues/8
constexpr uint8_t COMMAND_COOLDOWN = 75u;
constexpr uint16_t DEFAULT_SLEEP_TIMEOUT_S = 20u;
// Change this value when the state object structure changes
constexpr uint32_t RESTORE_STATE_VERSION = 0xA62E0210;

class Configuration {
public:
  Configuration(Configuration const&) = delete;
  void operator=(Configuration const&) = delete;
  static Configuration *instance();

  static void set_temperature_unit(temperature_unit_t unit);
  static temperature_unit_t get_temperature_unit();
  static std::string get_temperature_unit_str();

  static void set_model(nspanel_model_t model);
  static void set_model(const std::string &model_str);
  static nspanel_model_t get_model();
  static std::string get_model_str();
  static uint16_t get_version();
  static void set_version(uint16_t version);

protected:
  Configuration();

  temperature_unit_t temperature_unit_;
  nspanel_model_t model_;
  uint16_t version_;
};

} // nspanel_lovelace
} // esphome