#include "card_base.h"

#include "config.h"
#include "page_base.h"
#include "page_item_base.h"
#include <cstring>
#include <stdint.h>
#include <string>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Card ===============
 */

Card::Card(const char *type, const std::string &uuid) : Page(type, uuid) {}

Card::Card(const char *type, const std::string &uuid, const std::string &title) : 
    Page(type, uuid, title) {}

Card::Card(
    const char *type, const std::string &uuid, 
    const std::string &title, const uint16_t sleep_timeout) :
    Page(type, uuid, title, sleep_timeout) {}

Card::~Card() {
  for (auto& item : this->items_) {
    if (auto card_item = page_item_cast<CardItem>(item.get())) {
      card_item->remove_card(this);
    }
  }
}

void Card::accept(PageVisitor& visitor) { visitor.visit(*this); }

void Card::on_item_added_(PageItem *item) {
  if (auto card_item = page_item_cast<CardItem>(item)) {
    card_item->add_card(this);
  }
}

std::string &Card::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  if (this->nav_left)
    buffer.append(this->nav_left->render()).append(1, SEPARATOR);
  else
    buffer.append("delete").append(6, SEPARATOR);
  if (this->nav_right)
    buffer.append(this->nav_right->render());
  else
    buffer.append("delete").append(5, SEPARATOR);

  for (auto& item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }

  return buffer;
}

/*
 * =============== CardItem ===============
 */

void CardItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

bool CardItem::has_card(Page *card) const {
  return this->find_card(card) != nullptr;
}

const Card *CardItem::find_card(Page *card) const {
  for (auto& c : cards_) {
    if (c != card)
      continue;
    return c;
  }
  return nullptr;
}

void CardItem::add_card(Card *card) {
  this->cards_.push_back(card);
}

void CardItem::remove_card(Card *card) {
  cards_.erase(std::remove(
    cards_.begin(), cards_.end(), card), 
    cards_.end());
}

void CardItem::set_entity_id(const std::string &entity_id) {
  StatefulPageItem::set_entity_id(entity_id);

  if (this->entity_id_.empty() || !this->display_name_.empty()) return;

  // todo: should be blank instead?
  this->set_display_name(this->entity_id_);
}

std::string &CardItem::render_(std::string &buffer) {
  StatefulPageItem::render_(buffer);
  if (this->entity_id_ == entity_type::delete_)
    buffer.append(1, SEPARATOR);
  else
    // displayName~
    PageItem_DisplayName::render_(buffer).append(1, SEPARATOR);
  return buffer;
}
  
uint16_t CardItem::get_render_buffer_reserve_() const {
  // try to guess the required size of the buffer to reduce heap fragmentation
  return StatefulPageItem::get_render_buffer_reserve_() + 
      this->display_name_.length() + 1;
}

} // namespace nspanel_lovelace
} // namespace esphome