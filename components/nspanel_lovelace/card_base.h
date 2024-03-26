#pragma once

#include "config.h"
#include "page_base.h"
#include "page_item_base.h"
#include "page_items.h"
#include <functional>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

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
  virtual ~Card();

  std::unique_ptr<NavigationItem> nav_left;
  std::unique_ptr<NavigationItem> nav_right;

  void set_nav_left(std::unique_ptr<NavigationItem> &nav) {
    this->nav_left.swap(nav);
  }
  void set_nav_right(std::unique_ptr<NavigationItem> &nav) {
    this->nav_right.swap(nav);
  }

  // when an item is added to the card, also add the card to the item
  void on_item_added_(PageItem *) override;

  const char *get_render_instruction() const override { return "entityUpd"; }
  std::string &render(std::string &buffer) override;
};

/*
 * =============== CardItem ===============
 */

// If a page item belongs to the 'card' page type and can exist outside of a
// specific card, then it should derive from CardItem and not PageItem.
class CardItem : public PageItem {
public:
  CardItem(const std::string &uuid) : PageItem(uuid) {}
  virtual ~CardItem() {}

  const uint32_t class_type() const override { return this->this_class_type_; }
  static const bool is_instance_of(PageItem *item) {
    return (item->class_type() & CardItem::this_class_type_) == CardItem::this_class_type_;
  }

  bool has_card(Page *card) const;
  const Card *find_card(Page *card) const;
  template <typename TCard> const TCard *find_card(TCard *card) const {
    static_assert(
        std::is_base_of<Card, TCard>::value, "TCard must derive from Card");
    auto c = this->find_card(static_cast<Page *>(card));
    return c == nullptr ? c : static_cast<TCard *>(c);
  }

  virtual void add_card(Card *card);
  virtual void remove_card(Card *card);

protected:
  static const uint32_t static_class_type_() { return CardItem::this_class_type_; }

  std::vector<Card *> cards_;

private:
  static const uint32_t this_class_type_;
};

/*
 * =============== StatefulCardItem ===============
 */

class StatefulCardItem :
    public CardItem,
    public PageItem_Type,
    public PageItem_EntityId,
    public PageItem_Icon,
    public PageItem_DisplayName,
    public PageItem_State {
public:
  StatefulCardItem(const std::string &uuid);
  StatefulCardItem(const std::string &uuid, const std::string &display_name);
  virtual ~StatefulCardItem() {}

  const uint32_t class_type() const override { return this->this_class_type_; }
  static const bool is_instance_of(PageItem *item) {
    return (item->class_type() & StatefulCardItem::this_class_type_) == StatefulCardItem::this_class_type_;
  }

  void set_entity_id(const std::string &entity_id) override;
  bool set_type(const char *type) override;
  void set_state(const std::string &state) override;
  void set_attribute(const char *attr, const std::string &value) override;
  void set_device_class(const std::string &device_class);

protected:
  static const uint32_t static_class_type_() { return StatefulCardItem::this_class_type_; }

  static void state_on_off_fn(StatefulCardItem *me);
  static void state_binary_sensor_fn(StatefulCardItem *me);
  static void state_cover_fn(StatefulCardItem *me);
  // static void state_button_fn(StatefulCardItem *me);
  // static void state_scene_fn(StatefulCardItem *me);
  // static void state_script_fn(StatefulCardItem *me);

  // output: type~internalName~icon~iconColor~displayName~
  std::string &render_(std::string &buffer) override;
  uint16_t get_render_buffer_reserve_() const override;

  // A function which modifies the entity when the state changes
  std::function<void(StatefulCardItem *)> on_state_callback_;
  std::string device_class_;

private:
  static const uint32_t this_class_type_;
};

} // namespace nspanel_lovelace
} // namespace esphome