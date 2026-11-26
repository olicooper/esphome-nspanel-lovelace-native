#include "panel_fixture.h"

using namespace esphome;
using namespace esphome::nspanel_lovelace;

static void test_uart_recovery() {
  Panel panel;
  std::vector<std::string> received;
  panel.add_incoming_msg_callback([&](std::string message) { received.push_back(message); });
  const std::string next = "event,custom,next";
  const auto wire = Panel::encode(next);
  const auto reached_next = [&]() {
    bool ok = received == std::vector<std::string>{next} && panel.buffered() == 0;
    received.clear();
    return ok;
  };
  panel.feed({0x55, 0xbb, 0x01, 0x10});  // 4097 bytes
  check(panel.buffered() == 0, "oversized UART header rejected immediately");
  panel.feed(wire);
  check(reached_next(), "valid frame accepted after oversized header");
  panel.feed({0x55});
  panel.feed(wire);
  check(reached_next(), "overlapping UART headers recover");
  panel.feed({0x55, 0xbb, 0xff});
  panel.feed(wire);
  check(reached_next(), "rejected length retains trailing first sync byte");
  host_millis += 200;
  panel.feed({0x55, 0xbb});
  panel.feed(wire);
  check(reached_next(), "rejected length retains trailing sync pair");
  host_millis += 200;
  panel.feed({0x88, 0x55});
  panel.feed({wire.begin() + 1, wire.end()});
  check(reached_next(), "rejected Nextion ready header retains frame sync");
  host_millis += 200;
  panel.feed({0x55, 0xbb, 0x00, 0x10, 'x'});  // In-range (4096), but truncated.
  host_millis += 200;
  panel.feed(wire);
  check(reached_next(), "valid frame accepted after idle following truncated in-range UART frame");

  // A plausible partial length swallows a whole following frame in the same drain.
  const auto lost = Panel::encode("event,custom,swallowed");
  const auto length = static_cast<uint8_t>(lost.size() + 1);
  std::vector<uint8_t> burst{0x55, 0xbb, length, 0, 'x'};
  burst.insert(burst.end(), lost.begin(), lost.end());
  const auto crc = crc16(burst.data(), burst.size());
  burst.push_back(static_cast<uint8_t>(crc ^ 1));
  burst.push_back(static_cast<uint8_t>(crc >> 8));
  burst.insert(burst.end(), wire.begin(), wire.end());
  panel.feed(burst);
  check(reached_next(), "same UART drain drops embedded frame and recovers after bad outer CRC");

  Panel fragmented;
  fragmented.add_incoming_msg_callback([&](std::string message) { received.push_back(message); });
  const std::string outer = "event,custom,outer:" + std::string(wire.begin(), wire.end()) + ":end";
  const auto outer_wire = Panel::encode(outer);
  fragmented.feed({outer_wire.begin(), outer_wire.end() - 6});
  check(received.empty(), "embedded CRC-valid packet not dispatched before outer frame completes");
  host_millis += 20;
  fragmented.feed({outer_wire.end() - 6, outer_wire.end()});
  check(received == std::vector<std::string>{outer}, "fragmented outer frame dispatches exactly once");
  received.clear();
  fragmented.feed({outer_wire.begin(), outer_wire.end() - 6});
  host_millis += 200;
  fragmented.loop();
  check(received.empty() && fragmented.buffered() == 0, "expiry discards partial frame without dispatching embedded packet");
}

