#include "page_manager.h"
#include <algorithm>
#include <utility>

namespace esphome {
namespace nspanel_lovelace {

bool PageManager::pages_empty() const {
  return pages_.empty();
}

size_t PageManager::page_count() const {
  return pages_.size();
}

size_t PageManager::current_index() const {
  return current_index_;
}

std::vector<std::unique_ptr<Page>>::const_iterator PageManager::begin() const {
  return pages_.begin();
}

std::vector<std::unique_ptr<Page>>::const_iterator PageManager::end() const {
  return pages_.end();
}

Page* PageManager::current_page() {
  if (pages_.empty()) return nullptr;
  return pages_[current_index_].get();
}

Page* PageManager::cycle_page(cycle_mode::Flag mode) {
  if (pages_.empty()) return nullptr;

  size_t original_index = current_index_;
  for (size_t i = (mode & cycle_mode::none ? 0 : 1);
      i < pages_.size(); ++i) {
    size_t index = (current_index_ + 
        (mode & cycle_mode::backward ? pages_.size() - i : i))
      % pages_.size();
    if (!pages_[index]->is_hidden()) {
      current_index_ = index;
      return pages_[current_index_].get();
    }
  }
  return pages_[original_index].get();
}

Page* PageManager::next_page() {
  return cycle_page(cycle_mode::forward);
}

Page* PageManager::previous_page() {
  return cycle_page(cycle_mode::backward);
}

Page* PageManager::get_page(size_t index) {
  if (index > pages_.size() - 1)
    index = pages_.size() - 1;
  return find_page(index, true);
}

Page* PageManager::find_page(
    size_t index, bool update_current_index) {
  if (index >= pages_.size()) return nullptr;
  auto p = &pages_[index];
  if (p && update_current_index) current_index_ = index;
  return p->get();
}

Page* PageManager::find_page(
    const std::string &uuid, bool update_current_index) {
  auto i = find_page_index(uuid);
  if (i != SIZE_MAX) {
    if (update_current_index) current_index_ = i;
    return pages_[i].get();
  }
  return nullptr;
}

size_t PageManager::find_page_index(const std::string &uuid) const {
  // shortcut if possible
  if (pages_[current_index_]->get_uuid() == uuid) return current_index_;

  for (size_t i = 0; i < pages_.size(); ++i) {
    if (pages_[i]->get_uuid() != uuid) continue;
    return i;
  }
  return SIZE_MAX;
}

bool PageManager::has_bookmark(uint8_t bookmark_id) {
  return bookmarks_.find(bookmark_id) != bookmarks_.end();
}

Page* PageManager::find_bookmarked_page(
    uint8_t bookmark_id, bool update_current_index) {
  if (page_count() == 0) return nullptr;
  auto it = bookmarks_.find(bookmark_id);
  if (it != bookmarks_.end() && it->second < pages_.size()) {
    if (update_current_index)
      current_index_ = it->second;
    return pages_[it->second].get();
  }
  return nullptr;
}

bool PageManager::bookmark_page(
    uint8_t bookmark_id, size_t page_index, bool overwrite) {
  if (page_index < pages_.size()) {
    auto b = bookmarks_.emplace(bookmark_id, page_index);
    if (!b.second && overwrite) b.first->second = page_index;
    return true;
  }
  return false;
}

bool PageManager::bookmark_page(
    uint8_t bookmark_id, const std::string& uuid, bool overwrite) {
  auto i = find_page_index(uuid);
  if (i == SIZE_MAX) return false;
  return bookmark_page(bookmark_id, i, overwrite);
}

void PageManager::delete_page(size_t index) {
  if (index >= pages_.size()) return;

  pages_.erase(pages_.begin() + index);
  if (pages_.empty()) {
    current_index_ = 0;
    bookmarks_.clear();
    return;
  }

  if (index < current_index_)
    --current_index_;
  else if (index == current_index_)
    this->cycle_page((cycle_mode::Flag)(cycle_mode::none | cycle_mode::backward));

  for (auto it = bookmarks_.begin(); it != bookmarks_.end();) {
    if (it->second == index)
      it = bookmarks_.erase(it);
    else {
      if (it->second > index)
        --(it->second);
      ++it;
    }
  }
}

void PageManager::delete_page(const std::string& uuid) {
  for (size_t i = 0; i < pages_.size(); ++i) {
    if (pages_[i]->get_uuid() == uuid) {
      delete_page(i);
      return;
    }
  }
}

} // namespace nspanel_lovelace
} // namespace esphome