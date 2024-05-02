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

Card::Card(page_type type, const std::string &uuid) :
    Page(type, uuid) {}

Card::Card(page_type type, const std::string &uuid,
    const std::string &title) : Page(type, uuid, title) {}

Card::Card(
    page_type type, const std::string &uuid, 
    const std::string &title, const uint16_t sleep_timeout) :
    Page(type, uuid, title, sleep_timeout) {}

void Card::accept(PageVisitor& visitor) { visitor.visit(*this); }

std::string &Card::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  this->render_nav(buffer);

  for (auto& item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }

  return buffer;
}

std::string &Card::render_nav(std::string &buffer) {
  if (this->nav_left)
    buffer.append(this->nav_left->render()).append(1, SEPARATOR);
  else
    buffer.append(entity_type::delete_).append(6, SEPARATOR);
  if (this->nav_right)
    buffer.append(this->nav_right->render());
  else
    buffer.append(entity_type::delete_).append(5, SEPARATOR);

  return buffer;
}

/*
 * =============== CardItem ===============
 */

CardItem::CardItem(const std::string &uuid, std::shared_ptr<Entity> entity) :
    StatefulPageItem(uuid, std::move(entity)),
    PageItem_DisplayName(this) {}

CardItem::CardItem(const std::string &uuid, std::shared_ptr<Entity> entity,
    const std::string &display_name) :
    StatefulPageItem(uuid, std::move(entity)),
    PageItem_DisplayName(this, display_name) {}

void CardItem::accept(PageItemVisitor& visitor) { visitor.visit(*this); }

std::string &CardItem::render_(std::string &buffer) {
  StatefulPageItem::render_(buffer);
  if (this->entity_->is_type(entity_type::delete_))
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