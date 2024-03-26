#pragma once

#include "types.h"
#include "card_base.h"
#include <stdint.h>
#include <string>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== GridCard ===============
 */

class GridCard : public Card {
public:
  GridCard(const std::string &uuid) :
      Card(page_type::cardGrid, uuid) {}
  GridCard(const std::string &uuid, const std::string &title) :
      Card(page_type::cardGrid, uuid, title) {}
  GridCard(
      const std::string &uuid, const std::string &title, 
      const uint16_t sleep_timeout) :
      Card(page_type::cardGrid, uuid, title, sleep_timeout) {}
};

/*
 * =============== EntitiesCard ===============
 */

class EntitiesCard : public Card {
public:
  EntitiesCard(const std::string &uuid) :
      Card(page_type::cardEntities, uuid) {}
  EntitiesCard(const std::string &uuid, const std::string &title) :
      Card(page_type::cardEntities, uuid, title) {}
  EntitiesCard(const std::string &uuid, const std::string &title, const uint16_t sleep_timeout) :
      Card(page_type::cardEntities, uuid, title, sleep_timeout) {}
};

/*
 * =============== QRCard ===============
 */

class QRCard : public Card {
public:
  QRCard(const std::string &uuid) :
      Card(page_type::cardQR, uuid) {}
  QRCard(const std::string &uuid, const std::string &title) :
      Card(page_type::cardQR, uuid, title) {}
  QRCard(
      const std::string &uuid, const std::string &title, 
      const uint16_t sleep_timeout) :
      Card(page_type::cardQR, uuid, title, sleep_timeout) {}

  const std::string &get_qr_text() const { return this->qr_text_; }
  void set_qr_text(const std::string &qr_text) { this->qr_text_ = qr_text; }

  std::string &render(std::string &buffer) override;

protected:
  std::string qr_text_;
};

} // namespace nspanel_lovelace
} // namespace esphome