static void test_wire() {
  Panel panel;
  auto climate = panel.create_entity("climate.test");
  climate->set_attribute(ha_attr_type::preset_modes, "['eco','home']");
  climate->set_attribute(ha_attr_type::swing_modes, "['off','both']");
  climate->set_attribute(ha_attr_type::fan_modes, "['auto','high']");
  panel.create_entity("light.test");
  const auto reject = [&](const std::string &wire, const std::string &label = "") {
    calls.clear();
    panel.frame("event,buttonPress2," + wire);
    check(calls.empty(), "reject " + (label.empty() ? wire : label));
  };
  const auto accept = [&](const std::string &wire, const char *action, const Data &data, const Data &templates = {}) {
    calls.clear();
    panel.frame("event,buttonPress2," + wire);
    check(calls.size() == 1 && calls.front().action == action && calls.front().data == data &&
          calls.front().data_template == templates, "dispatch " + wire);
  };
  for (const char *bad : {"abc", "-1", "2", "0junk", "", "2147483648"})
    for (const char *mode : {"preset_modes", "swing_modes", "fan_modes"})
      reject(std::string("climate.test,mode-") + mode + "," + bad);
  reject(std::string("climate.test,mode-preset_modes,0") + '\0' + 'x', "mode index with embedded NUL");
  accept("climate.test,mode-preset_modes,0", "climate.set_preset_mode", {{"entity_id", "climate.test"}, {"preset_mode", "eco"}});
  accept("climate.test,mode-swing_modes,0", "climate.set_swing_mode", {{"entity_id", "climate.test"}, {"swing_mode", "off"}});
  accept("climate.test,mode-fan_modes,0", "climate.set_fan_mode", {{"entity_id", "climate.test"}, {"fan_mode", "auto"}});
  for (const char *wire : {"light.test,OnOff,off", "light.test,OnOff,2", "light.test,,OnOff,1",
      "light.test,OnOff,,1", "light.test,OnOff,1,", "light.test,brightnessSlider,-1",
      "light.test,colorTempSlider,101", "cover.test,positionSlider,101", "cover.test,tiltSlider,-1",
      "media_player.test,volumeSlider,101", "climate.test,tempUpdHighLow,|250|190",
      "climate.test,tempUpdHighLow,250||190", "climate.test,tempUpdHighLow,250|190|",
      "climate.test,tempUpdHighLow,250|190|210", "light.test,colorWheel,80||80|160",
      "light.test,colorWheel,80|80|160|", "light.test,colorWheel,0|0|1e-46",
      "light.test,colorWheel,0|0|1e-45", "light.test,colorWheel,0|0|1e100",
      "light.test,colorWheel,1e308|0|160",
      "timer.test,timer-start,0::30:00", "timer.test,timer-start,0:35:60",
      "timer.test,timer-start,999999999:00:00", "timer.test,timer-start,abc",
      "timer.test,timer", "timer.test,timer-cancel,0:01:00", "number.test,number-set,nan"}) reject(wire);
  accept("light.test,OnOff,1", "light.turn_on", {{"entity_id", "light.test"}});
  accept("light.test,OnOff,0", "light.turn_off", {{"entity_id", "light.test"}});
  accept("light.test,brightnessSlider,100", "light.turn_on", {{"entity_id", "light.test"}, {"brightness", "255"}});
  accept("light.test,colorWheel,160|80|160", "light.turn_on", {{"entity_id", "light.test"}}, {{"rgb_color", "[255,0,0]"}});
  accept("climate.test,tempUpdHighLow,250|190", "climate.set_temperature", {{"entity_id", "climate.test"}, {"target_temp_high", "25.0"}, {"target_temp_low", "19.0"}});
  accept("climate.test,tempUpd,-32768", "climate.set_temperature", {{"entity_id", "climate.test"}, {"temperature", "-3276.8"}});
  reject("climate.test,tempUpd,-32769");
  reject("climate.test,tempUpdHighLow,32768|-32768");
  climate->set_attribute(ha_attr_type::min_temp, "-15.5");
  climate->set_attribute(ha_attr_type::max_temp, "120");
  for (const char *wire : {"climate.test,tempUpd,2147483647", "climate.test,tempUpd,-2147483648",
      "climate.test,tempUpd,1205", "climate.test,tempUpd,-160", "climate.test,tempUpdHighLow,1205|-155",
      "climate.test,tempUpdHighLow,1200|-160", "climate.test,tempUpdHighLow,2147483647|-2147483648"}) reject(wire);
  accept("climate.test,tempUpd,-155", "climate.set_temperature", {{"entity_id", "climate.test"}, {"temperature", "-15.5"}});
  accept("climate.test,tempUpd,1200", "climate.set_temperature", {{"entity_id", "climate.test"}, {"temperature", "120.0"}});
  accept("climate.test,tempUpdHighLow,1200|-155", "climate.set_temperature", {{"entity_id", "climate.test"}, {"target_temp_high", "120.0"}, {"target_temp_low", "-15.5"}});
  const std::pair<const char *, const char *> movement[] = {
    {"up", "cover.open_cover"}, {"stop", "cover.stop_cover"}, {"down", "cover.close_cover"},
    {"tiltOpen", "cover.open_cover_tilt"}, {"tiltStop", "cover.stop_cover_tilt"}, {"tiltClose", "cover.close_cover_tilt"},
  };
  for (const auto &[button, service] : movement) {
    const std::string wire = std::string("cover.test,") + button;
    reject(wire + ",0");
    accept(wire, service, {{"entity_id", "cover.test"}});
    accept(wire + ",", service, {{"entity_id", "cover.test"}});
  }
  accept("timer.test,timer-start,00:90:01", "timer.start", {{"entity_id", "timer.test"}, {"duration", "01:30:01"}});
  accept("timer.test,timer-start,00:1440:00", "timer.start", {{"entity_id", "timer.test"}, {"duration", "24:00:00"}});
  accept("timer.test,timer-start,00:65535:59", "timer.start", {{"entity_id", "timer.test"}, {"duration", "1092:15:59"}});
  for (const char *bad : {"00:65536:00", "00:90:60", "00:-90:01", "01:90:01", "00:90:01:"})
    reject(std::string("timer.test,timer-start,") + bad);
  accept("timer.test,timer-start,0:35:40", "timer.start", {{"entity_id", "timer.test"}, {"duration", "0:35:40"}});
  accept("timer.test,timer-start", "timer.start", {{"entity_id", "timer.test"}});
  accept("timer.test,timer-start,", "timer.start", {{"entity_id", "timer.test"}});
  reject("number.test,number-set,0x1p2");
  reject("climate.test,tempUpdHighLow,190|250");
  accept("number.test,number-set,12345678.123456789", "number.set_value", {{"entity_id", "number.test"}, {"value", "12345678.123456789"}});
  display.clear();
  calls.clear();
  std::vector<std::string> raw_events;
  panel.add_incoming_msg_callback([&](std::string message) { raw_events.push_back(message); });
  panel.frame("event,custom,a,b,c,d,e");
  check(raw_events == std::vector<std::string>{"event,custom,a,b,c,d,e"}, "custom multi-field event reaches callback");
  panel.frame("event,buttonPress2," + std::string(200, ','));
  check(calls.empty(), "separator-heavy frame rejected");
  const std::vector<std::string> short_details{
    "event,pageOpenDetail,cover,", "event,pageOpenDetail,,uuid.cover", "event,pageOpenDetail,cover"};
  raw_events.clear();
  for (const auto &event : short_details) panel.frame(event);
  check(display.empty(), "short detail frames do not render");
  check(raw_events == short_details, "each short detail frame reaches the raw callback exactly once");
  panel.frame("event,pageOpenDetail,cover,uuid.cover");
  check(display.empty(), "unknown detail item is ignored");
}

static void test_button_validation() {
  Panel panel;
  calls.clear();
  std::vector<std::string> raw;
  panel.add_incoming_msg_callback([&](std::string event) { raw.push_back(event); });
  for (const std::string bad : {"switch.test,button,junk", "switch.test,media-next", "media_player.test,media-back,junk",
       "climate.test,hvac_action,", "climate.test,hvac_action,heating", "light.test,hvac_action,heat"}) {
    const auto event = "event,buttonPress2," + bad;
    raw.clear();
    panel.frame(event);
    check(calls.empty() && raw == std::vector<std::string>{event}, "bad action ignored, raw event kept: " + bad);
  }
  for (const auto &[action, service] : {std::pair{"switch.test,button", "switch.toggle"},
       std::pair{"media_player.test,media-back", "media_player.media_previous_track"},
       std::pair{"climate.test,hvac_action,heat", "climate.set_hvac_mode"}}) {
    calls.clear();
    panel.frame(std::string("event,buttonPress2,") + action);
    check(calls.size() == 1 && calls.front().action == service, std::string("valid action: ") + action);
  }
  calls.clear();
  for (const std::string value : {"10", "20", "30", "junk"})
    panel.feed(Panel::encode("event,buttonPress2,light.test,brightnessSlider," + value));
  panel.feed(Panel::encode("event,buttonPress2,switch.test,media-next"));
  const auto origin = panel.create_page<EntitiesCard>("origin");
  const auto destination = panel.create_page<EntitiesCard>("destination");
  panel.feed(Panel::encode("event,buttonPress2,navigate.uuid.missing,button"));
  panel.feed(Panel::encode("event,buttonPress2,switch.bad.name,button"));
  display.clear();
  panel.feed(Panel::encode("event,buttonPress2,navigate.uuidXdestination,button"));
  check(panel.page_mgr_.current_page() == origin && display.empty() && panel.timeouts.count("btnpr"),
        "malformed navigate.uuid delimiter cannot navigate or cancel queued repeat");
  check(calls.size() == 1 && panel.timeouts.count("btnpr"), "malformed action neither dispatches nor cancels queued repeat");
  panel.run_timeout();
  check(calls.size() == 2 && calls.back().data == Data{{"entity_id", "light.test"}, {"brightness", "76"}},
        "malformed repeat cannot replace last valid queued payload");
  panel.frame("event,buttonPress2,navigate.uuid.destination,button");
  check(panel.page_mgr_.current_page() == destination && display == std::vector<std::string>{"current page"},
        "correctly delimited navigate.uuid switches to the existing destination");
}

