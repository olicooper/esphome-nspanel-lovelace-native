#include "pages.h"

#include "config.h"
#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Screensaver ===============
 */

// output: weatherUpd~5x[type~internalName~icon~iconColor~displayName~value]
std::string &Screensaver::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction());
      // .append(1, SEPARATOR)
      // .append(this->get_title())
      // .append(1, SEPARATOR)
      // .append(this->left_icon->render())
      // .append(1, SEPARATOR)
      // .append(this->right_icon->render())
      // .append(1, SEPARATOR);

  for (auto &item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }
  
  return buffer;
}

} // namespace nspanel_lovelace
} // namespace esphome