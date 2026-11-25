// Auto generated code by esphome
// ========== AUTO GENERATED INCLUDE BLOCK BEGIN ===========
#include "esphome.h"
using namespace esphome;
alignas(logger::Logger) static unsigned char logger__logger_logger_id__pstorage[sizeof(logger::Logger)];
static logger::Logger *const logger_logger_id = reinterpret_cast<logger::Logger *>(logger__logger_logger_id__pstorage);
#ifndef __PICOLIBC__
using std::isnan;
#endif
using std::min;
using std::max;
#include <new>
using namespace time;
using namespace text_sensor;
alignas(wifi::WiFiComponent) static unsigned char wifi__wifi_wificomponent_id__pstorage[sizeof(wifi::WiFiComponent)];
static wifi::WiFiComponent *const wifi_wificomponent_id = reinterpret_cast<wifi::WiFiComponent *>(wifi__wifi_wificomponent_id__pstorage);
alignas(mdns::MDNSComponent) static unsigned char mdns__mdns_mdnscomponent_id__pstorage[sizeof(mdns::MDNSComponent)];
static mdns::MDNSComponent *const mdns_mdnscomponent_id = reinterpret_cast<mdns::MDNSComponent *>(mdns__mdns_mdnscomponent_id__pstorage);
alignas(network::NetworkComponent) static unsigned char network__network_networkcomponent_id__pstorage[sizeof(network::NetworkComponent)];
static network::NetworkComponent *const network_networkcomponent_id = reinterpret_cast<network::NetworkComponent *>(network__network_networkcomponent_id__pstorage);
alignas(preferences::IntervalSyncer) static unsigned char preferences__preferences_intervalsyncer_id__pstorage[sizeof(preferences::IntervalSyncer)];
static preferences::IntervalSyncer *const preferences_intervalsyncer_id = reinterpret_cast<preferences::IntervalSyncer *>(preferences__preferences_intervalsyncer_id__pstorage);
alignas(api::APIServer) static unsigned char api__api_apiserver_id__pstorage[sizeof(api::APIServer)];
static api::APIServer *const api_apiserver_id = reinterpret_cast<api::APIServer *>(api__api_apiserver_id__pstorage);
alignas(api::UserServiceTrigger<api::enums::SUPPORTS_RESPONSE_NONE, StringRef>) static unsigned char api__api_userservicetrigger_id__pstorage[sizeof(api::UserServiceTrigger<api::enums::SUPPORTS_RESPONSE_NONE, StringRef>)];
static api::UserServiceTrigger<api::enums::SUPPORTS_RESPONSE_NONE, StringRef> *const api_userservicetrigger_id = reinterpret_cast<api::UserServiceTrigger<api::enums::SUPPORTS_RESPONSE_NONE, StringRef> *>(api__api_userservicetrigger_id__pstorage);
using namespace json;
alignas(nspanel_lovelace::NSPanelLovelace) static unsigned char nspanel_lovelace__nspanel__pstorage[sizeof(nspanel_lovelace::NSPanelLovelace)];
static nspanel_lovelace::NSPanelLovelace *const nspanel = reinterpret_cast<nspanel_lovelace::NSPanelLovelace *>(nspanel_lovelace__nspanel__pstorage);
alignas(Automation<StringRef>) static unsigned char esphome__automation_id__pstorage[sizeof(Automation<StringRef>)];
static Automation<StringRef> *const automation_id = reinterpret_cast<Automation<StringRef> *>(esphome__automation_id__pstorage);
alignas(StatelessLambdaAction<StringRef>) static unsigned char esphome__lambdaaction_id__pstorage[sizeof(StatelessLambdaAction<StringRef>)];
static StatelessLambdaAction<StringRef> *const lambdaaction_id = reinterpret_cast<StatelessLambdaAction<StringRef> *>(esphome__lambdaaction_id__pstorage);
using namespace api;
using namespace uart;
alignas(uart::IDFUARTComponent) static unsigned char uart__uart_idfuartcomponent_id__pstorage[sizeof(uart::IDFUARTComponent)];
static uart::IDFUARTComponent *const uart_idfuartcomponent_id = reinterpret_cast<uart::IDFUARTComponent *>(uart__uart_idfuartcomponent_id__pstorage);
alignas(esp32::ESP32InternalGPIOPin) static unsigned char esp32__esp32_esp32internalgpiopin_id__pstorage[sizeof(esp32::ESP32InternalGPIOPin)];
static esp32::ESP32InternalGPIOPin *const esp32_esp32internalgpiopin_id = reinterpret_cast<esp32::ESP32InternalGPIOPin *>(esp32__esp32_esp32internalgpiopin_id__pstorage);
alignas(esp32::ESP32InternalGPIOPin) static unsigned char esp32__esp32_esp32internalgpiopin_id_2__pstorage[sizeof(esp32::ESP32InternalGPIOPin)];
static esp32::ESP32InternalGPIOPin *const esp32_esp32internalgpiopin_id_2 = reinterpret_cast<esp32::ESP32InternalGPIOPin *>(esp32__esp32_esp32internalgpiopin_id_2__pstorage);
alignas(homeassistant::HomeassistantTime) static unsigned char homeassistant__homeassistant_time__pstorage[sizeof(homeassistant::HomeassistantTime)];
static homeassistant::HomeassistantTime *const homeassistant_time = reinterpret_cast<homeassistant::HomeassistantTime *>(homeassistant__homeassistant_time__pstorage);
using namespace nspanel_lovelace;
constexpr FrozenCharMap<const char *, TRANSLATION_MAP_SIZE> esphome::nspanel_lovelace::TRANSLATION_MAP {{
  {nspanel_lovelace::translation_item::none, "none_codegen_custom"},
  {nspanel_lovelace::translation_item::unknown, "Unknown"},
  {nspanel_lovelace::translation_item::preset_mode, "Preset mode"},
  {nspanel_lovelace::translation_item::swing_mode, "Swing mode"},
  {nspanel_lovelace::translation_item::fan_mode, "Fan mode"},
  {nspanel_lovelace::translation_item::activity, "Activity"},
  {nspanel_lovelace::translation_item::away, "Away"},
  {nspanel_lovelace::translation_item::boost, "Boost"},
  {nspanel_lovelace::translation_item::comfort, "Comfort"},
  {nspanel_lovelace::translation_item::eco, "Eco"},
  {nspanel_lovelace::translation_item::home, "Home"},
  {nspanel_lovelace::translation_item::sleep_, "Sleep"},
  {nspanel_lovelace::translation_item::cool, "Cool"},
  {nspanel_lovelace::translation_item::cooling, "Cooling"},
  {nspanel_lovelace::translation_item::dry, "Dry"},
  {nspanel_lovelace::translation_item::drying, "Drying"},
  {nspanel_lovelace::translation_item::fan, "Fan"},
  {nspanel_lovelace::translation_item::heat, "Heat"},
  {nspanel_lovelace::translation_item::heating, "Heating"},
  {nspanel_lovelace::translation_item::heat_cool, "Heat/Cool"},
  {nspanel_lovelace::translation_item::idle, "Idle"},
  {nspanel_lovelace::translation_item::auto_, "Auto"},
  {nspanel_lovelace::translation_item::fan_only, "Fan only"},
  {nspanel_lovelace::translation_item::on, "On"},
  {nspanel_lovelace::translation_item::off, "Off"},
  {nspanel_lovelace::translation_item::currently, "Currently"},
  {nspanel_lovelace::translation_item::state, "State"},
  {nspanel_lovelace::translation_item::action, "Action"},
  {nspanel_lovelace::translation_item::lock, "Lock"},
  {nspanel_lovelace::translation_item::unlock, "Unlock"},
  {nspanel_lovelace::translation_item::paused, "Paused"},
  {nspanel_lovelace::translation_item::active, "Active"},
  {nspanel_lovelace::translation_item::activate, "Activate"},
  {nspanel_lovelace::translation_item::press, "Press"},
  {nspanel_lovelace::translation_item::run, "Run"},
  {nspanel_lovelace::translation_item::speed, "Speed"},
  {nspanel_lovelace::translation_item::brightness, "Brightness"},
  {nspanel_lovelace::translation_item::color, "Color"},
  {nspanel_lovelace::translation_item::color_temp, "Color temperature"},
  {nspanel_lovelace::translation_item::above_horizon, "Above Horizon"},
  {nspanel_lovelace::translation_item::below_horizon, "Below Horizon"},
  {nspanel_lovelace::translation_item::position, "Position"},
  {nspanel_lovelace::translation_item::tilt_position, "Tilt position"},
  {nspanel_lovelace::translation_item::start, "Start"},
  {nspanel_lovelace::translation_item::pause_, "Pause"},
  {nspanel_lovelace::translation_item::cancel, "Cancel"},
  {nspanel_lovelace::translation_item::finish, "Finish"},
  {nspanel_lovelace::translation_item::arm_home, "Arm home"},
  {nspanel_lovelace::translation_item::arm_away, "Arm away"},
  {nspanel_lovelace::translation_item::arm_night, "Arm night"},
  {nspanel_lovelace::translation_item::arm_vacation, "Arm vacation"},
  {nspanel_lovelace::translation_item::arm_custom_bypass, "Arm bypass"},
  {nspanel_lovelace::translation_item::disarm, "Disarm"},
  {nspanel_lovelace::translation_item::disarming, "Disarm"},
  {nspanel_lovelace::translation_item::disarmed, "Disarm"},
  {nspanel_lovelace::translation_item::arming, "Arming"},
  {nspanel_lovelace::translation_item::pending, "Pend"},
  {nspanel_lovelace::translation_item::triggered, "Trig"},
  {nspanel_lovelace::translation_item::armed_away, "Armed"},
  {nspanel_lovelace::translation_item::armed_home, "Armed"},
  {nspanel_lovelace::translation_item::armed_night, "Armed"},
  {nspanel_lovelace::translation_item::armed_vacation, "Armed"},
  {nspanel_lovelace::translation_item::armed_custom_bypass, "Armed"},
  {nspanel_lovelace::translation_item::not_home, "Away"},
  {nspanel_lovelace::translation_item::start_cleaning, "Start cleaning"},
  {nspanel_lovelace::translation_item::return_to_base, "Return to dock"},
  {nspanel_lovelace::translation_item::docked, "Docked"},
  {nspanel_lovelace::translation_item::turn_on, "Turn on"},
  {nspanel_lovelace::translation_item::turn_off, "Turn off"},
  {nspanel_lovelace::translation_item::month_january, "January"},
  {nspanel_lovelace::translation_item::month_jan, "Jan"},
  {nspanel_lovelace::translation_item::month_february, "February"},
  {nspanel_lovelace::translation_item::month_feb, "Feb"},
  {nspanel_lovelace::translation_item::month_march, "March"},
  {nspanel_lovelace::translation_item::month_mar, "Mar"},
  {nspanel_lovelace::translation_item::month_april, "April"},
  {nspanel_lovelace::translation_item::month_apr, "Apr"},
  {nspanel_lovelace::translation_item::month_may, "May"},
  {nspanel_lovelace::translation_item::month_june, "June"},
  {nspanel_lovelace::translation_item::month_jun, "Jun"},
  {nspanel_lovelace::translation_item::month_july, "July"},
  {nspanel_lovelace::translation_item::month_jul, "Jul"},
  {nspanel_lovelace::translation_item::month_august, "August"},
  {nspanel_lovelace::translation_item::month_aug, "Aug"},
  {nspanel_lovelace::translation_item::month_september, "September"},
  {nspanel_lovelace::translation_item::month_sep, "Sep"},
  {nspanel_lovelace::translation_item::month_october, "October"},
  {nspanel_lovelace::translation_item::month_oct, "Oct"},
  {nspanel_lovelace::translation_item::month_november, "November"},
  {nspanel_lovelace::translation_item::month_nov, "Nov"},
  {nspanel_lovelace::translation_item::month_december, "December"},
  {nspanel_lovelace::translation_item::month_dec, "Dec"},
  {nspanel_lovelace::translation_item::dow_sunday, "Sunday"},
  {nspanel_lovelace::translation_item::dow_sun, "Sun"},
  {nspanel_lovelace::translation_item::dow_monday, "Monday"},
  {nspanel_lovelace::translation_item::dow_mon, "Mon"},
  {nspanel_lovelace::translation_item::dow_tuesday, "Tuesday"},
  {nspanel_lovelace::translation_item::dow_tue, "Tue"},
  {nspanel_lovelace::translation_item::dow_wednesday, "Wednesday"},
  {nspanel_lovelace::translation_item::dow_wed, "Wed"},
  {nspanel_lovelace::translation_item::dow_thursday, "Thursday"},
  {nspanel_lovelace::translation_item::dow_thu, "Thu"},
  {nspanel_lovelace::translation_item::dow_friday, "Friday"},
  {nspanel_lovelace::translation_item::dow_fri, "Fri"},
  {nspanel_lovelace::translation_item::dow_saturday, "Saturday"},
  {nspanel_lovelace::translation_item::dow_sat, "Sat"},
  {"codegen_extra", "codegen_extra_value"},
}};
constexpr std::array<const esphome::nspanel_lovelace::icon_char_t*, CUSTOM_ICONS_SIZE> CUSTOM_ICONS {{u8"\uED7E"}};
#include <tuple>
static auto esphome_controllers() { return std::tuple{api_apiserver_id}; }
#include "esphome/core/controller_dispatch.h"
static constexpr size_t ESPHOME_LOOPING_COMPONENT_COUNT = \
  (1 * HasLoopOverride<logger::Logger>::value) + \
  (1 * HasLoopOverride<wifi::WiFiComponent>::value) + \
  (1 * HasLoopOverride<mdns::MDNSComponent>::value) + \
  (1 * HasLoopOverride<network::NetworkComponent>::value) + \
  (1 * HasLoopOverride<preferences::IntervalSyncer>::value) + \
  (1 * HasLoopOverride<api::APIServer>::value) + \
  (1 * HasLoopOverride<nspanel_lovelace::NSPanelLovelace>::value) + \
  (1 * HasLoopOverride<uart::IDFUARTComponent>::value) + \
  (1 * HasLoopOverride<homeassistant::HomeassistantTime>::value);
