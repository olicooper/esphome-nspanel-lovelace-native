#!/bin/sh
# Host-side tests. Mirrors the ESP-IDF compile flags (-fno-exceptions -fno-rtti)
# that the crash fixes in the component exist because of.
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo=$(dirname "$(dirname "$here")")

for test in test_helpers test_process_command; do
  echo "--- $test ---"
  out=${TMPDIR:-/tmp}/nspanel-$test
  g++ -std=gnu++17 -Wall -Wextra -fno-exceptions -fno-rtti \
    -I "$here" -I "$repo/components/nspanel_lovelace" \
    "$here/$test.cpp" -o "$out"
  "$out"
done
