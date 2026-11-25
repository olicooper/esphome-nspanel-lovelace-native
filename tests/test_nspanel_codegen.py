"""Codegen regression tests for the nspanel_lovelace component.

Runs ``esphome compile --only-generate`` against the synthetic fixture
``ci/codegen.yaml`` and asserts on the generated ``src/main.cpp``.
No hardware and no secrets: the fixture uses only fictional entities and
hardcoded values. Run inside the approved ESPHome 2026.11.0 runner image.
"""

import json
import re
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
CODEGEN_YAML = REPO_ROOT / "ci" / "codegen.yaml"
BUILD_DIR = REPO_ROOT / "ci" / ".esphome" / "build" / "ci-codegen"
MAIN_CPP = BUILD_DIR / "src" / "main.cpp"
DEFINES_H = BUILD_DIR / "src" / "esphome" / "core" / "defines.h"


def run_esphome(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        ["esphome", *args],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=900,
    )


# Minimal synthetic config for temporary codegen scenarios (constructed in
# tmp_path, not duplicated production YAML). No real entities, no secrets.
SYNTHETIC_CONFIG = """\
esphome:
  name: {name}
  comment: "synthetic codegen fixture"
  build_path: ./build/{name}
{esphome_extra}

esp32:
  board: esp32dev
  framework:
    type: esp-idf

external_components:
  - source:
      type: local
      path: {components}
    components: [nspanel_lovelace]

nspanel_lovelace:
{nspanel_body}

logger:
  baud_rate: 115200

wifi:
  ssid: 'CODEGEN_SSID'
  password: 'CODEGEN_PASSWORD'

api: {{}}

uart:
  tx_pin: 16
  rx_pin: 17
  baud_rate: 115200

time:
  - platform: homeassistant
    id: codegen_time
    timezone: Europe/London
"""


def generate_synthetic(tmp_path: Path, nspanel_body: str, name: str,
                       cwd: Path | None = None, esphome_extra: str = "") -> str:
    """Write a synthetic config to tmp_path, run codegen only, and return the
    generated main.cpp source. Uuids are allocated numerically in codegen
    order, so a screensaver without an explicit id gets uuid "1". cwd runs
    esphome from a different working directory (default: repo root); the
    build output still lands under the config file's directory. esphome_extra
    adds indented keys to the esphome: section (e.g. build_flags)."""
    cfg = tmp_path / f"{name}.yaml"
    cfg.write_text(SYNTHETIC_CONFIG.format(
        name=name,
        components=str(REPO_ROOT / "components"),
        nspanel_body=nspanel_body,
        esphome_extra=esphome_extra,
    ))
    result = subprocess.run(
        ["esphome", "compile", "--only-generate", str(cfg)],
        cwd=cwd or REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=900,
    )
    assert result.returncode == 0, (
        f"esphome compile --only-generate {name}.yaml failed:\n"
        f"{result.stdout[-4000:]}\n{result.stderr[-4000:]}"
    )
    main = cfg.parent / ".esphome" / "build" / name / "src" / "main.cpp"
    assert main.is_file(), f"missing generated {main}"
    return main.read_text()


def _full_translation(**overrides) -> dict:
    """A complete translation dict (all required keys), with overrides."""
    en = json.loads((REPO_ROOT / "components" / "nspanel_lovelace"
                     / "translations" / "en.json").read_text(encoding="utf-8"))
    en.update(overrides)
    return en


_TRANSLATION_CARD = """\
  cards:
    - type: cardGrid
      id: main_card
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
"""


def test_translation_config_relative_wins(tmp_path: Path) -> None:
    """A translation file present in both the config dir and the working dir:
    the config-relative file wins (the CWD copy is deliberately missing a
    required key, so using it would fail the build)."""
    work = tmp_path / "work"
    cfg_dir = work / "cfgdir"
    cfg_dir.mkdir(parents=True)
    (cfg_dir / "shared.json").write_text(
        json.dumps(_full_translation(none="CFG_RELATIVE_WINS")), encoding="utf-8")
    bad = _full_translation()
    del bad["none"]
    (work / "shared.json").write_text(json.dumps(bad), encoding="utf-8")
    body = "  locale:\n    language: shared.json\n" + _TRANSLATION_CARD
    src = generate_synthetic(cfg_dir, body, "trans-wins", cwd=work)
    assert "CFG_RELATIVE_WINS" in src
    assert_no_none_tokens(src)


