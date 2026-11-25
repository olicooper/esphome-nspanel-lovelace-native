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
for test in test_helpers test_process_command; do
  echo "--- $test ---"
  set --
  if [ "$test" = test_process_command ]; then
    python3 "$here/panel_sources.py" "$component" "$out"
    for file in config entity page_item_base page_base page_items card_base card_items cards page_manager page_item_visitor; do
      set -- "$@" "$component/$file.cpp"
    done
  fi
  # Do not let string pooling hide type-name pointer comparisons.
  if g++ -std=gnu++17 -O2 -fno-merge-constants -Wall -Wextra -Wno-unused-parameter -fno-exceptions -fno-rtti ${CXXFLAGS:-} \
    -I "$here" -I "$component" -I "$out" \
    "$here/$test.cpp" "$@" -o "$out/$test"; then
    TZ=UTC "$out/$test" || status=1
  else
    status=1
  fi
done
exit "$status"
