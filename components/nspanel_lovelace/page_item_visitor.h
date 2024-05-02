#pragma once

#include <type_traits>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== PageItem Visitors ===============
 */

class PageItem;
class DeleteItem;
class NavigationItem;
class StatusIconItem;
class WeatherItem;
class AlarmButtonItem;
class AlarmIconItem;
class CardItem;
class StatefulPageItem;
class GridCardEntityItem;
class EntitiesCardEntityItem;

class PageItemVisitor {
public:
  virtual bool visit(PageItem &) = 0;
  virtual bool visit(DeleteItem &) = 0;
  virtual bool visit(NavigationItem &) = 0;
  virtual bool visit(StatusIconItem &) = 0;
  virtual bool visit(WeatherItem &) = 0;
  virtual bool visit(AlarmButtonItem &) = 0;
  virtual bool visit(AlarmIconItem &) = 0;
  virtual bool visit(CardItem &) = 0;
  virtual bool visit(StatefulPageItem &) = 0;
  virtual bool visit(GridCardEntityItem &) = 0;
  virtual bool visit(EntitiesCardEntityItem &) = 0;
};

class InheritancePageItemVisitor : public PageItemVisitor {
public:
  virtual bool visit(PageItem &);
  virtual bool visit(DeleteItem &);
  virtual bool visit(NavigationItem &);
  virtual bool visit(StatusIconItem &);
  virtual bool visit(WeatherItem &);
  virtual bool visit(AlarmButtonItem &);
  virtual bool visit(AlarmIconItem &);
  virtual bool visit(CardItem &);
  virtual bool visit(StatefulPageItem &);
  virtual bool visit(GridCardEntityItem &);
  virtual bool visit(EntitiesCardEntityItem &);
};

template <class T = PageItem>
class DerivedCastPageItemVisitor : public InheritancePageItemVisitor {
public:
  DerivedCastPageItemVisitor(T *&t) : t_casted_(t) {}
  virtual bool visit(T &t) {
    t_casted_ = &t;
    return true;
  }

private:
  T *&t_casted_;
};

template<class TDerived = PageItem, typename TBase>
TDerived *page_item_cast(TBase *obj) {
  static_assert(
      std::is_base_of<PageItem, TBase>::value, "TBase must derive from PageItem");
  TDerived *t = nullptr;
  if (obj) {
    DerivedCastPageItemVisitor<TDerived> visitor(t);
    obj->accept(visitor);
  }
  return t;
}

} // namespace nspanel_lovelace
} // namespace esphome