namespace esphome {
static const char COMP_SRC_TABLE_STR_0[] PROGMEM = "logger";
static const char COMP_SRC_TABLE_STR_1[] PROGMEM = "wifi";
static const char COMP_SRC_TABLE_STR_2[] PROGMEM = "mdns";
static const char COMP_SRC_TABLE_STR_3[] PROGMEM = "network";
static const char COMP_SRC_TABLE_STR_4[] PROGMEM = "preferences";
static const char COMP_SRC_TABLE_STR_5[] PROGMEM = "api";
static const char COMP_SRC_TABLE_STR_6[] PROGMEM = "nspanel_lovelace";
static const char COMP_SRC_TABLE_STR_7[] PROGMEM = "uart";
static const char COMP_SRC_TABLE_STR_8[] PROGMEM = "homeassistant.time";
static const char *const COMP_SRC_TABLE[] PROGMEM = {COMP_SRC_TABLE_STR_0, COMP_SRC_TABLE_STR_1, COMP_SRC_TABLE_STR_2, COMP_SRC_TABLE_STR_3, COMP_SRC_TABLE_STR_4, COMP_SRC_TABLE_STR_5, COMP_SRC_TABLE_STR_6, COMP_SRC_TABLE_STR_7, COMP_SRC_TABLE_STR_8};
const LogString *component_source_lookup(uint8_t index) {
  if (index == 0 || index > 9) return LOG_STR("<unknown>");
  return reinterpret_cast<const LogString *>(
    progmem_read_ptr(&COMP_SRC_TABLE[index - 1]));
}
}  // namespace esphome
// ========== AUTO GENERATED INCLUDE BLOCK END ==========="

