#pragma once

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
  virtual ~Screensaver() {}

  void accept(PageVisitor& visitor) override;

  void set_icon_left(std::shared_ptr<StatusIconItem> left_icon);
  void set_icon_right(std::shared_ptr<StatusIconItem> right_icon);
  bool should_render_status_update(const std::string &entity_id = "") {
    if (this->left_icon && (entity_id.empty() ||
        this->left_icon->get_entity_id() == entity_id)) {
      return true;
    }
    if (this->right_icon && (entity_id.empty() ||
        this->right_icon->get_entity_id() == entity_id)) {
      return true;
    }
    return false;
  }
  
  const char *get_render_instruction() const override { return "weatherUpdate"; };
  std::string &render(std::string &buffer) override;

  virtual std::string &render_status_update(std::string &buffer);

protected:
  std::shared_ptr<StatusIconItem> left_icon;
  std::shared_ptr<StatusIconItem> right_icon;
};

} // namespace nspanel_lovelace
} // namespace esphome