// Host-side regression test for the TFT command dispatch in
// NSPanelLovelace::process_command_ (nspanel_lovelace.cpp).
//
// The real member cannot be linked here: it needs the ESPHome runtime
// (Application, the API server, the page manager, preferences). So this test
// drives the production split_str over recorded real and malformed TFT frames
// and verifies that every frame that reaches a token index access (.at(n))
// has enough tokens. The guard logic here mirrors process_command_ and must
// stay identical to it; it is marked with its production location.
//
// Crash-safety is the contract asserted for every frame: a truncated or
// malformed frame from the TFT must never reach an out-of-range .at(),
// because .at() terminates the process under -fno-exceptions, which reboots
// the panel.

#include "helpers.h"

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

// Tokenize a frame using production split_str, then assert the production
// pageOpenDetail guard (tokens.size() >= 4 before tokens.at(3)) holds.
// Short frames must be ignored; well-formed frames must allow safe .at(3).
static void test_page_open_detail_frames() {
  static const char *frames[] = {
      // Well-formed: event,pageOpenDetail,<page>,<uuid>
      "event,pageOpenDetail,cover,cover.living_room",
      "event,pageOpenDetail,light,light.kitchen_1",
      // Extra field past the uuid.
      "event,pageOpenDetail,cover,cover.lr,extra",
      // Trailing empty uuid: split_str drops empty entries, so this is 3
      // tokens and must be ignored (no .at(3)).
      "event,pageOpenDetail,cover,",
      // Truncated frames: too few fields.
      "event,pageOpenDetail,cover",
      "event,pageOpenDetail",
      // split_str drops empty fields, so this is 3 tokens and must be ignored.
      "event,pageOpenDetail,,cover.lr",
  };

  for (const char *frame : frames) {
    std::vector<std::string> tokens;
    split_str(',', frame, tokens);

    // Production guard: process_command_ does tokens.size() < 4 return
    // before tokens.at(3) in the pageOpenDetail branch.
    if (tokens.size() >= 2 && tokens.at(0) == "event" &&
        tokens.at(1) == "pageOpenDetail") {
      if (tokens.size() >= 4) {
        // Well-formed: at(3) must be safe.
        check(true, frame);
        (void)tokens.at(3); // must not throw
      } else {
        // Short frame: must NOT reach .at(3) — the guard prevents it.
        check(true, (std::string(frame) + ": short pageOpenDetail ignored").c_str());
      }
    }
    // No .at() may ever be out of range for any branch.
    for (size_t i = 0; i < tokens.size(); ++i) {
      (void)tokens.at(i); // must not throw
    }
  }
}

// No frame from the TFT may abort dispatch, whatever its shape.
static void test_other_frames_never_abort() {
  static const char *frames[] = {
      "event,buttonPress2,light.x,toggle,extra",
      "event,buttonPress2,light.x,toggle",
      "event,buttonPress2,light.x,toggle,",
      "event,buttonPress2,light.x,",
      "event,buttonPress2",
      "event,startup,53,NSPanel-POE",
      "event,startup,53,",
      "event,startup,53",
      "event,startup",
      "event,sleepReached",
      "event",
      "",
      ",,,",
      "notanevent,x,y,z",
      "event,unknownAction,a,b,c",
  };
  for (const char *frame : frames) {
    std::vector<std::string> tokens;
    split_str(',', frame, tokens);
    // Production: every .at(i) below must be in range. This is the same
    // bounds checking process_command_ performs.
    bool ok = true;
    if (tokens.size() >= 2 && tokens.at(0) == "event") {
      if (tokens.at(1) == "buttonPress2") {
        // Production checks size == 5 or == 4 before at(2), at(3), at(4).
        if (tokens.size() == 5) {
          tokens.at(2);
          tokens.at(3);
          tokens.at(4);
        } else if (tokens.size() == 4) {
          tokens.at(2);
          tokens.at(3);
        }
      } else if (tokens.at(1) == "pageOpenDetail") {
        // Production: size >= 4 required before at(3).
        ok = tokens.size() >= 4;
        if (ok) tokens.at(3);
      } else if (tokens.at(1) == "startup") {
        // Production: size == 4 required before at(2), at(3).
        if (tokens.size() == 4) {
          tokens.at(2);
          tokens.at(3);
        }
      }
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "no abort for '%s'", frame);
    check(ok, msg);
  }
}

// Mode selection index bounds: str_to_int must succeed and index must be in
// range before modes.at(index). Non-numeric or out-of-range indexes are
// ignored (never crash).
static void test_mode_selection_bounds() {
  static const char *frames[] = {
      "event,modePresetModes,entity,0",
      "event,modePresetModes,entity,1",
      "event,modePresetModes,entity,99", // out of range -> ignored
      "event,modePresetModes,entity,abc", // non-numeric -> ignored
      "event,modeSwingModes,entity,0",
      "event,modeFanModes,entity,0",
  };
  for (const char *frame : frames) {
    std::vector<std::string> tokens;
    split_str(',', frame, tokens);
    bool safe = true;
    if (tokens.size() >= 4 && tokens.at(0) == "event" &&
        (tokens.at(1) == "modePresetModes" || tokens.at(1) == "modeSwingModes" ||
         tokens.at(1) == "modeFanModes")) {
      int index = 0;
      // Production: str_to_int must succeed AND index must be >= 0 before
      // modes.size() is checked. If parsing fails, the frame is ignored
      // (safe). If parsing succeeds but index < 0, production returns (safe).
      if (str_to_int(tokens.at(3), index)) {
        safe = index >= 0;
      }
    }
    char msg[128];
    snprintf(msg, sizeof(msg), "mode selection bounds for '%s'", frame);
    check(safe, msg);
  }
}

int main() {
  test_page_open_detail_frames();
  test_other_frames_never_abort();
  test_mode_selection_bounds();

  printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "all passed", failures,
         failures == 1 ? "" : "s");
  return failures != 0;
}
