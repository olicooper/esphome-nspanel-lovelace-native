#pragma once

#include "card_base.h"
#include "entity.h"
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
  // virtual ~GridCard() {}

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
  // virtual ~EntitiesCard() {}

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
  // virtual ~QRCard() {}

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

class AlarmCard : public Card, public IEntitySubscriber {
public:
  AlarmCard(const std::string &uuid,
      const std::shared_ptr<Entity> &alarm_entity);
  AlarmCard(const std::string &uuid,
      const std::shared_ptr<Entity> &alarm_entity,
      const std::string &title);
  AlarmCard(const std::string &uuid,
      const std::shared_ptr<Entity> &alarm_entity,
      const std::string &title, const uint16_t sleep_timeout);
  virtual ~AlarmCard();

  void accept(PageVisitor& visitor) override;

  void set_show_keypad(bool show_keypad) { this->show_keypad_ = show_keypad; }
  bool set_arm_button(
      alarm_arm_action action, const std::string &display_name);
  void set_disarm_button(const std::string &display_name);

  void on_entity_state_change(const std::string &state) override;
  void on_entity_attribute_change(ha_attr_type attr, const std::string &value) override;

  std::string &render(std::string &buffer) override;

protected:
  std::shared_ptr<Entity> alarm_entity_;
  bool show_keypad_, status_icon_flashing_;
  std::unique_ptr<AlarmButtonItem> disarm_button_;
  std::unique_ptr<AlarmIconItem> status_icon_;
  std::unique_ptr<AlarmIconItem> info_icon_;
};

/*
 * =============== ThermoCard ===============
 */

class ThermoCard : public Card, public IEntitySubscriber {
public:
  ThermoCard(const std::string &uuid,
      const std::shared_ptr<Entity> &thermo_entity);
  ThermoCard(const std::string &uuid,
      const std::shared_ptr<Entity> &thermo_entity,
      const std::string &title);
  ThermoCard(
      const std::string &uuid,
      const std::shared_ptr<Entity> &thermo_entity,
      const std::string &title, const uint16_t sleep_timeout);
  virtual ~ThermoCard();

  void accept(PageVisitor& visitor) override;

  void configure_temperature_unit();

  std::string &render(std::string &buffer) override;

protected:
  std::shared_ptr<Entity> thermo_entity_;
  const char* temperature_unit_icon_;
};

} // namespace nspanel_lovelace
} // namespace esphome