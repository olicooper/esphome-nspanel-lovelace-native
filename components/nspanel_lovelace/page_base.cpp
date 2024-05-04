#include "page_base.h"

#include "config.h"
#include "types.h"

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Page ===============
 */

Page::Page() :
    uuid_(""), type_(page_type::unknown),
    render_type_(page_type::unknown), hidden_(false),
    sleep_timeout_(DEFAULT_SLEEP_TIMEOUT_S) {}

Page::Page(page_type type, const std::string &uuid) :
    uuid_(uuid), type_(type), render_type_(type),
    hidden_(false), sleep_timeout_(DEFAULT_SLEEP_TIMEOUT_S) {}

Page::Page(
    page_type type, const std::string &uuid, const std::string &title) :
    uuid_(uuid), type_(type), render_type_(type),
    title_(title), hidden_(false),
    sleep_timeout_(DEFAULT_SLEEP_TIMEOUT_S) {}

Page::Page(
    page_type type, const std::string &uuid, const std::string &title,
    const uint16_t sleep_timeout) :
    uuid_(uuid), type_(type), render_type_(type),
    title_(title), hidden_(false), sleep_timeout_(sleep_timeout) {}

// Copy constructor overridden so the uuid is cleared
Page::Page(const Page &other) :
    uuid_(""), type_(other.type_), render_type_(other.render_type_),
    title_(other.title_), hidden_(other.hidden_),
    sleep_timeout_(other.sleep_timeout_) {}

void Page::accept(PageVisitor& visitor) { visitor.visit(*this); }

bool Page::is_type(page_type type) const {
  return this->type_ == type;
}

const char *Page::get_render_type_str() const {
  return to_string(this->render_type_);
}

void Page::set_render_type(page_type type) {
  this->render_type_ = type;
}

void Page::set_items_render_invalid() { 
  for (auto &i : this->items_) {
    i.get()->set_render_invalid();
  }
}

void Page::set_on_item_added_callback(
    std::function<void(const std::shared_ptr<PageItem>&)> &&callback) {
  this->on_item_added_callback_ = std::move(callback);
}

void Page::add_item(const std::shared_ptr<PageItem> &item) {
  if (item->get_uuid() != entity_type::delete_) {
    for (auto &i : this->items_) {
      if (i->get_uuid() == item->get_uuid())
        return;
    }
  }
  this->items_.push_back(item);
  this->on_item_added_(item);
}

void Page::add_item_range(const std::vector<std::shared_ptr<PageItem>> &items) {
  for (auto& item : items) {
    this->add_item(item);
  }
}

void Page::on_item_added_(const std::shared_ptr<PageItem> &item) {
  this->on_item_added_callback_(item);
}

} // namespace nspanel_lovelace
} // namespace esphome