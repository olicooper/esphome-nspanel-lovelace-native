#include "card_base.h"

#include "page_base.h"
#include "page_item_base.h"
#include "config.h"
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
    if (CardItem::is_instance_of(item.get())) {
      static_cast<CardItem*>(item.get())->remove_card(this);
    }
  }
}

void Card::on_item_added_(PageItem *item) {
  // cannot use dynamic_cast (not available) error: 'dynamic_cast' not permitted with -fno-rtti
  if (CardItem::is_instance_of(item)) {
    static_cast<CardItem*>(item)->add_card(this);
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

const uint32_t CardItem::this_class_type_ = 
    // PageItem | CardItem
    (1<<0) | (1<<1);

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

/*
 * =============== StatefulCardItem ===============
 */

const uint32_t StatefulCardItem::this_class_type_ = 
    // PageItem | CardItem | StatefulCardItem
    (1<<0) | (1<<1) | (1<<2);

StatefulCardItem::StatefulCardItem(
    const std::string &uuid) :
    CardItem(uuid), PageItem_Type(this), PageItem_EntityId(this), PageItem_Icon(this),
    PageItem_DisplayName(this), PageItem_State(this, "unknown") {}

StatefulCardItem::StatefulCardItem(
    const std::string &uuid, const std::string &display_name) :
    CardItem(uuid), PageItem_Type(this), PageItem_EntityId(this), PageItem_Icon(this),
    PageItem_DisplayName(this, display_name), PageItem_State(this, "unknown") {}

void StatefulCardItem::set_entity_id(const std::string &entity_id) {
  PageItem_EntityId::set_entity_id(entity_id);

  if (!this->entity_id_.empty() && 
      this->set_type(get_entity_type(this->entity_id_))) {

    if (this->display_name_.empty())
      // todo: should be blank instead?
      this->set_display_name(this->entity_id_);

    // extract the text from iText entities
    if (this->is_type(entity_type::itext)) {
      auto pos = this->entity_id_.rfind('.', strlen(entity_type::itext) + 1);
      if (pos != std::string::npos && pos < this->entity_id_.length()) {
        this->set_state(this->entity_id_.substr(pos + 1));
      }
    }
  }
}

bool StatefulCardItem::set_type(const char *type) {
  if (type == nullptr || type[0] == '\0') {
    return false;
  }
  this->type_ = type;

  if (type == entity_type::delete_) {
    this->render_type_ = nullptr;
    return true;
  }

  auto it = ENTITY_RENDER_TYPE_MAP.find(this->type_);
  if (it != ENTITY_RENDER_TYPE_MAP.end()) {
    this->render_type_ = it->second;
  } else {
    this->render_type_ = entity_render_type::text;
  }

  if (this->type_ != entity_type::sensor) {
    auto icon_value = get_icon_by_name(ENTITY_ICON_MAP, this->type_);
    if (icon_value != nullptr) {
      this->icon_value_ = this->icon_default_value_ = icon_value;
    }
  }
  this->icon_value_overridden_ = false;

  if (this->type_ == entity_type::light ||
      this->type_ == entity_type::switch_ ||
      this->type_ == entity_type::input_boolean ||
      this->type_ == entity_type::automation ||
      this->type_ == entity_type::fan) {
    this->on_state_callback_ = StatefulCardItem::state_on_off_fn;
  } else if (this->type_ == entity_type::binary_sensor) {
    this->on_state_callback_ = StatefulCardItem::state_binary_sensor_fn;
  } else if (this->type_ == entity_type::cover) {
    this->on_state_callback_ = StatefulCardItem::state_cover_fn;
  } /* else if (
      this->type_ == entity_type::button ||
      this->type_ == entity_type::input_button) {
    this->on_state_callback_ = StatefulCardItem::state_button_fn;
  } else if (this->type_ == entity_type::scene) {
    this->on_state_callback_ = StatefulCardItem::state_scene_fn;
  } else if (this->type_ == entity_type::script) {
    this->on_state_callback_ = StatefulCardItem::state_script_fn;
  }*/

  this->set_render_invalid();

  // also need to make sure the state is updated based on the new 'type'
  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
  return true;
}

void StatefulCardItem::set_state(const std::string &state) {
  this->state_ = state;

  this->set_render_invalid();

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

void StatefulCardItem::set_attribute(const char *attr, const std::string &value) {
  if (value.empty() || value == "None" || value == "none") {
    attributes_.erase(attr);
    return;
  }
  if (attr == ha_attr_type::brightness) {
    attributes_[attr] = std::to_string(scale_value(std::stoi(value), {0, 255}, {0, 100}));
  } else if (attr == ha_attr_type::color_temp) {
    auto minstr = this->get_attribute(ha_attr_type::min_mireds);
    auto maxstr = this->get_attribute(ha_attr_type::max_mireds);
    uint16_t min_mireds = minstr.empty() ? 153 : std::stoi(minstr);
    uint16_t max_mireds = maxstr.empty() ? 500 : std::stoi(maxstr);
    attributes_[attr] = std::to_string(scale_value(
        std::stoi(value),
        {static_cast<double>(min_mireds), static_cast<double>(max_mireds)},
        {0, 100}));
  } else {
    attributes_[attr] = value;
  }
}

void StatefulCardItem::set_device_class(const std::string &device_class) {
  this->device_class_ = device_class;

  this->set_render_invalid();

  if (!this->icon_value_overridden_) {
    if (this->type_ == entity_type::sensor) {
      this->icon_default_value_ = u8"\uE5D5"; // default: alert-circle-outline
      auto icon = get_icon_by_name(SENSOR_ICON_MAP, this->device_class_);
      this->icon_value_ = (icon == nullptr ? this->icon_default_value_ : icon);
    }
  }

  if (this->on_state_callback_) {
    this->on_state_callback_(this);
  }
}

std::string &StatefulCardItem::render_(std::string &buffer) {
  buffer.clear();
  // type~
  PageItem_Type::render_(buffer).append(1, SEPARATOR);
  if (this->entity_id_ == entity_type::delete_)
    // internalName(delete)~
    buffer.append(this->entity_id_).append(1, SEPARATOR);
  else
    // internalName(uuid)~
    PageItem::render_(buffer).append(1, SEPARATOR);
  // iconValue~iconColor~
  PageItem_Icon::render_(buffer).append(1, SEPARATOR);
  // displayName~
  return PageItem_DisplayName::render_(buffer).append(1, SEPARATOR);
}
  
uint16_t StatefulCardItem::get_render_buffer_reserve_() const {
  // try to guess the required size of the buffer to reduce heap fragmentation
  return (this->type_ == nullptr ? 0 : strlen(this->type_)) + 
      this->uuid_.length() + 6 +
      this->get_icon_color_str().length() + 
      this->display_name_.length() +
      // icon is 4 char long + separator chars
      10;
}

void StatefulCardItem::state_on_off_fn(StatefulCardItem *me) {
  if (me->icon_color_overridden_) {
    return;
  }

  if (me->state_ == "on") {
    me->icon_color_ = 64909u; // yellow
  } else if (me->state_ == "off") {
    me->icon_color_ = 17299u; // blue
  } else {
    me->icon_color_ = 38066u; // grey
  }
}

void StatefulCardItem::state_binary_sensor_fn(StatefulCardItem *me) {
  const char *icon = nullptr;
  if (me->state_ == "on") {
    if (!me->icon_color_overridden_)
      me->icon_color_ = 64909u; // yellow
    if (!me->icon_value_overridden_) {
      icon = get_icon_by_name(SENSOR_ON_ICON_MAP, me->device_class_);
      me->icon_value_ = icon == nullptr ? u8"\uE132" : icon; // default: checkbox-marked-circle
    }
  } else {
    if (!me->icon_color_overridden_) {
      if (me->state_ == "off")
        me->icon_color_ = 17299u; // blue
      else
        me->icon_color_ = 38066u; // grey
    }
    if (!me->icon_value_overridden_) {
      icon = get_icon_by_name(SENSOR_OFF_ICON_MAP, me->device_class_);
      me->icon_value_ = icon == nullptr ? u8"\uE43C" : icon; // default: radiobox-blank
    }
  }
}

void StatefulCardItem::state_cover_fn(StatefulCardItem *me) {
  if (!me->icon_color_overridden_) {
    if (me->state_ == "closed")
      me->icon_color_ = 17299u; // blue
    else if (me->state_ == "open")
      me->icon_color_ = 64909u; // yellow
    else 
      me->icon_color_ = 38066u; // grey
  }
  
  if (!me->icon_value_overridden_) {
    auto icons = get_icon_by_name(COVER_MAP, me->device_class_);
    if (icons != nullptr) {
      if (me->state_ == "closed")
        me->icon_value_ = icons->at(1);
      else
        me->icon_value_ = icons->at(0);
    }
  }
}

// clang-format off
// void StatefulCardItem::state_button_fn(StatefulCardItem *me) {}
// void StatefulCardItem::state_scene_fn(StatefulCardItem *me) {}
// void StatefulCardItem::state_script_fn(StatefulCardItem *me) {}
// clang-format on

} // namespace nspanel_lovelace
} // namespace esphome