static void test_popup_selection() {
  Panel panel;
  const auto cover = panel.create_entity("cover.test");
  const auto sw = panel.create_entity("switch.test");
  const auto first = panel.create_page<EntitiesCard>("first");
  first->add_item(std::make_shared<EntitiesCardEntityItem>("cover", cover));
  first->add_item(std::make_shared<EntitiesCardEntityItem>("switch", sw));
  panel.create_page<EntitiesCard>("second");
  for (const std::string event : {"popupFan,uuid.cover", "popupShutter,uuid.switch", "unknown,uuid.cover"}) {
    display.clear();
    panel.frame("event,pageOpenDetail," + event);
    check(display.empty() && panel.popup_page_current_uuid_.empty(), "wrong or unsupported popup: " + event);
    panel.popup_page_current_uuid_.clear();
  }
  panel.frame("event,pageOpenDetail,popupShutter,uuid.cover");
  check(panel.popup_page_current_uuid_ == "cover", "successful popup records UUID");
  panel.popup_page_current_uuid_.clear();
  panel.frame("event,buttonPress2,uuid.cover,up");  // Populate the global UUID cache.
  panel.page_mgr_.find_page("second", true);
  display.clear();
  panel.frame("event,pageOpenDetail,popupShutter,uuid.cover");
  check(display.empty() && panel.popup_page_current_uuid_.empty(), "cached item from a different page cannot open popup");
}

static void test_light_effect_popup() {
  Panel panel;
  auto light = panel.create_entity("light.test");
  auto first = panel.create_page<EntitiesCard>("light-page");
  first->add_item(std::make_shared<EntitiesCardEntityItem>("light", light));
  panel.create_page<EntitiesCard>("other-page");
  light->set_attribute(ha_attr_type::effect_list, "['Pulse','Rainbow']");
  display.clear();
  panel.frame("event,pageOpenDetail,popupLight,uuid.light");
  check(!display.empty() && display.front().find("entityUpdateDetail~uuid.light~") == 0, "light detail opens first");
  display.clear();
  panel.frame("event,pageOpenDetail,popupInSel,uuid.light");
  check(!display.empty() && display.front().find("entityUpdateDetail2~uuid.light~") == 0 &&
        display.front().find("~Pulse?Rainbow~") != std::string::npos, "effect button opens light options");
  calls.clear();
  panel.frame("event,buttonPress2,uuid.light,mode-light,1");
  check(calls.size() == 1 && calls.front().data == Data{{"entity_id", "light.test"}, {"effect", "Rainbow"}}, "effect index dispatches");
  display.clear();
  panel.on_entity_attribute_update_("light.test", "state", "on");
  panel.run_timeout("light.test");
  panel.on_entity_attribute_update_("light.test", "effect_list", "['Pulse','Rainbow']");
  panel.run_timeout("light.test");
  check(panel.popup_page_current_uuid_ == "light" &&
        std::find(display.begin(), display.end(), "current page") == display.end(),
        "state update and unchanged options leave effects picker open");

  calls.clear();
  panel.feed(Panel::encode("event,buttonPress2,uuid.light,mode-light,1"));
  check(panel.timeouts.count("btnpr") && calls.empty(), "valid effect repeat is queued before options change");
  display.clear();
  panel.on_entity_attribute_update_("light.test", "effect_list", "['Pulse','Sparkle']");
  check(!panel.timeouts.count("btnpr"), "options change cancels a queued effect tap");
  panel.run_timeout("light.test");
  check(display == std::vector<std::string>{"current page"}, "changed options close effects picker instead of relabeling in place");
  panel.run_timeout("btnpr");
  check(calls.empty(), "cancelled effect repeat cannot dispatch");

  // Keep an unrelated valid repeat pending while the stale frame arrives on its own.
  panel.feed(Panel::encode("event,buttonPress2,light.test,brightnessSlider,10"));
  panel.feed(Panel::encode("event,buttonPress2,light.test,brightnessSlider,30"));
  check(panel.timeouts.count("btnpr") == 1, "valid brightness repeat is pending before stale tap");
  calls.clear();
  const auto before_stale_tap = panel.debounce_state();
  panel.feed(Panel::encode("event,buttonPress2,uuid.light,mode-light,0"));
  check(calls.empty(), "standalone stale effect tap makes no HA call");
  check(panel.debounce_state() == before_stale_tap, "standalone stale effect tap leaves debounce unchanged");
  panel.run_timeout("btnpr");
  check(calls.size() == 1 && calls.front().data == Data{{"entity_id", "light.test"}, {"brightness", "76"}},
        "valid repeat still dispatches after stale effect tap");
  calls.clear();

  display.clear();
  panel.frame("event,pageOpenDetail,popupInSel,uuid.light");
  check(!display.empty() && display.front().find("~Pulse?Sparkle~") != std::string::npos, "reopened picker shows new options");
  panel.frame("event,buttonPress2,uuid.light,mode-light,1");
  check(calls.size() == 1 && calls.front().data == Data{{"entity_id", "light.test"}, {"effect", "Sparkle"}},
        "fresh tap selects the newly displayed effect");

  panel.feed(Panel::encode("event,buttonPress2,light.test,brightnessSlider,10"));
  panel.feed(Panel::encode("event,buttonPress2,light.test,brightnessSlider,30"));
  check(panel.popup_page_current_uuid_ == "light" && panel.timeouts.count("btnpr") == 1,
        "brightness repeat is pending while the effects picker is open");
  const auto before_close = panel.debounce_state();
  calls.clear();
  display.clear();
  panel.on_entity_attribute_update_("light.test", "effect_list", "['Pulse','Rainbow']");
  check(display == std::vector<std::string>{"current page"} && panel.popup_page_current_uuid_.empty() &&
        panel.debounce_state() == before_close && calls.empty(),
        "effects picker closes without changing the pending brightness repeat");
  panel.run_timeout("btnpr");
  check(calls.size() == 1 && calls.front().action == "light.turn_on" &&
        calls.front().data == Data{{"entity_id", "light.test"}, {"brightness", "76"}},
        "brightness repeat queued before picker closure still dispatches");

  panel.on_entity_attribute_update_("light.test", "effect_list", "['', '?', 'Pulse?Fast', '', 'Sea~Blue, Slow']");
  display.clear();
  panel.frame("event,pageOpenDetail,popupInSel,uuid.light");
  check(!display.empty() && display.front().find("~_?Pulse_Fast?Sea_Blue, Slow~") != std::string::npos,
        "effect labels preserve one slot per nonempty entry and escape TFT delimiters");
  for (const auto &[index, effect] : {std::pair{"0", "?"}, std::pair{"1", "Pulse?Fast"}, std::pair{"2", "Sea~Blue, Slow"}}) {
    calls.clear();
    panel.frame(std::string("event,buttonPress2,uuid.light,mode-light,") + index);
    check(calls.size() == 1 && calls.front().data == Data{{"entity_id", "light.test"}, {"effect", effect}},
          std::string("effect tap preserves original option at index ") + index);
  }
  calls.clear();
  panel.frame("event,buttonPress2,uuid.light,mode-light,3");
  check(calls.empty(), "empty entries and embedded delimiters do not add effect slots");
  panel.on_entity_attribute_update_("light.test", "effect_list", "['','']");
  display.clear();
  panel.frame("event,pageOpenDetail,popupInSel,uuid.light");
  check(!display.empty() && display.front().size() >= 2 && display.front().substr(display.front().size() - 2) == "~~",
        "all-empty effects produce an empty picker");
  panel.frame("event,buttonPress2,uuid.light,mode-light,0");
  check(calls.empty(), "empty picker does not dispatch an effect");

  panel.page_mgr_.find_page("other-page", true);
  display.clear();
  panel.frame("event,pageOpenDetail,popupInSel,uuid.light");
  check(display.empty(), "effects popup rejects a light from another page");
}

