#!/bin/sh
# Host-side helper tests. Mirrors the ESP-IDF compile flags (-fno-exceptions
# -fno-rtti) that the crash fixes in helpers.h exist because of.
set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo=$(dirname "$(dirname "$here")")
out=${TMPDIR:-/tmp}/nspanel-host-tests

g++ -std=gnu++17 -Wall -Wextra -fno-exceptions -fno-rtti \
  -I "$here" -I "$repo/components/nspanel_lovelace" \
  "$here/test_helpers.cpp" -o "$out"

"$out"
