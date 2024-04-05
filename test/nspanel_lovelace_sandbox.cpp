#include <iostream>
#include <memory>

#include "logging.h"
#include "mock_esphome_core.h"

#include "config.h"
#include "nspanel_lovelace.h"

#include "cards.h"
#include "card_items.h"

using namespace esphome;
using namespace nspanel_lovelace;

std::unique_ptr<nspanel_lovelace::NSPanelLovelace> nspanel;

void test_add_pages() {
  auto nspanel_card_1 = std::make_shared<nspanel_lovelace::GridCard>(
      "front_room", "Front Room", 10);

  auto nspanel_card_1_navleft =
      std::unique_ptr<nspanel_lovelace::NavigationItem>(
          new nspanel_lovelace::NavigationItem("10", "9", u8"\uE730"));
  nspanel_card_1->set_nav_left(nspanel_card_1_navleft);
  auto nspanel_card_1_navright =
      std::unique_ptr<nspanel_lovelace::NavigationItem>(
          new nspanel_lovelace::NavigationItem("11", "7", u8"\uE733"));
  nspanel_card_1->set_nav_right(nspanel_card_1_navright);

  auto nspanel_card_1_item_1 =
      std::make_shared<nspanel_lovelace::GridCardEntityItem>(
          "12", "light.front_room_all", "All");
  nspanel_card_1->add_item(nspanel_card_1_item_1);
  auto nspanel_card_1_item_2 =
      std::make_shared<nspanel_lovelace::GridCardEntityItem>(
          "13", "light.front_room_inner", "Inner");
  nspanel_card_1->add_item(nspanel_card_1_item_2);
  auto nspanel_card_1_item_3 =
      std::make_shared<nspanel_lovelace::GridCardEntityItem>(
          "14", "light.front_room_outer", "Outer");
  nspanel_card_1->add_item(nspanel_card_1_item_3);

  nspanel->add_page(nspanel_card_1);
}

int main()
{
    std::cout << "STARTING SADNBOX\n";
    
    nspanel = make_unique<nspanel_lovelace::NSPanelLovelace>();
    
    test_add_pages();
}