#pragma once

#include "config.h"
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
  Card(const char *type, const std::string &uuid);
  Card(const char *type, const std::string &uuid, const std::string &title);
  Card(const char *type, const std::string &uuid, const std::string &title, const uint16_t sleep_timeout);
  virtual ~Card() {}

  void accept(PageVisitor& visitor) override;

  std::unique_ptr<NavigationItem> nav_left;
  std::unique_ptr<NavigationItem> nav_right;

  void set_nav_left(std::unique_ptr<NavigationItem> &nav) {
    this->nav_left.swap(nav);
  }
  void set_nav_right(std::unique_ptr<NavigationItem> &nav) {
    this->nav_right.swap(nav);
  }

  const char *get_render_instruction() const override { return "entityUpd"; }
  std::string &render(std::string &buffer) override;
};

/*
 * =============== CardItem ===============
 */

class CardItem : 
    public StatefulPageItem,
    public PageItem_DisplayName {
public:
  CardItem(const std::string &uuid) :
    StatefulPageItem(uuid), PageItem_DisplayName(this) {}
  CardItem(const std::string &uuid, const std::string &display_name) :
    StatefulPageItem(uuid), PageItem_DisplayName(this, display_name) {}
  virtual ~CardItem() {}

  void accept(PageItemVisitor& visitor) override;

  void set_entity_id(const std::string &entity_id) override;

protected:
  // output: type~internalName~icon~iconColor~displayName~
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;
};

} // namespace nspanel_lovelace
} // namespace esphome