static void test_effect_utf8() {
  Panel panel;
  const auto light = panel.create_entity("light.test");
  panel.create_page<EntitiesCard>("effects")->add_item(std::make_shared<EntitiesCardEntityItem>("light", light));
  const auto open = [&](const std::string &value) {
    panel.on_entity_attribute_update_("light.test", "effect_list", value);
    panel.run_timeout("light.test");
    display.clear();
    panel.frame("event,pageOpenDetail,popupInSel,uuid.light");
  };
  const auto tap = [&](int index, const std::string &effect) {
    calls.clear();
    panel.frame("event,buttonPress2,uuid.light,mode-light," + std::to_string(index));
    return calls.size() == 1 && calls.front().action == "light.turn_on" &&
        calls.front().data == Data{{"entity_id", "light.test"}, {"effect", effect}};
  };
  open(R"(['Pulse','Warm\xa0White','Invisible\u200bMark','Tag\U000e0001','Rainbow'])");
  const std::string escaped_labels = u8"Pulse?Warm\u00a0White?Invisible\u200bMark?Tag\U000e0001?Rainbow";
  check(!display.empty() && display.front().find("~" + escaped_labels + "~") != std::string::npos,
        "Python repr escapes render UTF-8 without dropping neighbors");
  check(tap(0, "Pulse") && tap(1, u8"Warm\u00a0White") && tap(2, u8"Invisible\u200bMark") &&
        tap(3, u8"Tag\U000e0001") && tap(4, "Rainbow"), "escaped Unicode taps dispatch exact UTF-8 values");

  open(R"(['Pulse','Line\nBreak','Carriage\rReturn','Rainbow'])");
  check(!display.empty() && display.front().find("~Pulse?Line_Break?Carriage_Return?Rainbow~") != std::string::npos,
        "escaped LF and CR are sanitized only in labels");
  check(tap(0, "Pulse") && tap(1, "Line\nBreak") && tap(2, "Carriage\rReturn") && tap(3, "Rainbow"),
        "escaped LF and CR dispatch decoded values with neighboring indices intact");

  for (char control : {'\n', '\r'}) {
    open(std::string("['Pulse','Raw") + control + "Break','Rainbow']");
    calls.clear();
    panel.frame("event,buttonPress2,uuid.light,mode-light,0");
    check(!display.empty() && display.front().size() >= 2 &&
          display.front().substr(display.front().size() - 2) == "~~" && calls.empty(),
          control == '\n' ? "raw quoted LF rejects the whole list" : "raw quoted CR rejects the whole list");
  }

  for (const std::string &bad : {std::string(R"(\xG0)"), std::string(R"(\u12)"), std::string(R"(\ud800)"),
       std::string(R"(\U00110000)"), std::string("\xc0\xaf"), std::string("\xe2\x82")}) {
    open("['Pulse','" + bad + "','Rainbow']");
    calls.clear();
    panel.frame("event,buttonPress2,uuid.light,mode-light,0");
    check(!display.empty() && display.front().size() >= 2 &&
          display.front().substr(display.front().size() - 2) == "~~" && calls.empty(),
          "invalid effect encoding fails closed without shifting indices");
  }

  std::string list = "[";
  std::string labels;
  std::string last_effect;
  for (int i = 0; i < 16; ++i) {
    const std::string prefix = std::string(i == 13 ? 34 : 36, 'L') + std::to_string(10 + i);
    const std::string glyph = i >= 13 ? u8"\U0001f600" : i % 2 == 0 ? u8"\u20ac" : u8"\u00e9";
    const std::string effect = prefix + glyph + std::string(20, 'Z');
    if (i != 0) list += ',';
    list += "'" + effect + "'";
    if (i == 14) last_effect = effect;
    if (i >= 15) continue;
    if (!labels.empty()) labels += '?';
    labels += prefix;
    if (i % 2 != 0) labels += glyph;
  }
  open(list + "]");
  check(labels.size() == 598 && !display.empty() && display.front().find("~" + labels + "~") != std::string::npos,
        "15 long labels fit the TFT list and truncate only at complete UTF-8 characters");
  check(tap(14, last_effect), "15th effect dispatches its full untruncated name");
  calls.clear();
  panel.frame("event,buttonPress2,uuid.light,mode-light,15");
  check(calls.empty(), "16th effect is not selectable");
}

static void test_uuid() {
  Panel panel;
  const auto cover = panel.create_entity("cover.test");
  const auto page = panel.create_page<EntitiesCard>("uuid-page");
  page->add_item(std::make_shared<EntitiesCardEntityItem>("cover", cover));
  page->add_item(std::make_shared<EntitiesCardEntityItem>("", cover));
  std::vector<std::string> raw;
  panel.add_incoming_msg_callback([&](std::string event) { raw.push_back(event); });
  for (const std::string id : {"uuid.cover", "uuid.missing", "uuid.", "uuidXcover", "uuidx.cover", "uuid"}) {
    display.clear();
    raw.clear();
    const std::string event = "event,pageOpenDetail,popupShutter," + id;
    panel.frame(event);
    check((display.empty() == (id != "uuid.cover")) && raw == std::vector<std::string>{event},
          "popup render and unchanged raw callback: " + id);
    calls.clear();
    raw.clear();
    const std::string button = "event,buttonPress2," + id + ",up";
    panel.frame(button);
    check(raw == std::vector<std::string>{button}, "unchanged button raw callback: " + id);
    check(id == "uuid.cover" ? calls.size() == 1 && calls.front().data == Data{{"entity_id", "cover.test"}} : calls.empty(),
          "cover command UUID resolution: " + id);
  }
}

