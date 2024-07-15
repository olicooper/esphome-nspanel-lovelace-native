#pragma once

#include "page_item_base.h"
#include "page_visitor.h"
#include <algorithm>
#include <functional>
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
  Page(page_type type, const std::string &uuid);
  Page(page_type type, const std::string &uuid, const std::string &title);
  Page(
      page_type type, const std::string &uuid, const std::string &title,
      const uint16_t sleep_timeout);
  Page(const Page &other);
  virtual ~Page() {}

  virtual void accept(PageVisitor& visitor);

  const std::string &get_uuid() const { return this->uuid_; }
  const std::string &get_title() const { return this->title_; }
  bool is_type(page_type type) const;
  const char *get_render_type_str() const;
  void set_render_type(page_type type);
  bool is_hidden() const { return this->hidden_; }
  uint16_t get_sleep_timeout() const { return this->sleep_timeout_; }

  virtual void set_uuid(const std::string &uuid) { this->uuid_ = uuid; }
  virtual void set_title(const std::string &title) { this->title_ = title; }
  virtual void set_hidden(const bool hidden) { this->hidden_ = hidden; }
  virtual void set_sleep_timeout(const uint16_t timeout) {
    this->sleep_timeout_ = timeout;
  }
  
  virtual void set_items_render_invalid();

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

  void set_on_item_added_callback(
      std::function<void(const std::shared_ptr<PageItem>&)> &&callback);

protected:
  Page();

  virtual const char *get_render_instruction() const = 0;
  virtual void on_item_added_(const std::shared_ptr<PageItem> &item);

  std::string uuid_;
  page_type type_;
  page_type render_type_;
  std::string title_;
  bool hidden_;
  uint16_t sleep_timeout_;

  std::vector<std::shared_ptr<PageItem>> items_;
  std::function<void(const std::shared_ptr<PageItem>&)> on_item_added_callback_;
};

} // namespace nspanel_lovelace
} // namespace esphome