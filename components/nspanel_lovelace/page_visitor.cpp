#include "page_visitor.h"
#include "cards.h"
#include "pages.h"

namespace esphome {
namespace nspanel_lovelace {

bool InheritancePageVisitor::visit(Page &page) { return false; }
bool InheritancePageVisitor::visit(Screensaver &page) {
  return visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(Card &page) {
  return visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(GridCard &page) {
  return visit(static_cast<Card &>(page)) ||
          visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(EntitiesCard &page) {
  return visit(static_cast<Card &>(page)) ||
          visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(QRCard &page) {
  return visit(static_cast<Card &>(page)) ||
          visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(AlarmCard &page) {
  return visit(static_cast<Card &>(page)) ||
          visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(ThermoCard &page) {
  return visit(static_cast<Card &>(page)) ||
          visit(static_cast<Page &>(page));
}
bool InheritancePageVisitor::visit(MediaCard &page) {
  return visit(static_cast<Card &>(page)) ||
          visit(static_cast<Page &>(page));
}

} // namespace nspanel_lovelace
} // namespace esphome