#pragma once

#include "config.h"
#include "page_base.h"
#include "page_items.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Screensaver ===============
 */

class Screensaver : public Page {
public:
  Screensaver(const std::string &uuid) : Page(page_type::screensaver, uuid) {}

  std::unique_ptr<IconItem> left_icon;
  std::unique_ptr<IconItem> right_icon;

  void set_icon_left(std::unique_ptr<IconItem> &left_icon) {
    this->left_icon.swap(left_icon);
  }
  void set_icon_right(std::unique_ptr<IconItem> &right_icon) {
    this->right_icon.swap(right_icon);
  }
  
  const char *get_render_instruction() const override { return "weatherUpdate"; };
  std::string &render(std::string &buffer) override;
};

} // namespace nspanel_lovelace
} // namespace esphome