#include "cards.h"

#include "config.h"
#include "helpers.h"
#include "types.h"
#include "card_base.h"
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCard ===============
 */

void GridCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== EntitiesCard ===============
 */

void EntitiesCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

/*
 * =============== QRCard ===============
 */

void QRCard::accept(PageVisitor& visitor) { visitor.visit(*this); }

std::string &QRCard::render(std::string &buffer) {
  buffer.assign(this->get_render_instruction())
      .append(1, SEPARATOR)
      .append(this->get_title())
      .append(1, SEPARATOR);
  
  if (this->nav_left)
    buffer.append(this->nav_left->render()).append(1, SEPARATOR);
  else
    buffer.append("delete").append(6, SEPARATOR);
  if (this->nav_right)
    buffer.append(this->nav_right->render()).append(1, SEPARATOR);
  else
    buffer.append("delete").append(6, SEPARATOR);

  buffer.append(this->qr_text_);

  for (auto& item : this->items_) {
    buffer.append(1, SEPARATOR).append(item->render());
  }

  return buffer;
}

} // namespace nspanel_lovelace
} // namespace esphome