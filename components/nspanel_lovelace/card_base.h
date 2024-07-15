#pragma once

#include "page_base.h"
#include "page_item_base.h"
#include "page_items.h"
#include "page_item_visitor.h"
#include "page_visitor.h"
#include <memory>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Card ===============
 */

// todo not added: key,nav1Override,nav2Override,last_update,cooldown,id
class Card : public Page {

public:
  Card(page_type type, const std::string &uuid);
  Card(page_type type, const std::string &uuid, const std::string &title);
  Card(page_type type, const std::string &uuid,
      const std::string &title, const uint16_t sleep_timeout);
  virtual ~Card() {}

  void accept(PageVisitor& visitor) override;

  void set_nav_left(std::unique_ptr<NavigationItem> &nav) {
    this->nav_left.swap(nav);
  }
  void set_nav_right(std::unique_ptr<NavigationItem> &nav) {
    this->nav_right.swap(nav);
  }

  std::string &render(std::string &buffer) override;

protected:
  std::unique_ptr<NavigationItem> nav_left;
  std::unique_ptr<NavigationItem> nav_right;

  const char *get_render_instruction() const override { return "entityUpd"; }
  std::string &render_nav(std::string &buffer);
};

/*
 * =============== CardItem ===============
 */

class CardItem : 
    public StatefulPageItem,
    public PageItem_DisplayName {
public:
  CardItem(const std::string &uuid, std::shared_ptr<Entity> entity);
  CardItem(const std::string &uuid, std::shared_ptr<Entity> entity,
      const std::string &display_name);
  virtual ~CardItem() {}

  void accept(PageItemVisitor& visitor) override;

protected:
  // output: type~internalName~icon~iconColor~displayName~
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;
};

} // namespace nspanel_lovelace
} // namespace esphome