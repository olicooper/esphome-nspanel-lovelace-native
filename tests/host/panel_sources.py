"""Build selected unmodified panel methods without the ESPHome/device runtime."""
import re
import sys
from pathlib import Path

component = Path(sys.argv[1])
out = Path(sys.argv[2])
header = (component / 'nspanel_lovelace.h').read_text()
(out / 'host_panel.h').write_text(re.sub(r'^#include ["<](?:esphome/|driver/|esp_http_client\.h)[^\n]*\n', '', header, flags=re.M))
source = (component / 'nspanel_lovelace.cpp').read_text()
methods = '''NSPanelLovelace loop on_entity_attribute_update_ create_entity on_page_item_added_callback set_display_timeout process_data_ process_command_
render_popup_page_ render_popup_page_update_ render_cover_detail_update_ render_timer_detail_update_ render_fan_detail_update_ render_light_detail_update_ render_input_select_detail_update_
try_replace_uuid_with_entity_id_ get_mode_from_index_ get_current_page_item_ process_button_press_ get_page_item_
get_entity_ call_ha_service_ send_buffered_command_ set_display_dim set_display_active_dim set_display_inactive_dim'''.split()
# Native weather production paths: dispatch/timeout, reply guards, blank
# rendering and expiry. apply_weather_forecast consumes ArduinoJson and is
# only extracted when the caller passes 'yes' as argv[3] (the JSON-gated
# leg in run.sh); the fixture forward-declares JsonObjectConst otherwise.
methods += '''send_weather_update_command_ on_weather_state_update_ on_weather_temperature_update_
 on_weather_temperature_unit_update_ log_weather_forecast_diagnostic_ check_weather_forecast_
 clear_weather_forecast_ handle_weather_forecast_error weather_today_date_key_
 weather_forecast_slots_full_ observe_weather_restart_ha_ weather_stall_restart_ready_ check_weather_forecast_stall_
 publish_weather_slots_ expire_weather_slots_ init_weather_forecast_slots'''.split()
if len(sys.argv) > 3 and sys.argv[3] == 'yes':
    methods.append('apply_weather_forecast')
# Keep the typed navigation overload stubbed; exercise UUID lookup and page switching.
methods.append(r'render_page_(?=\(const std::string &)')
# Leave the last call_ha_service_ overload (the native API boundary) to the fixture.
start = source.rindex('\nvoid NSPanelLovelace::call_ha_service_', 0, source.index('  api::HomeassistantActionRequest resp;'))
source = source[:start] + source[source.index('\n}', start) + 2:]
pattern = r'^([^\n]*\bNSPanelLovelace::(?:' + '|'.join(methods) + r')\([^\n]*\n.*?^})'
extracted = re.findall(pattern, source, re.M | re.S)
# Exercise actual TFT admission, but stop before hardware/network transfer.
upload = (component / 'nspanel_lovelace_upload_idf.cpp').read_text()
upload_start = upload.index('bool NSPanelLovelace::upload_tft(')
upload_end = upload.index('  this->is_updating_ = true;', upload_start)
extracted.append('#ifdef USE_NSPANEL_TFT_UPLOAD\n' + upload[upload_start:upload_end]
                 + '  return true;\n}\n#endif')
# The native-weather timing bounds are file-scope constants in the component;
# extract them verbatim so the host methods use the production values.
constants = re.search(r'#ifdef USE_NSPANEL_NATIVE_WEATHER\n(.*?)\n#endif', source, re.S)
if constants:
    extracted.insert(0, constants.group(1))
(out / 'host_panel.inc').write_text('\n\n'.join(extracted))

# Extract the production daily-forecast slot logic verbatim so the host test
# compiles the same code the firmware uses, not a copied parser.
match = re.search(r'// BEGIN WEATHER SLOT LOGIC\n(.*?)\n// END WEATHER SLOT LOGIC',
                  header, re.S)
(out / 'host_weather.h').write_text(
    '#pragma once\n#include <algorithm>\n#include <array>\n#include <cmath>\n'
    '#include <cstdint>\n#include <cstdio>\n#include <cstring>\n#include <ctime>\n'
    '#include <string>\n#include <vector>\n'
    'namespace esphome::nspanel_lovelace {\n'
    + (match.group(1) if match else '')
    + '\n}  // namespace esphome::nspanel_lovelace\n')