static void test_thermostat_steps() {
  Panel panel;
  auto climate = panel.create_entity("climate.test");
  ThermoCard thermo("thermo", climate);
  const Data single_min{{"entity_id", "climate.test"}, {"temperature", "20.0"}};
  const Data single_max{{"entity_id", "climate.test"}, {"temperature", "25.0"}};
  auto dispatch = [&](const std::string &event, const Data &expected, const std::string &label) {
    calls.clear();
    panel.frame("event,buttonPress2,climate.test," + event);
    check(expected.empty() ? calls.empty() : calls.size() == 1 &&
          calls.front().action == "climate.set_temperature" && calls.front().data == expected, label);
  };
  auto render_controls = [&]() {
    std::string buffer;
    thermo.render(buffer);
    std::vector<std::string> fields;
    size_t start = 0, end;
    while ((end = buffer.find('~', start)) != std::string::npos) {
      fields.emplace_back(buffer, start, end - start);
      start = end + 1;
    }
    std::array<int, 5> values{};
    check(fields.size() > 57 && str_to_int(fields[16], values[0]) && str_to_int(fields[18], values[1]) &&
          str_to_int(fields[19], values[2]) && str_to_int(fields[20], values[3]) &&
          (fields[57].empty() || str_to_int(fields[57], values[4])), "TFT receives numeric thermostat fields");
    return values;
  };
  auto set_limits = [&](const char *min, const char *max, const char *step) {
    climate->set_attribute(ha_attr_type::min_temp, min);
    climate->set_attribute(ha_attr_type::max_temp, max);
    climate->set_attribute(ha_attr_type::target_temp_step, step);
  };
  set_limits("20.0", "25.0", "0.5");
  climate->set_attribute(ha_attr_type::temperature, "20.2");
  const auto lower = render_controls();
  // v4.3.3 tests the pre-step value only, then sends the result verbatim.
  int next = lower[0] > lower[1] ? lower[0] - lower[3] : lower[0];
  check(next == 197, "TFT decrements 20.2 to 19.7 before sending");
  dispatch("tempUpd," + std::to_string(next), single_min, "single lower TFT overshoot clamps to 20.0");
  climate->set_attribute(ha_attr_type::temperature, "24.8");
  const auto upper = render_controls();
  next = upper[0] < upper[2] ? upper[0] + upper[3] : upper[0];
  check(next == 253, "TFT increments 24.8 to 25.3 before sending");
  dispatch("tempUpd," + std::to_string(next), single_max, "single upper TFT overshoot clamps to 25.0");

  set_limits("-2.04", "3.07", "0.25");
  climate->set_attribute(ha_attr_type::temperature, "");
  climate->set_attribute(ha_attr_type::target_temp_high, "2.9");
  climate->set_attribute(ha_attr_type::target_temp_low, "-1.9");
  const auto [dual_high, dual_min, dual_max, dual_step, dual_low] = render_controls();
  check(dual_min == -20 && dual_max == 30 && dual_step == 2, "fractional HA bounds round inward and TFT step truncates to tenths");
  const int high = dual_high < dual_max ? dual_high + dual_step : dual_high;
  const int low = dual_low > dual_min ? dual_low - dual_step : dual_low;
  check(high == 31 && low == -21, "dual TFT buttons step past both limits before sending");
  dispatch("tempUpdHighLow," + std::to_string(high) + "|" + std::to_string(low),
          {{"entity_id", "climate.test"}, {"target_temp_high", "3.0"}, {"target_temp_low", "-2.0"}},
          "dual fractional overshoots clamp to legal signed endpoints");
  dispatch("tempUpdHighLow,30|-19", {{"entity_id", "climate.test"}, {"target_temp_high", "3.0"},
          {"target_temp_low", "-1.9"}}, "in-range negative dual setpoint is unchanged");
  dispatch("tempUpdHighLow,-21|-19", {}, "reversed pair rejects before normalization");
  dispatch("tempUpdHighLow,32|-22", {}, "a full step past either limit cannot result from TFT pre-step check");
  dispatch("tempUpd,-2000", {}, "grossly out-of-range single setpoint rejects");

  set_limits("20.04", "25.0", "0.1");
  climate->set_attribute(ha_attr_type::target_temp_high, "24.0");
  climate->set_attribute(ha_attr_type::target_temp_low, "20.04");
  const auto [high_up, min_up, max_up, step_up, low_up] = render_controls();
  check(low_up == min_up && min_up == 201, "rendered low setpoint matches the fractional minimum");
  dispatch("tempUpdHighLow," + std::to_string(high_up < max_up ? high_up + step_up : high_up) + "|" + std::to_string(low_up),
          {{"entity_id", "climate.test"}, {"target_temp_high", "24.1"}, {"target_temp_low", "20.1"}},
          "dual high-up dispatches with untouched fractional low endpoint");

  set_limits("-5.0", "-2.04", "0.1");
  climate->set_attribute(ha_attr_type::target_temp_high, "-2.04");
  climate->set_attribute(ha_attr_type::target_temp_low, "-4.0");
  const auto [high_down, min_down, max_down, step_down, low_down] = render_controls();
  check(high_down == max_down && max_down == -21, "rendered high setpoint matches the negative fractional maximum");
  dispatch("tempUpdHighLow," + std::to_string(high_down) + "|" + std::to_string(low_down > min_down ? low_down - step_down : low_down),
          {{"entity_id", "climate.test"}, {"target_temp_high", "-2.1"}, {"target_temp_low", "-4.1"}},
          "dual low-down dispatches with untouched fractional high endpoint");

  set_limits("20.04", "24.96", "0.5");
  const auto positive = render_controls();
  check(positive[1] == 201 && positive[2] == 249, "positive fractional minimum rounds toward the legal range");
  dispatch("tempUpd,200", {{"entity_id", "climate.test"}, {"temperature", "20.1"}},
          "single fractional minimum clamps inward");
  set_limits("-5.0", "-2.04", "0.5");
  const auto negative = render_controls();
  check(negative[1] == -50 && negative[2] == -21, "negative fractional maximum rounds toward the legal range");
  dispatch("tempUpd,-20", {{"entity_id", "climate.test"}, {"temperature", "-2.1"}},
          "single negative fractional maximum clamps inward");
  set_limits("20.04", "20.06", "0.5");
  dispatch("tempUpd,200", {}, "no legal tenths endpoint in an empty representable range");
}

