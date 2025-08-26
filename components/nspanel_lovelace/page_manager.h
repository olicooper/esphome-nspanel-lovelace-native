#pragma once

#include "page_base.h"
#include <cassert>
#include <cinttypes>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace esphome {
namespace nspanel_lovelace {

namespace cycle_mode
{
  enum Flag : uint8_t {
    none = 1 << 0,
    forward = 1 << 1,
    backward = 1 << 2
  };
}

class PageManager {
public:
  PageManager() : current_index_(0) {
    pages_.reserve(20);
  }

  bool pages_empty() const;
  size_t page_count() const;
  size_t current_index() const;

  std::vector<std::unique_ptr<Page>>::const_iterator begin() const;
  std::vector<std::unique_ptr<Page>>::const_iterator end() const;
  
  Page* current_page();
  // Find the first non-hidden page adjacent to the current page.
  // cycle_mode::none will choose the current page if possible.
  // Combine cycle_mode::none with cycle_mode::backward to cycle backward instead of forward.
  Page* cycle_page(cycle_mode::Flag mode);
  Page* next_page();
  Page* previous_page();
  
  Page* get_page(size_t index);
  template<class TPage>
  inline TPage* get_page(size_t index) {
    return static_cast<TPage*>(get_page(index));
  }

  Page* find_page(size_t index, bool update_current_index = false);
  Page* find_page(const std::string& uuid, bool update_current_index = false);
  size_t find_page_index(const std::string &uuid) const;

  bool has_bookmark(uint8_t bookmark_id);
  Page* find_bookmarked_page(uint8_t bookmark_id,
    bool update_current_index = false);
  template<class TPage>
  inline TPage* find_bookmarked_page(uint8_t bookmark_id,
      bool update_current_index = false) {
    return static_cast<TPage*>(
      find_bookmarked_page(bookmark_id, update_current_index));
  }

  bool bookmark_page(uint8_t bookmark_id, size_t page_index,
    bool overwrite = false);
  bool bookmark_page(uint8_t bookmark_id, const std::string& uuid,
    bool overwrite = false);

  template <typename TPage, typename... TArgs>
  TPage* insert_page(size_t index, TArgs &&... args) {
    static_assert(
      std::is_base_of<Page, TPage>::value,
      "TPage must derive from esphome::nspanel_lovelace::Page");
    assert(page_count() < UINT8_MAX);
    if (index > pages_.size()) return nullptr;

    auto page = std::make_unique<TPage>(std::forward<TArgs>(args)...);
    TPage* p_page = page.get();
    if (pages_empty()) pages_.emplace_back(std::move(page));
    else {
      pages_.insert(pages_.begin() + index, std::move(page));
      if (index <= current_index_) ++current_index_;
      for (auto &[id, idx] : bookmarks_) {
        if (index <= idx) ++idx;
      }
    }
    return p_page;
  }

  template <typename TPage, typename... TArgs>
  TPage* create_page(TArgs &&... args) {
    static_assert(
      std::is_base_of<Page, TPage>::value,
      "TPage must derive from esphome::nspanel_lovelace::Page");
    assert(page_count() < UINT8_MAX);

    auto page = std::make_unique<TPage>(std::forward<TArgs>(args)...);
    TPage* p_page = page.get();
    pages_.emplace_back(std::move(page));
    return p_page;
  }

  void delete_page(size_t index);
  void delete_page(const std::string& uuid);

protected:
  size_t current_index_;
  std::vector<std::unique_ptr<Page>> pages_;
  std::unordered_map<uint8_t, size_t> bookmarks_;
};

} // namespace nspanel_lovelace
} // namespace esphome