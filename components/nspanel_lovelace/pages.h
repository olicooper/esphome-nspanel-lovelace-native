#pragma once

#include "config.h"
#include "page_base.h"
#include "page_items.h"
#include "page_visitor.h"
#include <memory>
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

  void accept(PageVisitor& visitor) override;

  std::shared_ptr<StatusIconItem> left_icon;
  std::shared_ptr<StatusIconItem> right_icon;

  void set_icon_left(std::shared_ptr<StatusIconItem> &left_icon);
  void set_icon_right(std::shared_ptr<StatusIconItem> &right_icon);
  
  const char *get_render_instruction() const override { return "weatherUpdate"; };
  std::string &render(std::string &buffer) override;

  virtual std::string &render_status_update(std::string &buffer);
};

} // namespace nspanel_lovelace
} // namespace esphome