static void test_tile_callbacks() {
  const std::string switch_type = "switch", cover_type = "cover";
  const auto sw = std::make_shared<Entity>("switch.test", switch_type.c_str());
  const auto cover = std::make_shared<Entity>("cover.test", cover_type.c_str());
  GridCardEntityItem grid_switch("grid-switch", sw), grid_cover("grid-cover", cover);
  EntitiesCardEntityItem entities_switch("entities-switch", sw);
  sw->set_state("on");
  check(grid_switch.get_icon_color() == 64909, "grid switch callback sets on color");
  check(entities_switch.get_icon_color() == 64909 && entities_switch.get_value() == "1", "entities switch callback sets on wire value");
  cover->set_state("open");
  check(grid_cover.get_icon_color() == 64909, "grid cover callback sets open color");
  cover->set_state("closed");
  check(grid_cover.get_icon_color() == 17299, "grid cover callback sets closed color");

  const std::string lock_type = "lock";
  const auto lock = std::make_shared<Entity>("lock.test", lock_type.c_str());
  StatusIconItem status_lock("status-lock", lock);
  lock->set_state("locked");
  check(std::strcmp(CHAR8_CAST(status_lock.get_icon_value()), CHAR8_CAST(icon_t::lock)) == 0,
        "lock status icon is a closed padlock while locked");
  lock->set_state("unlocked");
  check(std::strcmp(CHAR8_CAST(status_lock.get_icon_value()), CHAR8_CAST(icon_t::lock_open)) == 0,
        "lock status icon opens when unlocked");
}

static void test_numeric_render() {
  auto climate = std::make_shared<Entity>("climate.test");
  ThermoCard thermo("thermo", climate);
  std::string buffer;
  climate->set_attribute(ha_attr_type::temperature, "3e38");
  climate->set_attribute(ha_attr_type::min_temp, "-214748400");
  climate->set_attribute(ha_attr_type::max_temp, "214748364.7");
  climate->set_attribute(ha_attr_type::target_temp_step, "1e20");
  thermo.render(buffer);
  check(buffer.find("~0~unknown~0~0~5~") != std::string::npos,
        "thermostat extreme values render safe single/min/max/step fallbacks");
  climate->set_attribute(ha_attr_type::temperature, "");
  climate->set_attribute(ha_attr_type::target_temp_high, "22.5");
  climate->set_attribute(ha_attr_type::target_temp_low, "-1e30");
  thermo.render(buffer);
  check(buffer.find("~225~unknown~") != std::string::npos && buffer.size() >= 4 &&
        buffer.compare(buffer.size() - 4, 4, "~0~1") == 0, "thermostat dual setpoint conversion is safe");

  auto media = std::make_shared<Entity>("media_player.test");
  MediaCard media_card("media", media);
  media->set_attribute(ha_attr_type::volume_level, "3e38");
  media_card.render(buffer);
  check(buffer.find("media_player.test~~~~~0~") != std::string::npos, "bad media volume renders safe fallback");

  const char *ranges[][4] = {{"200", "200", "200", "14"}, {"500", "153", "200", "14"}, {"-1", "66036", "200", "14"},
      {"200", "201", "2147483647", "100"}, {"200", "201", "-2147483648", "0"}};
  for (const auto &[min, max, value, expected] : ranges) {
    Entity light("light.test");
    light.set_attribute(ha_attr_type::min_mireds, min);
    light.set_attribute(ha_attr_type::max_mireds, max);
    light.set_attribute(ha_attr_type::color_temp, value);
    check(light.get_attribute(ha_attr_type::color_temp) == expected,
          std::string("mired conversion with bounds ") + min + ".." + max + " and value " + value);
  }

  Panel panel;
  auto fan = panel.create_entity("fan.test");
  const auto page = panel.create_page<EntitiesCard>("fan-page");
  page->add_item(std::make_shared<EntitiesCardEntityItem>("fan", fan));
  fan->set_attribute(ha_attr_type::percentage_step, "25");
  fan->set_attribute(ha_attr_type::percentage, "3e38");
  display.clear();
  panel.frame("event,pageOpenDetail,popupFan,uuid.fan");
  check(!display.empty() && display.front().find("~0~4~") != std::string::npos, "bad fan speed renders safe fallback");
}

static void test_cover_popup() {
  Panel panel;
  const auto cover = panel.create_entity("cover.test");
  const auto page = panel.create_page<EntitiesCard>("cover-page");
  const auto tile = std::make_shared<EntitiesCardEntityItem>("cover", cover);
  page->add_item(tile);
  cover->set_state("opening");
  cover->set_attribute(ha_attr_type::supported_features, "240");
  cover->set_attribute(ha_attr_type::current_tilt_position, "40");
  const std::string tile_payload = tile->render();
  const auto popup_has_tilt = [](const std::string &suffix) {
    return !display.empty() && display.front().find("entityUpdateDetail~uuid.cover~") == 0 &&
           display.front().size() >= suffix.size() &&
           display.front().compare(display.front().size() - suffix.size(), suffix.size(), suffix) == 0;
  };
  display.clear();
  panel.frame("event,pageOpenDetail,popupShutter,uuid.cover");
  check(popup_has_tilt("~enable~enable~enable~40%"), "registered popup initially renders tilt 40%");
  // A stale button UUID can clear the general item lookup cache.
  panel.frame("event,buttonPress2,uuid.missing,tiltSlider,40");
  display.clear();
  panel.on_entity_attribute_update_("cover.test", "current_tilt_position", "100");
  panel.run_timeout("cover.test");
  check(display.size() == 1 && popup_has_tilt("~disable~enable~enable~100%"),
        "tilt attribute refreshes the open popup even after an unrelated UUID lookup");
  check(!tile->get_render_invalid() && tile->render() == tile_payload, "tilt refresh leaves tile cached");

  panel.popup_page_current_uuid_.clear();
  display.clear();
  panel.on_entity_attribute_update_("cover.test", "current_tilt_position", "0");
  panel.run_timeout("cover.test");
  check(display.empty() && !tile->get_render_invalid(), "tilt attribute does not refresh a closed popup or its tile");

  cover->set_attribute(ha_attr_type::supported_features, "48");  // Open/close tilt only, no stop or slider.
  for (const auto &[position, suffix] : {std::pair{"0", "~enable~disable~disable~disable"},
      std::pair{"100", "~disable~disable~enable~disable"}, std::pair{"50", "~enable~disable~enable~disable"}}) {
    cover->set_attribute(ha_attr_type::current_tilt_position, position);
    display.clear();
    panel.frame("event,pageOpenDetail,popupShutter,uuid.cover");
    check(popup_has_tilt(suffix), std::string("arrow-only tilt at ") + position);
  }
  struct Case { const char *features, *position, *slider, *text, *buttons; };
  for (const Case &test : {
      Case{"0", "40", "disable", ": opening", "disable~disable~disable"},
      Case{"1", "40", "disable", "position: 40%", "enable~disable~disable"},
      Case{"2", "40", "disable", "position: 40%", "disable~disable~enable"},
      Case{"8", "40", "disable", "position: 40%", "disable~enable~disable"},
      Case{"11", "40", "disable", "position: 40%", "enable~enable~enable"},
      Case{"128", "40", "disable", ": opening", "disable~disable~disable"},
      Case{"4", "0", "0", "position: 0%", "disable~disable~disable"},
      Case{"15", "100", "100", "position: 100%", "disable~enable~enable"},
      Case{"15", "unavailable", "disable", "position: opening", "enable~enable~enable"},
      Case{"4", "", "disable", "position: opening", "disable~disable~disable"},
      Case{"4", "101", "disable", "position: opening", "disable~disable~disable"},
      Case{"4x", "40", "disable", ": opening", "disable~disable~disable"},
  }) {
    cover->set_attribute(ha_attr_type::supported_features, test.features);
    cover->set_attribute(ha_attr_type::current_position, test.position);
    display.clear();
    panel.frame("event,pageOpenDetail,popupShutter,uuid.cover");
    std::vector<std::string> fields;
    if (!display.empty()) {
      size_t start = 0;
      for (size_t end = display.front().find('~'); end != std::string::npos; end = display.front().find('~', start)) {
        fields.emplace_back(display.front(), start, end - start);
        start = end + 1;
      }
      fields.emplace_back(display.front(), start);
    }
    check(fields.size() == 20 && fields[0] == "entityUpdateDetail" && fields[1] == "uuid.cover" &&
          fields[2] == test.slider && fields[3] == test.text &&
          fields[9] + "~" + fields[10] + "~" + fields[11] == test.buttons,
          std::string("cover popup wire: features=") + test.features + " position='" + test.position + "'");
  }
}

