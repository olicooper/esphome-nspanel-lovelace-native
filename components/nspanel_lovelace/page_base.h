#pragma once

#include "config.h"
#include "page_item_base.h"
#include "page_visitor.h"
#include <algorithm>
#include <memory>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Page ===============
 */

class Page {
public:
  Page(const char *type, const std::string &uuid);
  Page(const char *type, const std::string &uuid, const std::string &title);
  Page(
      const char *type, const std::string &uuid, const std::string &title,
      const uint16_t sleep_timeout);
  Page(const Page &other);
  virtual ~Page();

  virtual void accept(PageVisitor& visitor);

  const std::string &get_uuid() const { return this->uuid_; }
  const std::string &get_title() const { return this->title_; }
  bool is_type(const char *type) const;
  const char *get_type() const { return this->type_; }
  bool is_hidden() const { return this->hidden_; }
  uint16_t get_sleep_timeout() const { return this->sleep_timeout_; }

  virtual void set_uuid(const std::string &uuid) { this->uuid_ = uuid; }
  virtual void set_title(const std::string &title) { this->title_ = title; }
  virtual void set_hidden(const bool hidden) { this->hidden_ = hidden; }
  virtual void set_sleep_timeout(const uint16_t timeout) {
    this->sleep_timeout_ = timeout;
  }
  
  virtual void set_items_render_invalid();

  virtual const char *get_render_instruction() const = 0;
  virtual std::string &render(std::string &buffer) = 0;

  void add_item(const std::shared_ptr<PageItem> &item);
  void add_item_range(const std::vector<std::shared_ptr<PageItem>> &items);
  const std::vector<std::shared_ptr<PageItem>> &get_items() {
    return this->items_;
  }

  template<class TPageItem = PageItem>
  TPageItem* get_item(const size_t index) {
     static_assert(
        std::is_base_of<PageItem, TPageItem>::value,
        "TPageItem must derive from esphome::nspanel_lovelace::PageItem");
    if ((this->items_.size() - 1) < index) 
      return nullptr;
    return page_item_cast<TPageItem>(this->items_.at(index).get());
  }

protected:
  Page();

  virtual void on_item_added_(PageItem *);

  std::string uuid_;
  const char *type_;
  std::string title_;
  bool hidden_;
  uint16_t sleep_timeout_;

  std::vector<std::shared_ptr<PageItem>> items_;
};

} // namespace nspanel_lovelace
} // namespace esphome