def test_translation_legacy_cwd_fallback(tmp_path: Path) -> None:
    """A translation file that exists only in the working dir (legacy layout)
    still loads, with a one-time working-directory warning."""
    work = tmp_path / "work"
    cfg_dir = work / "cfgdir"
    cfg_dir.mkdir(parents=True)
    (work / "legacy.json").write_text(
        json.dumps(_full_translation(none="LEGACY_CWD_VALUE")), encoding="utf-8")
    body = "  locale:\n    language: legacy.json\n" + _TRANSLATION_CARD
    cfg = cfg_dir / "trans-legacy.yaml"
    cfg.write_text(SYNTHETIC_CONFIG.format(
        name="trans-legacy",
        components=str(REPO_ROOT / "components"),
        nspanel_body=body,
        esphome_extra="",
    ))
    result = subprocess.run(
        ["esphome", "compile", "--only-generate", str(cfg)],
        cwd=work,
        capture_output=True,
        text=True,
        timeout=900,
    )
    assert result.returncode == 0, (
        f"legacy CWD translation should still build:\n"
        f"{result.stdout[-4000:]}\n{result.stderr[-4000:]}")
    out = result.stdout + result.stderr
    assert out.count("working-directory") == 1, out
    main = cfg_dir / ".esphome" / "build" / "trans-legacy" / "src" / "main.cpp"
    assert "LEGACY_CWD_VALUE" in main.read_text()


def _build_dir(tmp_path: Path, name: str) -> Path:
    # ESPHome 2026.11.0 builds under <config dir>/.esphome/build/<name>
    # regardless of the deprecated build_path key.
    return tmp_path / ".esphome" / "build" / name


def test_codegen_test_device_mode_disables_psram(tmp_path: Path) -> None:
    """esphome.build_flags: [-DTEST_DEVICE_MODE] (the location test.yaml
    actually uses) must skip the component's PSRAM SDK settings and defines."""
    generate_synthetic(tmp_path, _TRANSLATION_CARD, "test-device",
                       esphome_extra="  build_flags:\n    - -DTEST_DEVICE_MODE")
    build = _build_dir(tmp_path, "test-device")
    sdk = (build / "sdkconfig.test-device").read_text()
    assert "CONFIG_SPIRAM_USE_MALLOC=y" not in sdk
    assert "CONFIG_ESP32_SPIRAM_SUPPORT=y" not in sdk
    defines = (build / "src" / "esphome" / "core" / "defines.h").read_text()
    assert "#define USE_PSRAM" not in defines


def test_codegen_default_psram_settings(tmp_path: Path) -> None:
    """Without the test flag, the component's PSRAM settings are unchanged."""
    generate_synthetic(tmp_path, _TRANSLATION_CARD, "psram-default")
    build = _build_dir(tmp_path, "psram-default")
    sdk = (build / "sdkconfig.psram-default").read_text()
    assert "CONFIG_SPIRAM=y" in sdk
    assert "CONFIG_SPIRAM_USE_MALLOC=y" in sdk
    assert "CONFIG_ESP32_SPIRAM_SUPPORT=y" in sdk
    defines = (build / "src" / "esphome" / "core" / "defines.h").read_text()
    assert "#define USE_PSRAM" in defines


def test_codegen_tft_upload_build_flag(tmp_path: Path) -> None:
    """-DDISABLE_NSPANEL_TFT_UPLOAD must suppress -DUSE_NSPANEL_TFT_UPLOAD in
    both flag locations; the default build still emits it."""
    generate_synthetic(
        tmp_path, _TRANSLATION_CARD, "tft-off",
        esphome_extra="  build_flags:\n    - -DDISABLE_NSPANEL_TFT_UPLOAD")
    cmake = (_build_dir(tmp_path, "tft-off") / "CMakeLists.txt").read_text()
    assert "-DUSE_NSPANEL_TFT_UPLOAD" not in cmake

    generate_synthetic(
        tmp_path, _TRANSLATION_CARD, "tft-off-legacy",
        esphome_extra=("  platformio_options:\n"
                       "    build_flags:\n"
                       "      - -DDISABLE_NSPANEL_TFT_UPLOAD"))
    cmake = (_build_dir(tmp_path, "tft-off-legacy") / "CMakeLists.txt").read_text()
    assert "-DUSE_NSPANEL_TFT_UPLOAD" not in cmake

    generate_synthetic(tmp_path, _TRANSLATION_CARD, "tft-on")
    cmake = (_build_dir(tmp_path, "tft-on") / "CMakeLists.txt").read_text()
    assert "-DUSE_NSPANEL_TFT_UPLOAD" in cmake


