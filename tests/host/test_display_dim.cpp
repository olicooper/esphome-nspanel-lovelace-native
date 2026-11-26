// Host-side tests for display dim defaults/restore guard (issue 13).
#include <cstdio>
#include <cstring>

#include "panel_fixture.h"

using namespace esphome;
using namespace esphome::nspanel_lovelace;

static int test_failures = 0;
#define CHECK(cond, msg) do { \
  if (!(cond)) { \
    std::fprintf(stderr, "FAIL: %s\n", msg); \
    ++test_failures; \
  } else { \
    std::printf("ok   %s\n", msg); \
  } \
} while (0)

struct DimPanel : Panel {
  using NSPanelLovelace::restore_state_;
  using NSPanelLovelace::save_state_;
  using NSPanelLovelace::display_active_dim_;
  using NSPanelLovelace::display_inactive_dim_;
  using NSPanelLovelace::display_defaults_configured_;
  using NSPanelLovelace::pref_;
};

static void set_stored_dims(uint8_t active, uint8_t inactive) {
  NSPanelRestoreState state{};
  state.display_active_dim_ = active;
  state.display_inactive_dim_ = inactive;
  host_prefs.loaded = true;
  std::memcpy(host_prefs.stored, &state, sizeof(state));
}

static void test_restore_skipped_when_defaults_configured() {
  DimPanel p;
  set_stored_dims(80, 20);
  p.display_active_dim_ = 100;
  p.display_inactive_dim_ = 50;
  p.display_defaults_configured_ = true;

  bool restored = p.restore_state_();
  CHECK(restored, "restore returns true when pref loaded");
  CHECK(p.display_active_dim_ == 100, "configured active dim preserved");
  CHECK(p.display_inactive_dim_ == 50, "configured inactive dim preserved");
}

static void test_restore_applied_when_defaults_not_configured() {
  DimPanel p;
  set_stored_dims(80, 20);
  p.display_active_dim_ = 100;
  p.display_inactive_dim_ = 50;
  p.display_defaults_configured_ = false;

  bool restored = p.restore_state_();
  CHECK(restored, "restore returns true when pref loaded and not configured");
  CHECK(p.display_active_dim_ == 80, "persisted active dim restored");
  CHECK(p.display_inactive_dim_ == 20, "persisted inactive dim restored");
}

static void test_restore_noop_when_not_loaded() {
  DimPanel p;
  host_prefs.loaded = false;
  p.display_active_dim_ = 100;
  p.display_inactive_dim_ = 50;
  p.display_defaults_configured_ = false;

  bool restored = p.restore_state_();
  CHECK(!restored, "restore returns false when pref not loaded");
  CHECK(p.display_active_dim_ == 100, "active dim unchanged when not loaded");
  CHECK(p.display_inactive_dim_ == 50, "inactive dim unchanged when not loaded");
}

static void test_save_state_writes_current_dims() {
  DimPanel p;
  host_prefs.loaded = true;
  std::memset(host_prefs.stored, 0, sizeof(host_prefs.stored));
  p.pref_ = global_preferences->make_preference<NSPanelRestoreState>(0);
  p.display_active_dim_ = 70;
  p.display_inactive_dim_ = 30;

  bool saved = p.save_state_();
  CHECK(saved, "save_state_ returns true");

  NSPanelRestoreState stored{};
  std::memcpy(&stored, host_prefs.stored, sizeof(stored));
  CHECK(stored.display_active_dim_ == 70, "saved active dim");
  CHECK(stored.display_inactive_dim_ == 30, "saved inactive dim");
}

int main() {
  test_restore_skipped_when_defaults_configured();
  test_restore_applied_when_defaults_not_configured();
  test_restore_noop_when_not_loaded();
  test_save_state_writes_current_dims();
  if (test_failures == 0) std::printf("all passed\n");
  return test_failures;
}
