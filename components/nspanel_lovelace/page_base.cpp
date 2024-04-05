#include "page_base.h"

#include "config.h"
#include "types.h"
#include <stdint.h>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Page ===============
 */

Page::Page() :
    uuid_(""), type_(nullptr), hidden_(false),
    sleep_timeout_(DEFAULT_SLEEP_TIMEOUT_S) {}

Page::Page(const char *type, const std::string &uuid) :
    uuid_(uuid), type_(type), hidden_(false),
    sleep_timeout_(DEFAULT_SLEEP_TIMEOUT_S) {}

Page::Page(
    const char *type, const std::string &uuid, const std::string &title) :
    uuid_(uuid),
    type_(type), title_(title), hidden_(false),
    sleep_timeout_(DEFAULT_SLEEP_TIMEOUT_S) {}

Page::Page(
    const char *type, const std::string &uuid, const std::string &title,
    const uint16_t sleep_timeout) :
    uuid_(uuid),
    type_(type), title_(title), hidden_(false), sleep_timeout_(sleep_timeout) {}

// Copy constructor overridden so the uuid is cleared
Page::Page(const Page &other) :
    uuid_(""), type_(other.type_), title_(other.title_), hidden_(other.hidden_),
    sleep_timeout_(other.sleep_timeout_) {}

void Page::accept(PageVisitor& visitor) { visitor.visit(*this); }

bool Page::is_type(const char *type) const {
  return this->type_ == type || std::strcmp(this->type_, type) == 0;
}

void Page::set_items_render_invalid() { 
  for (auto &i : this->items_) {
    i.get()->set_render_invalid();
  }
}

void Page::add_item(const std::shared_ptr<PageItem> &item) {
  for (auto &i : this->items_) {
    if (i->get_uuid() == item->get_uuid())
      return;
  }
  this->items_.push_back(item);
  this->on_item_added_(item.get());
}

void Page::add_item_range(const std::vector<std::shared_ptr<PageItem>> &items) {
  for (auto& item : items) {
    this->add_item(item);
  }
}

} // namespace nspanel_lovelace
} // namespace esphome