def test_codegen_weather_template_sensor_default(tmp_path: Path) -> None:
    """forecast_method 'template_sensor' (the default) emits no service
    defines, and its deprecation warning must not promise a removal
    version (the 2026.6 promise is stale - 2026.11.0 is the floor)."""
    body = """\
  screensaver:
    weather:
      entity_id: weather.demo
  cards:
    - type: cardGrid
      id: main_card
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
"""
    cfg = tmp_path / "weather-legacy.yaml"
    cfg.write_text(SYNTHETIC_CONFIG.format(
        name="weather-legacy",
        components=str(REPO_ROOT / "components"),
        nspanel_body=body,
        esphome_extra="",
    ))
    result = subprocess.run(
        ["esphome", "compile", "--only-generate", str(cfg)],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=900,
    )
    assert result.returncode == 0, (
        f"template_sensor weather should still build:\n"
        f"{result.stdout[-4000:]}\n{result.stderr[-4000:]}")
    defines = (_build_dir(tmp_path, "weather-legacy")
               / "src" / "esphome" / "core" / "defines.h").read_text()
    assert "#define USE_NSPANEL_WEATHER_SERVICE" not in defines
    assert "#define USE_API_CUSTOM_SERVICES" not in defines
    out = result.stdout + result.stderr
    assert "forecast_method 'template_sensor' is deprecated" in out, out
    assert "will be removed" not in out, out


def assert_no_none_tokens(src: str) -> None:
    """No Python None token may leak into generated C++ (string literals are
    legitimate, e.g. the translation value "None", so they are stripped)."""
    stripped = re.sub(r'"(?:\\.|[^"\\])*"', '""', src)
    assert re.search(r"\bNone\b", stripped) is None


def test_codegen_baseline() -> None:
    result = run_esphome("compile", "--only-generate", str(CODEGEN_YAML))
    assert result.returncode == 0, (
        "esphome compile --only-generate ci/codegen.yaml failed:\n"
        f"{result.stdout[-4000:]}\n{result.stderr[-4000:]}"
    )
    assert MAIN_CPP.is_file(), f"missing generated {MAIN_CPP}"
    src = MAIN_CPP.read_text()
    defines = DEFINES_H.read_text()

    # Each unique ordinary entity is created exactly once even when the
    # tile repeats across cards; navigate/iText are registered as entities;
    # 'delete' is not an HA entity and must not be created.
    assert src.count('create_entity("light.kitchen")') == 1
    assert src.count('create_entity("sensor.demo_hidden")') == 1
    assert src.count('create_entity("navigate.hidden_card")') == 1
    assert src.count('create_entity("iText.Hello")') == 1
    assert 'create_entity("delete")' not in src

    # A tile without a name uses the 2-arg constructor (uuid + entity only);
    # the named tile gets the escaped display name as a third argument.
    assert re.search(
        r"std::make_shared<nspanel_lovelace::GridCardEntityItem>\(\"[0-9]+\", "
        r"nspanel_e\d+\)",
        src,
    ), "unnamed tile must generate the 2-arg constructor"
    assert re.search(
        r"std::make_shared<nspanel_lovelace::GridCardEntityItem>\(\"[0-9]+\", "
        r"nspanel_e\d+, \"Kitchen \\042Main\\042 \\134 west\"\)",
        src,
    ), "named tile must generate the escaped display name"

    # All 'hiking' tiles (two card tiles + the left status icon) share one
    # CUSTOM_ICONS slot (ED7E) and reference it three times; built-in icons
    # stay on the enum path.
    assert src.count('u8"\\uED7E"') == 1
    assert "#define CUSTOM_ICONS_SIZE 1" in defines
    assert src.count("CUSTOM_ICONS[0]") == 3

    # The hidden card is marked hidden exactly once.
    assert src.count("set_hidden(true)") == 1

    # Visible navigation ring: with two visible cards both prev and next
    # wrap around, so both card uuids appear as navigation targets.
    assert re.search(r'new nspanel_lovelace::NavigationItem\("[0-9]+", "main_card", ', src)
    assert re.search(r'new nspanel_lovelace::NavigationItem\("[0-9]+", "last_card", ', src)
    # The hidden card is not part of the visible ring but links home to the
    # screensaver (uuid "1", allocated first).
    assert re.search(r'new nspanel_lovelace::NavigationItem\("[0-9]+", "1", ', src)

    # Status icons: the left icon configures a custom glyph (hiking ->
    # CUSTOM_ICONS[0]) with explicit color 0 -> 4-arg constructor; the right
    # icon configures the built-in alert-circle-outline enum (no color) ->
    # 3-arg constructor; alt_font is preserved.
    assert len(re.findall(
        r'make_shared<nspanel_lovelace::StatusIconItem>\("[0-9]+", '
        r'nspanel_e\d+, CUSTOM_ICONS\[0\], 0u\)',
        src,
    )) == 1
    assert len(re.findall(
        r'make_shared<nspanel_lovelace::StatusIconItem>\("[0-9]+", '
        r'nspanel_e\d+, (?:nspanel_lovelace::)?icon_t::alert_circle_outline\)',
        src,
    )) == 1
    assert "set_alt_font(true)" in src

    # Weather uses the raw entity string, not create_entity; the fixture's
    # forecast_method 'service' emits both service defines.
    assert 'set_weather_entity_id("weather.demo_codegen")' in src
    assert "#define USE_NSPANEL_WEATHER_SERVICE" in defines
    assert "#define USE_API_CUSTOM_SERVICES" in defines

    # The api states/services defines are emitted unconditionally (the
    # pre-2025.8 version guard is gone; the floor is 2026.11.0).
    assert "#define USE_API_HOMEASSISTANT_STATES" in defines
    assert "#define USE_API_HOMEASSISTANT_SERVICES" in defines

    # The fixture locale is a custom JSON file: the firmware language is
    # "unknown" and the translation map carries the changed string value and
    # the extra key.
    assert 'set_language("unknown")' in src
    assert "none_codegen_custom" in src
    assert "codegen_extra_value" in src

    assert_no_none_tokens(src)


