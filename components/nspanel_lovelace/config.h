#pragma once

#define NSPANEL_LOVELACE_BUILD_VERSION "0.1.0 (beta)"

#include <stdint.h>

namespace esphome {
namespace nspanel_lovelace {

constexpr char SEPARATOR = '~';
// workaround for https://github.com/sairon/esphome-nspanel-lovelace-ui/issues/8
constexpr uint8_t COMMAND_COOLDOWN = 75u;
constexpr uint16_t DEFAULT_SLEEP_TIMEOUT_S = 20u;
// Change this value when the state object structure changes
constexpr uint32_t RESTORE_STATE_VERSION = 0xA62E0210;

}
}