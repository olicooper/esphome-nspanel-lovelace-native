// Host-side unit tests for the pure string/number helpers in helpers.h.
//
// helpers.h only needs the ESP-IDF heap_caps entry points (see esp_heap_caps.h
// next to this file), so it compiles on the host and these run without a
// device. Build and run with ./run.sh.
//
// They target the upstream PR #78 crash fixes: split_str item handling and the
// non-throwing str_to_* / value_or_default parsers. The ESPHome-API-dependent
// half of that PR (Entity::get_attribute reference lifetimes, the StringRef
// callback signatures) is covered only by the ESP-IDF build, not here.

#include "helpers.h"

#include <climits>
#include <cstdio>
#include <string>
#include <vector>

using namespace esphome::nspanel_lovelace;

static int failures = 0;

static void check(bool ok, const char *what) {
  if (!ok) {
    printf("FAIL  %s\n", what);
    ++failures;
  } else {
    printf("ok    %s\n", what);
  }
}

static std::vector<std::string> split(const std::string &s, char delim,
                                      uint16_t max_items = UINT16_MAX) {
  std::vector<std::string> out;
  split_str(delim, s, out, max_items);
  return out;
}

static void test_split_str() {
  // A string with no delimiter yields one item; it used to yield none, which
  // dropped the only entry of a single-option select or one-entry list.
  check(split("one", ',') == std::vector<std::string>{"one"},
        "split_str: no delimiter yields the whole string");

  // Empty entries are dropped (unchanged), but an entry after an empty one is no
  // longer dropped with it: "a,,b" used to collapse to just ["a"].
  check(split("a,,b", ',') == std::vector<std::string>{"a", "b"},
        "split_str: item after an empty entry is no longer dropped");
  check(split(",a", ',') == std::vector<std::string>{"a"},
        "split_str: leading empty entry is dropped");

  // A trailing delimiter used to append a phantom item copied from the stale
  // `item` variable left over from the last loop pass.
  check(split("a,", ',') == std::vector<std::string>{"a"},
        "split_str: trailing delimiter adds no phantom item");

  check(split(",,", ',').empty(), "split_str: delimiters only yield no items");
  check(split("", ',').empty(), "split_str: empty string yields no items");

  // max_items caps the delimiter-terminated items but never suppresses the
  // remainder after the last delimiter. Pre-existing quirk, pinned because the
  // behaviour changed shape around it and nothing here covered it.
  check(split("a,b,c,d", ',', 2) == std::vector<std::string>{"a", "b"},
        "split_str: max_items truncates while delimiters remain");
  check(split("a,b,c", ',', 2) == std::vector<std::string>{"a", "b", "c"},
        "split_str: max_items does not drop the final remainder");

  // The index lookups that used to abort now bail out instead; this is the
  // contract those callers rely on.
  auto modes = split("auto,off,", ',');
  uint8_t index = 3;
  check(modes.size() == 2 && modes.size() <= index,
        "split_str: out-of-range mode index is detectable by the caller");
}

static void test_str_to_number() {
  // ESP-IDF builds with -fno-exceptions, so std::stoi/stof/stod abort on these
  // inputs; the replacements must return false instead of aborting.
  int parsed = 0;
  check(!str_to_int("", parsed), "str_to_int: empty string fails");
  check(!str_to_int("abc", parsed), "str_to_int: non-numeric fails");
  check(str_to_int("12abc", parsed) && parsed == 12,
        "str_to_int: parses the numeric prefix");
  check(str_to_int("-5", parsed) && parsed == -5,
        "str_to_int: negative value");
  check(str_to_int(" 42", parsed) && parsed == 42,
        "str_to_int: leading whitespace");
  check(str_to_int("ff", parsed, 16) && parsed == 255,
        "str_to_int: explicit base");

  float parsed_f = 0.0f;
  check(!str_to_float("", parsed_f), "str_to_float: empty string fails");
  check(!str_to_float("nope", parsed_f),
        "str_to_float: non-numeric fails");
  check(str_to_float("0.5", parsed_f) && parsed_f == 0.5f,
        "str_to_float: decimal value");

  double parsed_d = 0.0;
  check(!str_to_double("", parsed_d),
        "str_to_double: empty string fails");
  check(str_to_double("2.25", parsed_d) && parsed_d == 2.25,
        "str_to_double: decimal value");

  // strtol saturates at LONG_MAX instead of throwing out_of_range, and the
  // narrowing cast to int wraps. ESP-IDF's std::stoi would have thrown and then
  // aborted under -fno-exceptions. Pinned so a later clamp here is visible.
  check(str_to_int("99999999999999999999", parsed) &&
            parsed == static_cast<int>(LONG_MAX),
        "str_to_int: out-of-range saturates instead of aborting");
}

static void test_value_or_default() {
  check(value_or_default(std::string(""), 7) == 7,
        "value_or_default: empty string returns the default");
  check(value_or_default(std::string("abc"), 7) == 7,
        "value_or_default: non-numeric returns the default");
  check(value_or_default(std::string("12"), 7) == 12,
        "value_or_default: numeric string is parsed");
  check(value_or_default(std::string("-3"), 7) == -3,
        "value_or_default: negative value");
  check(value_or_default(std::string(""), 3.5) == 3.5,
        "value_or_default(double): empty string returns the default");
  check(value_or_default(std::string("1.25"), 3.5) == 1.25,
        "value_or_default(double): numeric string is parsed");
  check(value_or_default(std::string(""), 4U) == 4U,
        "value_or_default(unsigned): empty string returns the default");
}

static void test_convert_python_arr_str() {
  check(convert_python_arr_str("") == "", "convert_python_arr_str: empty input");
  check(convert_python_arr_str("['a','b']") == "a,b",
        "convert_python_arr_str: quoted list is flattened");
  check(convert_python_arr_str("[1,2]") == "[1,2]",
        "convert_python_arr_str: unquoted list has no matching quotes and is returned as-is");

  // "[x]" has no closing quote pair: the old code called back() on an empty
  // string here, which is undefined behaviour. It must return the input instead.
  check(convert_python_arr_str("[x]") == "[x]",
        "convert_python_arr_str: malformed input returns unchanged");

  // Empty entries are skipped, matching the pre-existing "ignore empty entries"
  // behaviour of this function.
  check(convert_python_arr_str("['a','','b']") == "a,b",
        "convert_python_arr_str: empty entries skipped");
}

int main() {
  test_split_str();
  test_str_to_number();
  test_value_or_default();
  test_convert_python_arr_str();

  printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "all passed", failures,
         failures == 1 ? "" : "s");
  return failures != 0;
}
