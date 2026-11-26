#!/bin/sh
# Host-side tests. Mirrors the ESP-IDF compile flags (-fno-exceptions -fno-rtti)
# that the crash fixes in the component exist because of.
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo=$(dirname "$(dirname "$here")")

out=$(mktemp -d "${TMPDIR:-/tmp}/nspanel-tests.XXXXXX")
trap 'rm -rf "$out"' EXIT
component="$repo/components/nspanel_lovelace"
status=0
# The weather panel tests compare production date keys against localtime under
# a known timezone.
export TZ=UTC
for test in test_helpers test_display_dim test_process_command test_weather_forecast test_weather_panel; do
  echo "--- $test ---"
  want_json=no
  json_flags=""
  if [ "$test" = test_weather_panel ] && [ -n "${ARDUINOJSON_DIR:-}" ]; then
    want_json=yes
    json_flags="-DHOST_ARDUINOJSON -I${ARDUINOJSON_DIR}"
  fi
  python3 "$here/panel_sources.py" "$component" "$out" "$want_json"
  set --
  if [ "$test" = test_display_dim ] || [ "$test" = test_process_command ] || [ "$test" = test_weather_panel ]; then
    for file in config entity page_item_base page_base page_items pages card_base card_items cards page_manager page_item_visitor; do
      set -- "$@" "$component/$file.cpp"
    done
  fi
  # The ArduinoJson-consuming apply_weather_forecast leg runs only when the
  # caller points at the real ArduinoJson 7 include directory (the firmware
  # dependency, e.g. a managed_components/bblanchon__arduinojson/src copy);
  # the plain runners stay dependency-free and cover the non-JSON paths.
  # Do not let string pooling hide type-name pointer comparisons.
  if g++ -std=gnu++17 -O2 -fno-merge-constants -Wall -Wextra -Wno-unused-parameter -fno-exceptions -fno-rtti ${CXXFLAGS:-} ${json_flags} \
    -I "$here" -I "$component" -I "$out" \
    "$here/$test.cpp" "$@" -o "$out/$test"; then
    TZ=UTC "$out/$test" || status=1
  else
    status=1
  fi
done
exit "$status"
