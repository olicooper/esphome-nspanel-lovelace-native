#pragma once

#include "card_base.h"
#include "page_visitor.h"
#include "types.h"
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

  void accept(PageVisitor& visitor) override;
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

  void accept(PageVisitor& visitor) override;
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

  void accept(PageVisitor& visitor) override;

  const std::string &get_qr_text() const { return this->qr_text_; }
  void set_qr_text(const std::string &qr_text) { this->qr_text_ = qr_text; }

  std::string &render(std::string &buffer) override;

protected:
  std::string qr_text_;
};

/*
 * =============== AlarmCard ===============
 */

class AlarmCard : public Card {
public:
  AlarmCard(const std::string &uuid, const std::string &alarm_entity_id);
  AlarmCard(const std::string &uuid, const std::string &alarm_entity_id,
      const std::string &title);
  AlarmCard(const std::string &uuid, const std::string &alarm_entity_id,
      const std::string &title, const uint16_t sleep_timeout);

  void accept(PageVisitor& visitor) override;

  const std::string &get_state() const { return this->state_; }
  const std::string &get_alarm_entity_id() const { return this->alarm_entity_id_; }

  void set_state(const std::string &state);
  void set_show_keypad(bool show_keypad) { this->show_keypad_ = show_keypad; }
  bool set_arm_button(
      alarm_arm_action action, const std::string &display_name);
  void set_disarm_button(const std::string &display_name);

  std::string &render(std::string &buffer) override;

protected:
  std::string state_;
  bool show_keypad_, status_icon_flashing_;
  std::string alarm_entity_id_;
  std::unique_ptr<AlarmButtonItem> disarm_button_;
  std::unique_ptr<AlarmIconItem> status_icon_;
  std::unique_ptr<AlarmIconItem> info_icon_;
};

} // namespace nspanel_lovelace
} // namespace esphome