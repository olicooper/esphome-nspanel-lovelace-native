#include "page_item_visitor.h"
#include "card_base.h"
#include "card_items.h"
#include "page_items.h"

namespace esphome {
namespace nspanel_lovelace {

bool InheritancePageItemVisitor::visit(PageItem &item) { return false; }
bool InheritancePageItemVisitor::visit(DeleteItem &item) {
  return visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(NavigationItem &item) {
  return visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(StatusIconItem &item) {
  return visit(static_cast<StatefulPageItem &>(item)) ||
          visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(WeatherItem &item) {
  return visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(AlarmButtonItem &item) {
  return visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(AlarmIconItem &item) {
  return visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(CardItem &item) {
  return visit(static_cast<StatefulPageItem &>(item)) ||
          visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(StatefulPageItem &item) {
  return visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(GridCardEntityItem &item) {
  return visit(static_cast<CardItem &>(item)) ||
          visit(static_cast<StatefulPageItem &>(item)) ||
          visit(static_cast<PageItem &>(item));
}
bool InheritancePageItemVisitor::visit(EntitiesCardEntityItem &item) {
  return visit(static_cast<CardItem &>(item)) ||
          visit(static_cast<StatefulPageItem &>(item)) ||
          visit(static_cast<PageItem &>(item));
}

} // namespace nspanel_lovelace
} // namespace esphome