def test_codegen_all_hidden_cards(tmp_path: Path) -> None:
    """Screensaver plus all-hidden cards: codegen must not touch the (empty)
    visible list; hidden cards are still created and link home to the
    screensaver."""
    body = """\
  screensaver: {}
  cards:
    - type: cardGrid
      hidden: true
      entities:
        - entity_id: light.lamp_a
    - type: cardGrid
      hidden: true
      entities:
        - entity_id: light.lamp_b
"""
    src = generate_synthetic(tmp_path, body, "all-hidden")
    assert src.count("set_hidden(true)") == 2
    # Both hidden cards link home to the screensaver (uuid "1"); no right nav.
    assert len(re.findall(r'NavigationItem\("[0-9]+", "1", ', src)) == 2
    assert "set_nav_right" not in src
    assert_no_none_tokens(src)


def test_codegen_status_icon_color_only(tmp_path: Path) -> None:
    """A status icon that sets only a color (no icon) takes the default glyph
    with the configured color (4-arg constructor)."""
    body = """\
  screensaver:
    status_icon_left:
      entity_id: light.lamp_a
      icon:
        color: 300
"""
    src = generate_synthetic(tmp_path, body, "status-color-only")
    assert len(re.findall(
        r'make_shared<nspanel_lovelace::StatusIconItem>\("[0-9]+", '
        r'nspanel_e\d+, (?:nspanel_lovelace::)?icon_t::alert_circle_outline, 300u\)',
        src,
    )) == 1
    assert_no_none_tokens(src)


def test_codegen_trailing_hidden_card(tmp_path: Path) -> None:
    """Two visible cards followed by a trailing hidden card: the visible ring
    wraps between the two visible cards and the hidden card links home."""
    body = """\
  screensaver: {}
  cards:
    - type: cardGrid
      id: v1
      entities:
        - entity_id: light.lamp_a
    - type: cardGrid
      id: v2
      entities:
        - entity_id: light.lamp_b
    - type: cardGrid
      id: h1
      hidden: true
      entities:
        - entity_id: light.lamp_c
"""
    src = generate_synthetic(tmp_path, body, "trailing-hidden")
    # Page uuids: screensaver="1" (auto-allocated first), cards use their
    # explicit ids: v1, v2, h1.
    assert src.count("set_hidden(true)") == 1
    # Two visible cards wrap around: v1.prev=v2, v1.next=v2, v2.prev=v1, v2.next=v1
    assert len(re.findall(r'NavigationItem\("[0-9]+", "v2", ', src)) == 2
    assert len(re.findall(r'NavigationItem\("[0-9]+", "v1", ', src)) == 2
    # The trailing hidden card links home to the screensaver
    assert len(re.findall(r'NavigationItem\("[0-9]+", "1", ', src)) == 1
    assert src.count("set_nav_right") == 2
    assert_no_none_tokens(src)


def test_codegen_single_visible_card(tmp_path: Path) -> None:
    """A single visible card keeps its existing self-links (prev=next=self)."""
    body = """\
  screensaver: {}
  cards:
    - type: cardGrid
      id: only
      entities:
        - entity_id: light.lamp_a
"""
    src = generate_synthetic(tmp_path, body, "single-visible")
    # Page uuids: screensaver="1" (auto-allocated first), card uses its
    # explicit id "only"; both nav items point at the card itself.
    assert len(re.findall(r'NavigationItem\("[0-9]+", "only", ', src)) == 2
    assert src.count("set_nav_right") == 1
    assert "set_hidden" not in src
    assert_no_none_tokens(src)
