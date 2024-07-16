#pragma once

#include <type_traits>

namespace esphome {
namespace nspanel_lovelace {

/*
 * =============== Page Visitors ===============
 */

class Page;
class Screensaver;
class Card;
class GridCard;
class EntitiesCard;
class QRCard;
class AlarmCard;
class ThermoCard;
class MediaCard;

class PageVisitor {
public:
  virtual bool visit(Page &) = 0;
  virtual bool visit(Screensaver &) = 0;
  virtual bool visit(Card &) = 0;
  virtual bool visit(GridCard &) = 0;
  virtual bool visit(EntitiesCard &) = 0;
  virtual bool visit(QRCard &) = 0;
  virtual bool visit(AlarmCard &) = 0;
  virtual bool visit(ThermoCard &) = 0;
  virtual bool visit(MediaCard &) = 0;
};

class InheritancePageVisitor : public PageVisitor {
public:
  virtual bool visit(Page &);
  virtual bool visit(Screensaver &);
  virtual bool visit(Card &);
  virtual bool visit(GridCard &);
  virtual bool visit(EntitiesCard &);
  virtual bool visit(QRCard &);
  virtual bool visit(AlarmCard &);
  virtual bool visit(ThermoCard &);
  virtual bool visit(MediaCard &);
};

template<class T = Page>
class DerivedCastPageVisitor : public InheritancePageVisitor {
public:
  DerivedCastPageVisitor(T *&t) : t_casted_(t) {}
  virtual bool visit(T &t) {
    t_casted_ = &t;
    return true;
  }

private:
  T *&t_casted_;
};

template<class TDerived = Page, typename TBase>
TDerived *page_cast(TBase *obj) {
  static_assert(
      std::is_base_of<Page, TBase>::value, "TBase must derive from Page");
  TDerived *t = nullptr;
  if (obj) {
    DerivedCastPageVisitor<TDerived> visitor(t);
    obj->accept(visitor);
  }
  return t;
}

} // namespace nspanel_lovelace
} // namespace esphome