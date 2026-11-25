"""Build selected unmodified panel methods without the ESPHome/device runtime."""
import re
import sys
from pathlib import Path

component = Path(sys.argv[1])
out = Path(sys.argv[2])
header = (component / 'nspanel_lovelace.h').read_text()
(out / 'host_panel.h').write_text(re.sub(r'^#include ["<](?:esphome/|driver/)[^\n]*\n', '', header, flags=re.M))
source = (component / 'nspanel_lovelace.cpp').read_text()
methods = '''NSPanelLovelace loop on_entity_attribute_update_ create_entity on_page_item_added_callback set_display_timeout process_data_ process_command_
render_popup_page_ render_popup_page_update_ render_cover_detail_update_ render_timer_detail_update_ render_fan_detail_update_ render_light_detail_update_ render_input_select_detail_update_
try_replace_uuid_with_entity_id_ get_mode_from_index_ get_current_page_item_ process_button_press_ get_page_item_
get_entity_ call_ha_service_ send_buffered_command_'''.split()
# Keep the typed navigation overload stubbed; exercise UUID lookup and page switching.
methods.append(r'render_page_(?=\(const std::string &)')
# Leave the last call_ha_service_ overload (the native API boundary) to the fixture.
start = source.rindex('\nvoid NSPanelLovelace::call_ha_service_', 0, source.index('  api::HomeassistantActionRequest resp;'))
source = source[:start] + source[source.index('\n}', start) + 2:]
pattern = r'^([^\n]*\bNSPanelLovelace::(?:' + '|'.join(methods) + r')\([^\n]*\n.*?^})'
(out / 'host_panel.inc').write_text('\n\n'.join(re.findall(pattern, source, re.M | re.S)))
