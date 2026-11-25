// Host-side regression test for the TFT command dispatch in
// NSPanelLovelace::process_command_ (nspanel_lovelace.cpp).
//
// The real member cannot be linked here: it needs the ESPHome runtime
// (Application, the API server, the page manager, preferences). So this drives
// the production split_str over recorded real and malformed TFT frames and
// applies the same token-count decisions to a stub sink.
//
// dispatch() must stay identical to process_command_; the guard it mirrors is
// marked with its production location. Crash-safety is the contract asserted
// for every frame: a truncated or malformed frame from the TFT must never
// reach an out-of-range .at(), because .at() terminates the process under
// -fno-exceptions, which reboots the panel.

#include "helpers.h"

#include <cstdio>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <unistd.h>

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

// Records what the dispatch decided, so the frame table can assert the branch
// and the value handed to the popup renderer.
struct Sink {
  const char *branch = "none";
  std::string uuid;
};

// Mirrors NSPanelLovelace::process_command_(const std::string &) branch for
// branch, with the member calls replaced by sink writes.
static void dispatch(const std::vector<std::string> &tokens, Sink &sink) {
  if (tokens.size() < 2 || tokens.at(0) != "event") { return; }

  if (tokens.at(1) == "buttonPress2") {
    if (tokens.size() == 5) {
      sink.branch = "buttonPress2/3";
    } else if (tokens.size() == 4) {
      sink.branch = "buttonPress2/2";
    }
  } else if (tokens.at(1) == "pageOpenDetail") {
    // Mirrors the nspanel_lovelace.cpp process_command_ pageOpenDetail guard:
    // never index tokens.at(3) without at least 4 fields.
    if (tokens.size() < 4) return;
    sink.branch = "pageOpenDetail";
    sink.uuid = tokens.at(3);
  } else if (tokens.at(1) == "sleepReached") {
    sink.branch = "sleepReached";
  } else if (tokens.at(1) == "startup") {
    if (tokens.size() == 4) {
      sink.branch = "startup";
    }
  }
}

// Runs the dispatch for `frame` in a child process and reports whether it died
// from the abort that an out-of-range .at() produces under -fno-exceptions.
static bool dispatch_aborts(const char *frame) {
  fflush(stdout);
  fflush(stderr);
  pid_t pid = fork();
  if (pid < 0) {
    printf("FAIL  fork failed for %s\n", frame);
    ++failures;
    return false;
  }
  if (pid == 0) {
    std::vector<std::string> tokens;
    split_str(',', frame, tokens);
    Sink sink;
    dispatch(tokens, sink);
    _exit(0);
  }
  int status = 0;
  while (waitpid(pid, &status, 0) < 0) {}
  return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
}

// Real frames as the TFT sends them, plus truncated and malformed variants.
// `uuid` is what the popup renderer must receive; empty means "no popup fields".
static void test_page_open_detail_frames() {
  struct Case {
    const char *frame;
    const char *uuid;
  };
  static const Case cases[] = {
      // Well-formed: event,pageOpenDetail,<page>,<uuid>
      {"event,pageOpenDetail,cover,cover.living_room", "cover.living_room"},
      {"event,pageOpenDetail,light,light.kitchen_1", "light.kitchen_1"},
      // Extra field past the uuid.
      {"event,pageOpenDetail,cover,cover.lr,extra", "cover.lr"},
      // Trailing empty uuid: split_str produces no 4th token for this, so the
      // frame is now ignored. Before the split fix the phantom empty token made
      // it 4 fields and an empty uuid reached the popup renderer.
      {"event,pageOpenDetail,cover,", nullptr},
      // Truncated frames: too few fields for any popup to address.
      {"event,pageOpenDetail,cover", nullptr},
      {"event,pageOpenDetail", nullptr},
      // split_str drops empty fields, so this arrives as 3 tokens: the uuid
      // would be read from index 3 and abort. It is now ignored instead.
      {"event,pageOpenDetail,,cover.lr", nullptr},
  };

  for (const auto &c : cases) {
    std::vector<std::string> tokens;
    split_str(',', c.frame, tokens);
    int before = failures;
    check(!dispatch_aborts(c.frame), c.frame);
    // If the dispatch aborted, the branch/uuid assertions below are meaningless.
    if (failures != before) continue;

    Sink sink;
    dispatch(tokens, sink);
    if (c.uuid == nullptr) {
      char buf[128];
      snprintf(buf, sizeof(buf), "%s: short frame is ignored", c.frame);
      check(sink.branch == std::string("none"), buf);
    } else {
      char buf[128];
      snprintf(buf, sizeof(buf), "%s -> uuid '%s'", c.frame, c.uuid);
      check(sink.branch == std::string("pageOpenDetail") && sink.uuid == c.uuid,
            buf);
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
    char buf[128];
    snprintf(buf, sizeof(buf), "no abort for '%s'", frame);
    check(!dispatch_aborts(frame), buf);
  }
}

int main() {
  test_page_open_detail_frames();
  test_other_frames_never_abort();

  printf("\n%s (%d failure%s)\n", failures ? "FAILED" : "all passed", failures,
         failures == 1 ? "" : "s");
  return failures != 0;
}