static void test_details() {
  Panel panel;
  auto cover = panel.create_entity("cover.test");
  EntitiesCardEntityItem tile("cover", cover);
  cover->set_state("closed");
  cover->set_attribute(ha_attr_type::current_position, "0");
  cover->set_attribute(ha_attr_type::supported_features, "255");
  for (const char *bad : {"abc", "256", "-1"}) {
    cover->set_attribute(ha_attr_type::current_position, bad);
    check(tile.get_value().find("|enable|enable|disable") != std::string::npos,
          std::string("invalid position retains state-based controls: ") + bad);
    panel.render_cover_detail_update_(&tile);
    const auto &detail = panel.command_buffer_;
    check(std::count(detail.begin(), detail.end(), '~') == 19 &&
          detail.find("~enable~enable~disable~") != std::string::npos,
          std::string("complete state-based cover detail: ") + bad);
  }
  cover->set_state("opening");
  cover->set_attribute(ha_attr_type::current_position, "40");
  cover->set_attribute(ha_attr_type::current_tilt_position, "unavailable");
  panel.render_cover_detail_update_(&tile);
  check(panel.command_buffer_.find("~enable~enable~enable~") != std::string::npos,
        "invalid tilt preserves position controls and stop");
  cover->set_attribute(ha_attr_type::current_position, "unavailable");
  check(tile.get_value().find("|enable|") != std::string::npos, "invalid position preserves stop control");
  cover->set_attribute(ha_attr_type::current_tilt_position, "");
  cover->set_attribute(ha_attr_type::current_position, "");
  cover->set_state("open");
  panel.render_cover_detail_update_(&tile);
  check(panel.command_buffer_.find("~disable~enable~enable~") != std::string::npos,
        "open cover without position can close");
  cover->set_state("closed");
  panel.render_cover_detail_update_(&tile);
  check(panel.command_buffer_.find("~enable~enable~disable~") != std::string::npos,
        "closed cover without position can open");
  cover->set_attribute(ha_attr_type::current_position, "0");
  cover->set_attribute(ha_attr_type::current_position, "101");
  cover->set_attribute(ha_attr_type::current_position, "40");
  check(tile.get_value() != "|||disable|disable|disable", "cover recovers after invalid position");
  cover->set_attribute(ha_attr_type::current_tilt_position, "bad");
  check(tile.get_value() != "|||disable|disable|disable", "tile ignores unrelated invalid tilt");
  auto timer = panel.create_entity("timer.test");
  auto page = panel.create_page<EntitiesCard>("test-page");
  auto timer_item = std::make_shared<EntitiesCardEntityItem>("timer", timer);
  page->add_item(timer_item);
  EntitiesCardEntityItem &popup = *timer_item;
  timer->set_state("idle");
  for (const char *bad : {"0:bad:00", "00:90:01", "2147483647:59:59", "0::00:00", "0:00:60", "0:00:00:"}) {
    timer->set_attribute(ha_attr_type::duration, bad);
    display.clear();
    panel.command_buffer_ = "stale";
    panel.popup_page_current_uuid_ = "timer";
    const bool rendered = panel.render_popup_page_update_(&popup, true);
    check(!rendered && display == std::vector<std::string>{"current page"} &&
          !panel.interval && panel.command_buffer_.empty() && panel.popup_page_current_uuid_.empty(),
          std::string("timer fallback without stale output: ") + bad);
  }
  timer->set_attribute(ha_attr_type::duration, "1:02:03");
  display.clear();
  calls.clear();
  panel.frame("event,pageOpenDetail,popupTimer,uuid.timer");
  check(display.size() >= 2 && display[1].find("~uuid.timer~62~3~") != std::string::npos,
        "popupTimer frame resolves registered item and renders details");
  panel.frame("event,buttonPress2,uuid.timer,timer-start,0:35:40");
  check(calls.size() == 1 && calls[0].action == "timer.start" &&
        calls[0].data == Data{{"entity_id", "timer.test"}, {"duration", "0:35:40"}},
        "timer button resolves registered item and sends exact action");
  display.clear();
  check(panel.render_popup_page_update_(&popup, true) && display.size() == 2 &&
        display.back().find("~uuid.timer~62~3~") != std::string::npos && !panel.interval,
        "idle timer renders duration without periodic redraw");

  display.clear();
  host_millis += 1000;
  panel.loop();
  check(display.empty(), "idle timer leaves TFT inactivity alone");

  esphome::time::RealTimeClock clock{{1735689600}};  // 2025-01-01 00:00:00 UTC
  panel.set_time_id(&clock);
  timer->set_state("active");
  timer->set_attribute(ha_attr_type::finishes_at, "2025-01-01T00:01:05.123+00:00");
  panel.popup_page_current_uuid_ = "timer";
  display.clear();
  check(panel.render_popup_page_update_(&popup, true) && !panel.interval &&
        display.back().find("~uuid.timer~1~5~") != std::string::npos, "active timer renders snapshot without interval");
  timer->set_attribute(ha_attr_type::finishes_at, "2025-01-02T00:00:00Z");
  display.clear();
  check(panel.render_popup_page_update_(&popup, true) && display.back().find("~uuid.timer~1440~0~") != std::string::npos,
        "24-hour active timer retains full duration");
  timer->set_attribute(ha_attr_type::finishes_at, "2025-02-30T00:01:05Z");
  display.clear();
  check(!panel.render_popup_page_update_(&popup, true) && !panel.interval, "invalid calendar date rejected");
  timer->set_attribute(ha_attr_type::finishes_at, "2025-01-01T01:01:05+01:00");
  panel.popup_page_current_uuid_ = "timer";
  display.clear();
  check(panel.render_popup_page_update_(&popup, true) && display.back().find("~uuid.timer~1~5~") != std::string::npos,
        "active timer respects timezone offset");
  ++clock.current.timestamp;
  display.clear();
  panel.on_entity_attribute_update_("timer.test", "finishes_at", "2025-01-01T00:02:00Z");
  panel.run_timeout("timer.test");
  check(display.size() == 1 && display.back().find("~uuid.timer~1~59~") != std::string::npos,
        "HA restart refreshes the open timer with a new snapshot");
  timer->set_attribute(ha_attr_type::finishes_at, "2025-01-01T00:10:00Z");
  const auto opened_at = host_millis;
  display.clear();
  panel.frame("event,pageOpenDetail,popupTimer,uuid.timer");
  check(std::count(display.begin(), display.end(), "timeout~30") == 1 &&
        std::count(display.begin(), display.end(), "timeout~10") == 0, "timer popup uses only its 30-second timeout");
  for (int i = 1; i <= 35; ++i) {
    host_millis = opened_at + i * 1000;
    ++clock.current.timestamp;
    if (panel.interval) { auto refresh = panel.interval; refresh(); }
    panel.loop();
  }
  check(!panel.interval && panel.popup_page_current_uuid_ == "timer" && display.size() == 2,
        "timer takes one snapshot and leaves touch-driven expiry to the TFT");

}