void setup() {
  // ========== AUTO GENERATED CODE BEGIN ===========
  // logger:
  //   baud_rate: 115200
  //   boot_log_buffer_size: 0
  //   boot_log_level: WARN
  //   boot_logs: {}
  //   deassert_rts_dtr: false
  //   hardware_uart: UART0
  //   id: logger_logger_id
  //   level: DEBUG
  //   logs: {}
  //   runtime_tag_levels: false
  //   task_log_buffer_size: 768
  //   tx_buffer_size: 512
  new(logger_logger_id) logger::Logger(115200);
  logger_logger_id->create_pthread_key();
  logger_logger_id->pre_setup();
  logger_logger_id->set_log_level(ESPHOME_LOG_LEVEL_DEBUG);
  // network:
  //   enable_ipv4: true
  //   enable_ipv6: false
  //   id: network_networkcomponent_id
  //   min_ipv6_addr_count: 0
  // esphome:
  //   areas: []
  //   build_flags: []
  //   build_path: ./build/ci-codegen
  //   comment: CI codegen fixture
  //   debug_scheduler: false
  //   devices: []
  //   environment_variables: {}
  //   friendly_name: ''
  //   includes: []
  //   includes_c: []
  //   libraries: []
  //   merge_warnings: true
  //   min_version: 2026.11.0
  //   name: nspanel-codegen
  //   name_add_mac_suffix: false
  //   platformio_options: {}
  //   suspend_loop: false
  new (&App) Application;
  App.pre_setup("nspanel-codegen", 15, "", 0);
  App.looping_components_.init(ESPHOME_LOOPING_COMPONENT_COUNT);
  // time:
  // text_sensor:
  App.register_component_(logger_logger_id, 1);
  // wifi:
  //   domain: .local
  //   enable_btm: false
  //   enable_on_boot: true
  //   enable_rrm: false
  //   fast_connect:
  //     enabled: false
  //     storage: flash
  //   id: wifi_wificomponent_id
  //   lease_cache:
  //     enabled: false
  //   min_auth_mode: WPA2
  //   networks:
  //     - id: wifi_wifiap_id
  //       password: \033[8mCODEGEN_PASSWORD\033[28m
  //       priority: 0
  //       ssid: \033[8mCODEGEN_SSID\033[28m
  //   passive_scan: false
  //   post_connect_roaming: true
  //   power_save_mode: LIGHT
  //   reboot_timeout: 15min
  //   use_address: nspanel-codegen.local
  new(wifi_wificomponent_id) wifi::WiFiComponent();
  wifi_wificomponent_id->init_sta(1);
  {
  wifi::WiFiAP wifi_wifiap_id = wifi::WiFiAP();
  wifi_wifiap_id.set_ssid("CODEGEN_SSID");
  wifi_wifiap_id.set_password("CODEGEN_PASSWORD");
  wifi_wificomponent_id->add_sta(wifi_wifiap_id);
  }
  wifi_wificomponent_id->set_power_save_mode(wifi::WIFI_POWER_SAVE_LIGHT);
  App.register_component_(wifi_wificomponent_id, 2);
  // mdns:
  //   disabled: false
  //   id: mdns_mdnscomponent_id
  //   services: []
  new(mdns_mdnscomponent_id) mdns::MDNSComponent();
  App.register_component_(mdns_mdnscomponent_id, 3);
  new(network_networkcomponent_id) network::NetworkComponent();
  App.register_component_(network_networkcomponent_id, 4);
  // preferences:
  //   flash_write_interval: 60s
  //   id: preferences_intervalsyncer_id
  new(preferences_intervalsyncer_id) preferences::IntervalSyncer();
  preferences_intervalsyncer_id->set_update_interval(60000);
  App.register_component_(preferences_intervalsyncer_id, 5);
  // api:
  //   actions:
  //     - action: upload_tft
  //       automation_id: automation_id
  //       supports_response: none
  //       then:
  //         - lambda: !lambda |-
  //             id(nspanel).upload_tft(url);
  //           type_id: lambdaaction_id
  //       trigger_id: api_userservicetrigger_id
  //       variables:
  //         url:
  //           type: string
  //   batch_delay: 100ms
  //   custom_services: false
  //   homeassistant_services: false
  //   homeassistant_states: false
  //   id: api_apiserver_id
  //   listen_backlog: 4
  //   max_connections: 5
  //   max_send_queue: 8
  //   port: 6053
  //   reboot_timeout: 15min
  new(api_apiserver_id) api::APIServer();
  App.register_component_(api_apiserver_id, 6);
  static constexpr const char * api_action0_strings[] PROGMEM = {"upload_tft", "url"};
  new(api_userservicetrigger_id) api::UserServiceTrigger<api::enums::SUPPORTS_RESPONSE_NONE, StringRef>(api_action0_strings, 1889900489);
  // json:
  //   {}
  // esp32:
  //   board: esp32dev
  //   cpu_frequency: 240MHZ
  //   flash_size: 4MB
  //   framework:
  //     advanced:
  //       adc_oneshot_in_iram: false
  //       compiler_optimization: SIZE
  //       disable_debug_stubs: true
  //       disable_dev_null_vfs: true
  //       disable_fatfs: true
  //       disable_libc_locks_in_iram: true
  //       disable_mbedtls_peer_cert: true
  //       disable_mbedtls_pkcs7: true
  //       disable_mbedtls_tls: true
  //       disable_mbedtls_tls_extras: true
  //       disable_mbedtls_tls_server: true
  //       disable_ocd_aware: true
  //       disable_regi2c_in_iram: true
  //       disable_usb_serial_jtag_secondary: true
  //       disable_vfs_support_dir: true
  //       disable_vfs_support_select: true
  //       disable_vfs_support_termios: true
  //       enable_full_printf: false
  //       enable_idf_experimental_features: false
  //       enable_lwip_assert: true
  //       enable_lwip_bridge_interface: false
  //       enable_lwip_check_thread_safety: true
  //       enable_lwip_mdns_queries: true
  //       enable_lwip_tcpip_core_locking: true
  //       enable_ota_downgrade_protection: false
  //       enable_ota_rollback: false
  //       execute_from_psram: false
  //       freertos_in_iram: false
  //       heap_in_iram: false
  //       ignore_efuse_custom_mac: false
  //       ignore_efuse_mac_crc: false
  //       include_builtin_idf_components: []
  //       loop_task_stack_size: 8192
  //       ringbuf_in_iram: false
  //       sram1_as_iram: false
  //       use_full_certificate_bundle: false
  //     components: []
  //     log_level: ERROR
  //     sdkconfig_options: {}
  //     type: esp-idf
  //     version: 5.5.6
  //   variant: ESP32
  //   watchdog_timeout: 5s
  // external_components:
  //   - components:
  //       - nspanel_lovelace
  //     refresh: 1d
  //     source:
  //       path: <COMPONENTS>
  //       type: local
  // nspanel_lovelace:
  //   cards:
  //     - entities:
  //         - entity_id: light.kitchen
  //           icon: hiking
  //         - entity_id: navigate.hidden_card
  //         - entity_id: delete
  //         - entity_id: iText.Hello
  //       hidden: false
  //       id: main_card
  //       sleep_timeout: 65
  //       title: Main
  //       type: cardGrid
  //     - entities:
  //         - entity_id: sensor.demo_hidden
  //       hidden: true
  //       id: hidden_card
  //       sleep_timeout: 10
  //       type: cardGrid
  //     - entities:
  //         - entity_id: light.kitchen
  //           icon: hiking
  //           name: Kitchen "Main" \ west
  //       hidden: false
  //       id: last_card
  //       sleep_timeout: 10
  //       title: Last
  //       type: cardGrid
  //   experimental_vertical_ui: false
  //   id: nspanel
  //   locale:
  //     language: fixtures/translation.json
  //   model: eu
  //   screensaver:
  //     date_format: '%A, %d. %B %Y'
  //     double_tap_to_unlock: false
  //     status_icon_left:
  //       alt_font: true
  //       entity_id: light.demo_left
  //       icon:
  //         color: 0
  //         value: hiking
  //     status_icon_right:
  //       entity_id: binary_sensor.demo_right
  //       icon: alert-circle-outline
  //     time_format: '%H:%M'
  //     time_id: homeassistant_time
  //     weather:
  //       entity_id: weather.demo_codegen
  //       forecast_method: service
  //   sleep_timeout: 10
  //   uart_id: uart_idfuartcomponent_id
  new(nspanel) nspanel_lovelace::NSPanelLovelace();
  App.register_component_(nspanel, 7);
  new(automation_id) Automation<StringRef>(api_userservicetrigger_id);
  new(lambdaaction_id) StatelessLambdaAction<StringRef>([](StringRef url) -> void {
      nspanel->upload_tft(url);
  });
  automation_id->add_action(lambdaaction_id);
  api_apiserver_id->initialize_user_services({api_userservicetrigger_id});
  // uart:
  //   baud_rate: 115200
  //   data_bits: 8
  //   id: uart_idfuartcomponent_id
  //   parity: NONE
  //   rx_buffer_size: 256
  //   rx_pin:
  //     drive_strength: 20.0
  //     hold_state: false
  //     id: esp32_esp32internalgpiopin_id_2
  //     ignore_pin_validation_error: false
  //     ignore_strapping_warning: false
  //     inverted: false
  //     mode:
  //       input: true
  //       open_drain: false
  //       output: false
  //       pulldown: false
  //       pullup: false
  //     number: 17
  //   rx_timeout: 2
  //   stop_bits: 1
  //   tx_pin:
  //     drive_strength: 20.0
  //     hold_state: false
  //     id: esp32_esp32internalgpiopin_id
  //     ignore_pin_validation_error: false
  //     ignore_strapping_warning: false
  //     inverted: false
  //     mode:
  //       input: false
  //       open_drain: false
  //       output: true
  //       pulldown: false
  //       pullup: false
  //     number: 16
  new(uart_idfuartcomponent_id) uart::IDFUARTComponent();
  App.register_component_(uart_idfuartcomponent_id, 8);
  uart_idfuartcomponent_id->set_baud_rate(115200);
  new(esp32_esp32internalgpiopin_id) esp32::ESP32InternalGPIOPin();
  esp32_esp32internalgpiopin_id->set_pin(::GPIO_NUM_16);
  esp32_esp32internalgpiopin_id->set_drive_strength(::GPIO_DRIVE_CAP_2);
  esp32_esp32internalgpiopin_id->set_flags(gpio::Flags::FLAG_OUTPUT);
  uart_idfuartcomponent_id->set_tx_pin(esp32_esp32internalgpiopin_id);
  new(esp32_esp32internalgpiopin_id_2) esp32::ESP32InternalGPIOPin();
  esp32_esp32internalgpiopin_id_2->set_pin(::GPIO_NUM_17);
  esp32_esp32internalgpiopin_id_2->set_drive_strength(::GPIO_DRIVE_CAP_2);
  esp32_esp32internalgpiopin_id_2->set_flags(gpio::Flags::FLAG_INPUT);
  uart_idfuartcomponent_id->set_rx_pin(esp32_esp32internalgpiopin_id_2);
  uart_idfuartcomponent_id->set_rx_buffer_size(256);
  uart_idfuartcomponent_id->set_rx_full_threshold(114);
  uart_idfuartcomponent_id->set_rx_timeout(2);
  uart_idfuartcomponent_id->set_stop_bits(1);
  uart_idfuartcomponent_id->set_data_bits(8);
  // time.homeassistant:
  //   id: homeassistant_time
  //   platform: homeassistant
  //   timezone: EST5EDT,M3.2.0,M11.1.0
  //   update_interval: 15min
  new(homeassistant_time) homeassistant::HomeassistantTime();
  {
  time::ParsedTimezone tz{};
  tz.std_offset_seconds = 18000;
  tz.dst_offset_seconds = 14400;
  tz.dst_start.time_seconds = 7200;
  tz.dst_start.day = 0;
  tz.dst_start.type = time::DSTRuleType::MONTH_WEEK_DAY;
  tz.dst_start.month = 3;
  tz.dst_start.week = 2;
  tz.dst_start.day_of_week = 0;
  tz.dst_end.time_seconds = 7200;
  tz.dst_end.day = 0;
  tz.dst_end.type = time::DSTRuleType::MONTH_WEEK_DAY;
  tz.dst_end.month = 11;
  tz.dst_end.week = 1;
  tz.dst_end.day_of_week = 0;
  time::set_global_tz(tz);
  }
  homeassistant_time->set_update_interval(900000);
  App.register_component_(homeassistant_time, 9);
  // socket:
  //   implementation: bsd_sockets
  nspanel->set_uart_parent(uart_idfuartcomponent_id);
  nspanel->set_display_timeout(10);
  nspanel->set_global_vertical_ui(false);
  nspanel->set_language("unknown");
  auto nspanel_e0 = nspanel->create_entity("light.kitchen");
  auto nspanel_e1 = nspanel->create_entity("navigate.hidden_card");
  auto nspanel_e2 = nspanel->create_entity("iText.Hello");
  auto nspanel_e3 = nspanel->create_entity("sensor.demo_hidden");
  auto nspanel_e4 = nspanel->create_entity("light.demo_left");
  auto nspanel_e5 = nspanel->create_entity("binary_sensor.demo_right");
  {
  nspanel->set_time_id(homeassistant_time);
  nspanel->set_date_format("%A, %d. %B %Y");
  nspanel->set_time_format("%H:%M");
  auto nspanel_screensaver = nspanel->create_screensaver("1");
  auto nspanel_screensaver_iconleft = std::make_shared<nspanel_lovelace::StatusIconItem>("2", nspanel_e4, CUSTOM_ICONS[0], 0u);
  nspanel_screensaver_iconleft->set_icon_value(CUSTOM_ICONS[0]);
  nspanel_screensaver_iconleft->set_icon_color(0);
  nspanel_screensaver_iconleft->set_alt_font(true);
  nspanel_screensaver->set_icon_left(nspanel_screensaver_iconleft);
  auto nspanel_screensaver_iconright = std::make_shared<nspanel_lovelace::StatusIconItem>("3", nspanel_e5, nspanel_lovelace::icon_t::alert_circle_outline);
  nspanel_screensaver_iconright->set_icon_value(nspanel_lovelace::icon_t::alert_circle_outline);
  nspanel_screensaver->set_icon_right(nspanel_screensaver_iconright);
  nspanel->set_weather_entity_id("weather.demo_codegen");
  nspanel_screensaver->add_item_range({std::make_shared<nspanel_lovelace::WeatherItem>("4"), std::make_shared<nspanel_lovelace::WeatherItem>("5"), std::make_shared<nspanel_lovelace::WeatherItem>("6"), std::make_shared<nspanel_lovelace::WeatherItem>("7"), std::make_shared<nspanel_lovelace::WeatherItem>("8")});
  }
  {
  auto nspanel_card_1 = nspanel->create_page<nspanel_lovelace::GridCard>("main_card", "Main", 65);
  auto nspanel_card_1_navleft = std::unique_ptr<nspanel_lovelace::NavigationItem>(new nspanel_lovelace::NavigationItem("9", "last_card", nspanel_lovelace::icon_t::arrow_left_bold));
  nspanel_card_1->set_nav_left(nspanel_card_1_navleft);
  auto nspanel_card_1_navright = std::unique_ptr<nspanel_lovelace::NavigationItem>(new nspanel_lovelace::NavigationItem("10", "last_card", nspanel_lovelace::icon_t::arrow_right_bold));
  nspanel_card_1->set_nav_right(nspanel_card_1_navright);
  auto nspanel_card_1_item_1 = std::make_shared<nspanel_lovelace::GridCardEntityItem>("11", nspanel_e0);
  nspanel_card_1_item_1->set_icon_value(CUSTOM_ICONS[0]);
  nspanel_card_1->add_item(nspanel_card_1_item_1);
  auto nspanel_card_1_item_2 = std::make_shared<nspanel_lovelace::GridCardEntityItem>("12", nspanel_e1);
  nspanel_card_1->add_item(nspanel_card_1_item_2);
  auto nspanel_card_1_item_3 = std::make_shared<nspanel_lovelace::DeleteItem>(nspanel_lovelace::page_type::cardGrid);
  nspanel_card_1->add_item(nspanel_card_1_item_3);
  auto nspanel_card_1_item_4 = std::make_shared<nspanel_lovelace::GridCardEntityItem>("13", nspanel_e2);
  nspanel_card_1->add_item(nspanel_card_1_item_4);
  }
  {
  auto nspanel_card_2 = nspanel->create_page<nspanel_lovelace::GridCard>("hidden_card", "", 10);
  nspanel_card_2->set_hidden(true);
  auto nspanel_card_2_navhome = std::unique_ptr<nspanel_lovelace::NavigationItem>(new nspanel_lovelace::NavigationItem("14", "1", nspanel_lovelace::icon_t::home));
  nspanel_card_2->set_nav_left(nspanel_card_2_navhome);
  auto nspanel_card_2_item_1 = std::make_shared<nspanel_lovelace::GridCardEntityItem>("15", nspanel_e3);
  nspanel_card_2->add_item(nspanel_card_2_item_1);
  }
  {
  auto nspanel_card_3 = nspanel->create_page<nspanel_lovelace::GridCard>("last_card", "Last", 10);
  auto nspanel_card_3_navleft = std::unique_ptr<nspanel_lovelace::NavigationItem>(new nspanel_lovelace::NavigationItem("16", "main_card", nspanel_lovelace::icon_t::arrow_left_bold));
  nspanel_card_3->set_nav_left(nspanel_card_3_navleft);
  auto nspanel_card_3_navright = std::unique_ptr<nspanel_lovelace::NavigationItem>(new nspanel_lovelace::NavigationItem("17", "main_card", nspanel_lovelace::icon_t::arrow_right_bold));
  nspanel_card_3->set_nav_right(nspanel_card_3_navright);
  auto nspanel_card_3_item_1 = std::make_shared<nspanel_lovelace::GridCardEntityItem>("18", nspanel_e0, "Kitchen \042Main\042 \134 west");
  nspanel_card_3_item_1->set_icon_value(CUSTOM_ICONS[0]);
  nspanel_card_3->add_item(nspanel_card_3_item_1);
  }
  // =========== AUTO GENERATED CODE END ============
  App.setup();
}

void loop() {
  App.loop();
}