static void test_service_tile() {
  Panel panel;
  auto service_tile = panel.create_entity("service.power_off");
  service_tile->set_ha_service(
      "remote.send_command",
      {{"entity_id", "remote.example_remote"}},
      {{"command", "['standby']"}});
  panel.create_entity("service.unconfigured");
  auto page = panel.create_page<GridCard>("service-page");
  page->add_item(std::make_shared<GridCardEntityItem>("power", service_tile));

  calls.clear();
  panel.frame("event,buttonPress2,service.power_off,button");
  check(calls.size() == 1 && calls.back().action == "remote.send_command" &&
        calls.back().data == Data{{"entity_id", "remote.example_remote"}} &&
        calls.back().data_template == Data{{"command", "['standby']"}},
        "service tile dispatches named action with plain and templated data");

  calls.clear();
  panel.frame("event,buttonPress2,service.unconfigured,button");
  check(calls.empty(), "service tile without configured action is ignored");
}

static void test_long_sleep_timeout() {
  Panel panel;
  const uint64_t start = 10000;
  host_millis = start;

  display.clear();
  panel.set_display_timeout(10);
  check(display == std::vector<std::string>{"timeout~10"}, "timeout <=65 stays under TFT control");

  display.clear();
  panel.set_display_timeout(600);
  check(display == std::vector<std::string>{"timeout~0"}, "long timeout disables the TFT timer");

  display.clear();
  host_millis = start + 599000;
  panel.loop();
  check(display.empty(), "ESP32 timer does not fire before the configured deadline");

  // A touch restarts the idle countdown even when it does not change the page.
  panel.frame("event,buttonPress2,none,button");
  display.clear();
  host_millis = start + 600000;
  panel.loop();
  check(display.empty(), "touch resets the ESP32 idle countdown");

  host_millis = start + 1198999;
  panel.loop();
  check(display.empty(), "reset countdown still holds one millisecond before expiry");
  host_millis = start + 1199000;
  panel.loop();
  check(display == std::vector<std::string>{"timeout~1"}, "expiry hands sleep back to the TFT with a one-second timer");

  panel.loop();
  check(display == std::vector<std::string>{"timeout~1"}, "expiry fires only once");

  // A touch before sleepReached must retain the configured 600 s, not leave the TFT at 1 s.
  display.clear();
  panel.frame("event,buttonPress2,none,button");
  check(display == std::vector<std::string>{"timeout~0"}, "hand-off touch stops the TFT countdown");
  host_millis = start + 1798999;
  panel.loop();
  check(display == std::vector<std::string>{"timeout~0"}, "hand-off touch re-arms the full timeout");
  host_millis = start + 1799000;
  panel.loop();
  check(display.back() == "timeout~1", "re-armed timeout expires 600 s after the touch");

  display.clear();
  panel.set_display_timeout(600);
  panel.frame("event,sleepReached,cardGrid");
  host_millis = start + 2399000;
  panel.loop();
  check(display == std::vector<std::string>{"timeout~0"}, "sleepReached cancels the ESP32 timer");

  // Malformed button frames are not user activity.
  display.clear();
  panel.set_display_timeout(600);
  host_millis = start + 2399000;
  panel.frame("event,buttonPress2,,");
  display.clear();
  host_millis = start + 2999000;
  panel.loop();
  check(display == std::vector<std::string>{"timeout~1"}, "invalid button frame does not reset the timer");

  // A shorter popup timeout cancels the long one.
  display.clear();
  panel.set_display_timeout(600);
  display.clear();
  panel.set_display_timeout(10);
  host_millis = start + 3599000;
  panel.loop();
  check(display == std::vector<std::string>{"timeout~10"}, "popup timeout cancels the long timer");
}

static void test_single_status_icon() {
  const std::string lock_type = "lock";
  const auto lock = std::make_shared<Entity>("lock.front_door", lock_type.c_str());
  const auto right = std::make_shared<StatusIconItem>("status-right", lock);
  lock->set_state("unlocked");

  Screensaver screensaver("screensaver");
  screensaver.set_icon_right(right);
  check(screensaver.should_render_status_update(), "single status icon requests an update");

  std::string buffer;
  screensaver.render_status_update(buffer);
  check(buffer.rfind("statusUpdate~~~", 0) == 0 &&
        buffer.find(CHAR8_CAST(icon_t::lock_open)) != std::string::npos,
        "single right status icon renders with a blank left side");
}

int main() {
  test_uart_recovery();
  test_service_tile();
  test_long_sleep_timeout();
  test_single_status_icon();
  test_wire();
  test_button_validation();
  test_popup_selection();
  test_light_effect_popup();
  test_effect_utf8();
  test_cover_popup();
  test_details();
  test_thermostat_steps();
  test_tile_callbacks();
  test_numeric_render();
  test_uuid();
  return failures != 0;
}
