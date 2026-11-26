"""Unit tests for nspanel_lovelace validators and helpers.

Two mechanisms:
- Direct import of the component module (namespace package) for pure
  validator/helper behaviour.
- ``esphome config`` subprocesses over synthetic YAML in ``tmp_path`` for
  full-schema (CONFIG_SCHEMA) behaviour. Synthetic configs contain no real
  entities and no secrets.

Run inside the approved ESPHome 2026.11.0 runner image.
"""

import importlib
import json
import re
import subprocess
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
COMPONENTS_DIR = REPO_ROOT / "components"
BUNDLED_EN = COMPONENTS_DIR / "nspanel_lovelace" / "translations" / "en.json"


def _full_translation(**overrides) -> dict:
    """A complete translation dict (all required keys), with overrides."""
    data = json.loads(BUNDLED_EN.read_text(encoding="utf-8"))
    data.update(overrides)
    return data


def load_component():
    """Import the component module under test (once)."""
    if str(REPO_ROOT) not in sys.path:
        sys.path.insert(0, str(REPO_ROOT))
    return importlib.import_module("components.nspanel_lovelace")


# Minimal valid base config; {nspanel_body} is the indented nspanel_lovelace
# body under test. `esphome config` validates only, so no toolchain is needed.
BASE_CONFIG = """\
esphome:
  name: {name}
  comment: "synthetic validation fixture"
  build_path: ./build/{name}

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
  ssid: 'VALIDATION_SSID'
  password: 'VALIDATION_PASSWORD'

api: {{}}

uart:
  tx_pin: 16
  rx_pin: 17
  baud_rate: 115200

time:
  - platform: homeassistant
    id: validation_time
    timezone: America/New_York
"""


def esphome_config(tmp_path: Path, nspanel_body: str, name: str = "validation"):
    """Write a synthetic config and run `esphome config` on it."""
    cfg = tmp_path / f"{name}.yaml"
    cfg.write_text(BASE_CONFIG.format(
        name=name,
        components=str(COMPONENTS_DIR),
        nspanel_body=nspanel_body,
    ))
    return subprocess.run(
        ["esphome", "config", str(cfg)],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
        timeout=300,
    )


def test_card_entity_limits_baseline() -> None:
    nsl = load_component()
    assert nsl.get_card_entities_length_limits("cardEntities", "eu") == [1, 4]
    assert nsl.get_card_entities_length_limits("cardEntities", "us-l") == [1, 4]
    assert nsl.get_card_entities_length_limits("cardEntities", "us-p") == [1, 6]
    assert nsl.get_card_entities_length_limits("cardGrid") == [1, 6]
    assert nsl.get_card_entities_length_limits("cardGrid2") == [1, 8]
    assert nsl.get_card_entities_length_limits("cardQR") == [1, 2]
    assert nsl.get_card_entities_length_limits("cardMedia") == [0, 8]


def _point_icons_at(nsl, tmp_path: Path, monkeypatch, file_name: str = "icons.json"):
    """Point the component's icon catalog lookup at a temp file and reset the
    catalog to a clean state. Both the path override and the catalog reset are
    registered with `monkeypatch`, so they are undone on test end even if an
    assertion below raises (no try/finally, no state leak)."""
    monkeypatch.setattr(nsl.resources, "__file__",
                        str(tmp_path / "nspanel_lovelace" / "resources.py"))
    monkeypatch.setattr(nsl.resources, "iconJson", None)
    return tmp_path / "nspanel_lovelace" / file_name


def test_load_icons_missing_catalog(tmp_path: Path, monkeypatch) -> None:
    nsl = load_component()
    target = _point_icons_at(nsl, tmp_path, monkeypatch)
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_icons()
    assert str(target) in str(exc_info.value)


def test_load_icons_bad_json(tmp_path: Path, monkeypatch) -> None:
    nsl = load_component()
    target = _point_icons_at(nsl, tmp_path, monkeypatch)
    target.parent.mkdir(parents=True)
    target.write_text("{not json", encoding="utf-8")
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_icons()
    assert str(target) in str(exc_info.value)
    # The loader must not write anything to the catalog path
    assert target.read_text(encoding="utf-8") == "{not json"


def test_load_icons_empty_catalog(tmp_path: Path, monkeypatch) -> None:
    nsl = load_component()
    target = _point_icons_at(nsl, tmp_path, monkeypatch)
    target.parent.mkdir(parents=True)
    target.write_text("[]", encoding="utf-8")
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_icons()
    assert str(target) in str(exc_info.value)


def test_load_icons_invalid_later_entry(tmp_path: Path, monkeypatch) -> None:
    nsl = load_component()
    target = _point_icons_at(nsl, tmp_path, monkeypatch)
    target.parent.mkdir(parents=True)
    target.write_text(
        '[{"name": "ok-icon", "hex": "E001"}, {"name": "bad-icon", "hex": "xy12"}]',
        encoding="utf-8",
    )
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_icons()
    assert "bad-icon" in str(exc_info.value)
    assert str(target) in str(exc_info.value)


def test_load_icons_valid_two_entries(tmp_path: Path, monkeypatch) -> None:
    nsl = load_component()
    target = _point_icons_at(nsl, tmp_path, monkeypatch)
    target.parent.mkdir(parents=True)
    target.write_text(
        '[{"name": "ok-icon", "hex": "E001"}, {"name": "other-icon", "hex": "E002"}]',
        encoding="utf-8",
    )
    nsl.load_icons()
    assert nsl.get_icon_hex("ok-icon") == "E001"
    assert nsl.get_icon_hex("hex:E002") == "E002"


def test_get_icon_hex_real_catalog() -> None:
    nsl = load_component()
    nsl.resources.iconJson = None
    nsl.load_icons()  # loads the bundled catalog
    assert nsl.get_icon_hex("hiking") == "ED7E"
    assert nsl.get_icon_hex("hex:ED7E") == "ED7E"
    assert nsl.get_icon_hex("no-such-icon-xyz") is None


def test_load_translations_valid() -> None:
    nsl = load_component()
    data = nsl.load_translations(BUNDLED_EN)
    assert data == json.loads(BUNDLED_EN.read_text(encoding="utf-8"))
    assert all(isinstance(v, str) for v in data.values())


@pytest.mark.parametrize(
    "path", sorted((COMPONENTS_DIR / "nspanel_lovelace" / "translations").glob("*.json")),
    ids=lambda p: p.stem,
)
def test_bundled_translations_all_valid(path: Path) -> None:
    """Every bundled locale, not just the English fallback, must load."""
    nsl = load_component()
    data = nsl.load_translations(path)
    assert data == json.loads(path.read_text(encoding="utf-8"))
    assert all(isinstance(v, str) for v in data.values())


def test_valid_color_formats() -> None:
    """Documented colour forms stay accepted; malformed forms are rejected."""
    nsl = load_component()
    assert nsl.valid_color(52857) == 52857
    assert nsl.valid_color("52857") == 52857
    for value in (0, 65535):
        assert nsl.valid_color(value) == value
    for value in ("#FF0000", "FF0000", [255, 0, 0]):
        assert nsl.valid_color(value) == 63488, value
    assert nsl.valid_color("123456") == 4522  # six digits stay hex, not decimal
    for value in ("#ce79", "##FF0000", True, -1, 65536, [256, 0, 0]):
        with pytest.raises(nsl.cv.Invalid):
            nsl.valid_color(value)


def test_load_translations_missing_file(tmp_path: Path) -> None:
    nsl = load_component()
    missing = tmp_path / "nope.json"
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_translations(missing)
    assert str(missing) in str(exc_info.value)
    assert list(exc_info.value.path) == ["locale", "language"]


def test_load_translations_invalid_json(tmp_path: Path) -> None:
    nsl = load_component()
    bad = tmp_path / "bad.json"
    bad.write_text("{not json", encoding="utf-8")
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_translations(bad)
    assert str(bad) in str(exc_info.value)


def test_load_translations_list_not_object(tmp_path: Path) -> None:
    nsl = load_component()
    lst = tmp_path / "list.json"
    lst.write_text("[]", encoding="utf-8")
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_translations(lst)
    assert "object" in str(exc_info.value)


def test_load_translations_missing_required_key(tmp_path: Path) -> None:
    nsl = load_component()
    data = _full_translation()
    del data["none"]
    f = tmp_path / "missing-key.json"
    f.write_text(json.dumps(data), encoding="utf-8")
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_translations(f)
    assert "'none'" in str(exc_info.value)


def test_load_translations_nonstring_values(tmp_path: Path) -> None:
    nsl = load_component()
    data = _full_translation(none=123, extra_key=["not", "a", "string"])
    f = tmp_path / "bad-values.json"
    f.write_text(json.dumps(data), encoding="utf-8")
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.load_translations(f)
    msg = str(exc_info.value)
    assert "none" in msg and "extra_key" in msg


def test_resolve_translation_path_bundled_and_absolute() -> None:
    nsl = load_component()
    config_relative = lambda s: Path("/cfg") / s
    p, legacy = nsl.resolve_translation_path("de", config_relative)
    # resolve_translation_path resolves the bundled directory relative to
    # resources.__file__ (the leaf data module), not the package __init__.
    assert p == Path(nsl.resources.__file__).resolve().parent / "translations" / "de.json"
    assert legacy is False
    p, legacy = nsl.resolve_translation_path("/abs/custom.json", config_relative)
    assert p == Path("/abs/custom.json") and legacy is False


def test_resolve_translation_path_config_relative_and_legacy(
    tmp_path: Path, monkeypatch
) -> None:
    nsl = load_component()
    cwd_file = tmp_path / "shared.json"
    cwd_file.write_text(json.dumps(_full_translation()), encoding="utf-8")
    config_dir = tmp_path / "cfg"
    config_dir.mkdir()
    monkeypatch.chdir(tmp_path)  # legacy CWD target
    config_relative = lambda s: config_dir / s
    # Config-relative target absent -> legacy CWD path is used
    p, legacy = nsl.resolve_translation_path("shared.json", config_relative)
    assert legacy is True and p == cwd_file
    # Config-relative wins when both exist
    (config_dir / "shared.json").write_text(
        json.dumps(_full_translation()), encoding="utf-8")
    p, legacy = nsl.resolve_translation_path("shared.json", config_relative)
    assert legacy is False and p == config_dir / "shared.json"


def _translation_body(language: str) -> str:
    return f"""\
  locale:
    language: {language}
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
"""


def test_translation_missing_file_fails_validation(tmp_path: Path) -> None:
    """A missing translation file must fail `esphome config` (not compile)
    with the locale.language path."""
    result = esphome_config(tmp_path, _translation_body("missing_custom.json"),
                           name="trans-missing")
    assert result.returncode != 0, f"missing translation should be invalid: {result.stdout}"
    out = result.stdout + result.stderr
    assert "Failed to load translation" in out, out
    # The error must sit at the locale.language node in the config tree:
    # ESPHome renders it directly above the offending key line.
    assert re.search(
        r"Failed to load translation[^\n]*\n\s+language: missing_custom\.json", out
    ), out


def test_translation_invalid_json_fails_validation(tmp_path: Path) -> None:
    (tmp_path / "bad.json").write_text("{not json", encoding="utf-8")
    result = esphome_config(tmp_path, _translation_body("bad.json"), name="trans-bad")
    assert result.returncode != 0, f"invalid JSON translation should be invalid: {result.stdout}"
    assert "Failed to load translation" in result.stdout + result.stderr


def test_translation_absolute_path_still_works(tmp_path: Path) -> None:
    custom = tmp_path / "absolute.json"
    custom.write_text(json.dumps(_full_translation(none="ABS_VALUE")), encoding="utf-8")
    result = esphome_config(tmp_path, _translation_body(f"{custom}"), name="trans-abs")
    assert result.returncode == 0, result.stdout + result.stderr


def test_empty_language_fails_validation(tmp_path: Path) -> None:
    result = esphome_config(tmp_path, _translation_body("''"), name="trans-empty")
    assert result.returncode != 0, f"empty language should be invalid: {result.stdout}"
    assert "language" in result.stdout + result.stderr


def test_entity_id_exact_domain_matching() -> None:
    """Entity domains must match exactly, not by prefix."""
    nsl = load_component()
    unrestricted = nsl.valid_entity_id()
    assert unrestricted("sensor.room") == "sensor.room"
    assert unrestricted("binary_sensor.room") == "binary_sensor.room"
    assert unrestricted("light.room") == "light.room"
    with pytest.raises(nsl.cv.Invalid):
        unrestricted("sensor_extra.room")  # prefix of 'sensor' is not 'sensor'
    light_only = nsl.valid_entity_id(["light"])
    assert light_only("light.kitchen") == "light.kitchen"
    with pytest.raises(nsl.cv.Invalid):
        light_only("lightning.room")  # prefix of 'light' is not 'light'
    with pytest.raises(nsl.cv.Invalid):
        light_only("sensor.room")


def test_entity_id_pseudo_entities_restricted_fields() -> None:
    """delete / iText / navigate / blank are only valid in generic tiles."""
    nsl = load_component()
    alarm = nsl.valid_entity_id(["alarm_control_panel"])
    assert alarm("alarm_control_panel.home") == "alarm_control_panel.home"
    for bad in ("delete", "iText.Label", "navigate.card_id", ""):
        with pytest.raises(nsl.cv.Invalid):
            alarm(bad)
    weather = nsl.valid_entity_id(["weather"])
    assert weather("weather.home") == "weather.home"
    with pytest.raises(nsl.cv.Invalid):
        weather("sensor.room")


def test_entity_id_pseudo_entities_generic_tile() -> None:
    nsl = load_component()
    tile = nsl.valid_entity_id(allow_navigation=True, allow_text=True, allow_delete=True)
    assert tile("iText.Label") == "iText.Label"
    assert tile("delete") == "delete"
    assert tile("navigate.card_id") == "navigate.card_id"
    assert tile("light.kitchen") == "light.kitchen"
    with pytest.raises(nsl.cv.Invalid):
        tile("iText.Bad~Label")  # tilde exclusion stays intact
    with pytest.raises(nsl.cv.Invalid):
        tile("iTextXabc")  # 'iText.' requires the literal dot
    with pytest.raises(nsl.cv.Invalid):
        tile("delete.something")  # 'delete' is an exact sentinel


def _weather_body(entity_id: str, extra: str = "") -> str:
    return f"""\
  screensaver:
    weather:
      entity_id: {entity_id}
{extra}"""


def test_weather_entity_without_method_valid(tmp_path: Path) -> None:
    """One weather entity is the complete native-weather configuration."""
    result = esphome_config(
        tmp_path, _weather_body("weather.forecast_home"), name="native-weather-ok"
    )
    assert result.returncode == 0, result.stdout + result.stderr


@pytest.mark.parametrize(("options", "milliseconds"), [
    ({}, 0),
    ({"restart_after_forecast_stall": "0s"}, 0),
    ({"restart_after_forecast_stall": "30min"}, 1800000),
    ({"restart_after_forecast_stall": "0.0005min"}, 30),
    ({"restart_after_forecast_stall": "0.000005h"}, 18),
    ({"restart_after_forecast_stall": "4294967.295s"}, 4294967295),
    ({"restart_after_forecast_stall": "4294967295ms"}, 4294967295),
])
def test_weather_stall_restart_duration(options, milliseconds) -> None:
    weather = load_component().valid_screensaver_weather({"entity_id": "weather.home", **options})
    assert weather["restart_after_forecast_stall"].total_milliseconds == milliseconds


@pytest.mark.parametrize("duration", [
    "soon", "-1ms", "-0.0005ms", "4294967296ms", "4294967295.0005ms", "0.1ms", "0.000001h",
])
def test_weather_stall_restart_rejects_invalid_duration(duration) -> None:
    nsl = load_component()
    with pytest.raises(nsl.cv.Invalid):
        nsl.valid_screensaver_weather({"entity_id": "weather.home", "restart_after_forecast_stall": duration})


def test_weather_stall_restart_requires_weather_block(tmp_path: Path) -> None:
    result = esphome_config(tmp_path, "  screensaver:\n    restart_after_forecast_stall: 30min\n",
                            name="weather-stall-misplaced")
    assert result.returncode != 0 and "restart_after_forecast_stall" in result.stdout + result.stderr


def test_omitted_weather_valid(tmp_path: Path) -> None:
    """Weather stays optional; a screensaver without weather still validates."""
    result = esphome_config(
        tmp_path, "  screensaver:\n    id: no_weather\n", name="native-weather-omitted"
    )
    assert result.returncode == 0, result.stdout + result.stderr


def test_sensor_weather_rejected(tmp_path: Path) -> None:
    """A template sensor is not a weather entity and must be rejected."""
    result = esphome_config(
        tmp_path, _weather_body("sensor.forecast_daily"), name="native-weather-sensor"
    )
    assert result.returncode != 0, "sensor.* must not be accepted as weather"
    assert "weather" in result.stdout + result.stderr


def test_forecast_method_service_rejected(tmp_path: Path) -> None:
    """The old service mode option must no longer be accepted."""
    result = esphome_config(
        tmp_path,
        _weather_body("weather.forecast_home", "      forecast_method: service\n"),
        name="native-weather-service-mode",
    )
    assert result.returncode != 0, "forecast_method: service must be rejected"
    assert "forecast_method" in result.stdout + result.stderr


def test_forecast_method_template_sensor_rejected(tmp_path: Path) -> None:
    """The old template-sensor mode option must no longer be accepted."""
    result = esphome_config(
        tmp_path,
        _weather_body("weather.forecast_home", "      forecast_method: template_sensor\n"),
        name="native-weather-template-mode",
    )
    assert result.returncode != 0, "forecast_method: template_sensor must be rejected"
    assert "forecast_method" in result.stdout + result.stderr


def test_duplicate_card_ids_rejected(tmp_path: Path) -> None:
    """A duplicate explicit card id must fail at the second id, not silently
    overwrite the card_ids entry for the first."""
    body = """\
  cards:
    - type: cardGrid
      id: same
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
    - type: cardGrid
      id: same
      entities:
        - entity_id: light.other
"""
    result = esphome_config(tmp_path, body, name="dup-cards")
    assert result.returncode != 0, f"duplicate card ids should be rejected:\n{result.stdout}"
    out = result.stdout + result.stderr
    assert "duplicate id 'same'" in out, out
    # The error must be attached to the SECOND occurrence of 'id: same'.
    msg_pos = out.index("duplicate id 'same'")
    first_id_pos = out.index("id: same")
    assert msg_pos > first_id_pos, out


def test_duplicate_screensaver_card_ids_rejected(tmp_path: Path) -> None:
    body = """\
  screensaver:
    id: shared
    weather:
      entity_id: weather.demo
  cards:
    - type: cardGrid
      id: shared
      entities:
        - entity_id: light.lamp
"""
    result = esphome_config(tmp_path, body, name="dup-screensaver")
    assert result.returncode != 0, (
        f"a screensaver and card sharing an id should be rejected:\n{result.stdout}"
    )
    assert "duplicate id 'shared'" in result.stdout + result.stderr


def test_unknown_default_card_rejected(tmp_path: Path) -> None:
    body = """\
  default_card: missing
  cards:
    - type: cardGrid
      id: present
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
"""
    result = esphome_config(tmp_path, body, name="default-card")
    assert result.returncode != 0
    out = result.stdout + result.stderr
    assert "Cannot find a card with the id 'missing'" in out, out


def test_unknown_card_type_rejected(tmp_path: Path) -> None:
    """An unknown card type such as lowercase 'cardgrid' fails validation."""
    body = """\
  cards:
    - type: cardgrid
      entities:
        - entity_id: light.lamp
"""
    result = esphome_config(tmp_path, body, name="unknown-type")
    assert result.returncode != 0, (
        f"unknown card type should be rejected:\n{result.stdout}"
    )
    out = result.stdout + result.stderr
    assert "Unknown value 'cardgrid'" in out, out
    # The schema must still offer the known keys as a suggestion.
    assert "cardGrid" in out, out


def test_status_icon_requires_entity_id(tmp_path: Path) -> None:
    """A provided status icon block must fail validation without an entity_id,
    not crash codegen."""
    body = """\
  screensaver:
    status_icon_left:
      icon: hiking
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
"""
    result = esphome_config(tmp_path, body, name="status-icon-no-entity")
    assert result.returncode != 0, f"status icon without entity_id should be rejected:\n{result.stdout}"
    out = result.stdout + result.stderr
    assert "entity_id" in out, out


def test_sleep_timeout_top_level_bounds(tmp_path: Path) -> None:
    """Top-level sleep_timeout is 0..65 (HMI protocol maximum)."""
    def body(timeout: int) -> str:
        return f"""\
  sleep_timeout: {timeout}
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: light.lamp
"""
    for timeout in (0, 65):
        result = esphome_config(tmp_path, body(timeout), name=f"timeout-top-{timeout}")
        assert result.returncode == 0, result.stdout + result.stderr
    for timeout in (-1, 66, 120):
        result = esphome_config(tmp_path, body(timeout), name=f"timeout-top-{timeout}")
        assert result.returncode != 0, (
            f"top-level sleep_timeout {timeout} should be rejected:\n{result.stdout}"
        )


def test_sleep_timeout_card_bounds(tmp_path: Path) -> None:
    """Per-card sleep_timeout is also 0..65; 66..120 is deliberately rejected."""
    def body(timeout: int) -> str:
        return f"""\
  screensaver: {{}}
  cards:
    - type: cardGrid
      sleep_timeout: {timeout}
      entities:
        - entity_id: light.lamp
"""
    for timeout in (0, 65):
        result = esphome_config(tmp_path, body(timeout), name=f"timeout-card-{timeout}")
        assert result.returncode == 0, result.stdout + result.stderr
    for timeout in (-1, 66, 120):
        result = esphome_config(tmp_path, body(timeout), name=f"timeout-card-{timeout}")
        assert result.returncode != 0, (
            f"card sleep_timeout {timeout} should be rejected:\n{result.stdout}"
        )


def _validated_body_config(cards, default_card=None, screensaver=None) -> dict:
    """A post-schema nspanel_lovelace config dict (defaults applied the way
    CONFIG_SCHEMA would), so validate_config can be exercised in-process:
    a subprocess (esphome config) cannot expose module state that persists
    across validations within one process."""
    config = {
        "model": "eu",
        "locale": {"language": "en"},
        "cards": cards,
    }
    if screensaver is not None:
        config["screensaver"] = screensaver
    if default_card is not None:
        config["default_card"] = default_card
    return config


def test_stale_card_id_rejected_in_same_process() -> None:
    """Card ids must not persist between validations in one process: after
    validating config A (card id 'a_card'), config B (different cards,
    default_card: a_card) must reject the stale reference. Only an
    in-process repeat validation can prove this."""
    import esphome.core as core
    from esphome.const import KEY_CORE, KEY_TARGET_FRAMEWORK

    nsl = load_component()
    core.CORE.reset()
    core.CORE.data[KEY_CORE] = {KEY_TARGET_FRAMEWORK: "esp-idf"}

    config_a = _validated_body_config(cards=[
        {"type": "cardGrid", "id": "a_card", "hidden": False, "sleep_timeout": 10,
         "entities": [{"entity_id": "light.a"}]},
    ], screensaver={})
    nsl.validate_config(config_a)  # must pass

    config_b = _validated_body_config(cards=[
        {"type": "cardGrid", "id": "b_card", "hidden": False, "sleep_timeout": 10,
         "entities": [{"entity_id": "light.b"}]},
    ], default_card="a_card", screensaver={})
    with pytest.raises(nsl.cv.Invalid) as exc_info:
        nsl.validate_config(config_b)
    assert "a_card" in str(exc_info.value)


def test_dangling_navigation_reports_tile_path(tmp_path: Path) -> None:
    """A navigation reference to a missing card id must fail validation with the
    exact tile path (cards.<i>.entities.<j>.entity_id), not crash with IndexError."""
    body = """\
  cards:
    - type: cardGrid
      id: main_card
      sleep_timeout: 0
      entities:
        - entity_id: navigate.nonexistent
"""
    result = esphome_config(tmp_path, body, name="dangling-nav")
    assert result.returncode != 0, f"config should be invalid: {result.stdout}"
    out = result.stdout + result.stderr
    assert 'no card has the id "nonexistent"' in out, out
    # ESPHome renders the error under the config tree node named by the
    # cv.Invalid path: with the exact tile path the message must sit directly
    # above the offending 'entity_id' line, inside the same tile entry.
    assert re.search(
        r"navigation entity_id invalid[^\n]*\n\s*entity_id: navigate\.nonexistent", out
    ), out


def test_service_tile_accepts_service_name_and_list_data(tmp_path: Path) -> None:
    """A service tile with a list-valued data field must pass config validation."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: service.power_off
          name: Power off
          service: remote.send_command
          data:
            entity_id: remote.example_remote
            command: [standby, volume_up]
"""
    result = esphome_config(tmp_path, body, name="service-tile-ok")
    assert result.returncode == 0, result.stdout + result.stderr


def test_service_tile_rejects_invalid_service_name(tmp_path: Path) -> None:
    """A service name must be a lowercase dotted domain.service, not arbitrary text."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: service.power_off
          service: Remote.Send
          data:
            entity_id: remote.example_remote
"""
    result = esphome_config(tmp_path, body, name="service-tile-bad-name")
    assert result.returncode != 0, "invalid service name should be rejected"
    assert 'service must match "domain.service_name"' in result.stdout + result.stderr


def test_service_tile_requires_data_entity_id(tmp_path: Path) -> None:
    """Service tiles must carry data.entity_id so a target is always supplied."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: service.power_off
          service: remote.send_command
          data:
            command: [standby]
"""
    result = esphome_config(tmp_path, body, name="service-tile-no-target")
    assert result.returncode != 0, "missing data.entity_id should be rejected"
    assert "data.entity_id is required for service tiles" in result.stdout + result.stderr


def test_data_without_service_is_rejected(tmp_path: Path) -> None:
    """data: on a regular tile has nowhere to go and must fail at config time."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: light.kitchen
          data:
            entity_id: light.kitchen
"""
    result = esphome_config(tmp_path, body, name="data-without-service")
    assert result.returncode != 0, "data: without service: should be rejected"
    assert "data is only valid with service" in result.stdout + result.stderr


def test_service_name_on_regular_tile_is_rejected(tmp_path: Path) -> None:
    """service: is restricted to stable service.* tile ids."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: light.kitchen
          service: light.turn_on
          data:
            entity_id: light.kitchen
"""
    result = esphome_config(tmp_path, body, name="service-on-light")
    assert result.returncode != 0, "service: on a light tile should be rejected"
    assert "service is only valid on a service.* tile" in result.stdout + result.stderr


def test_service_tile_rejects_unsupported_data_value(tmp_path: Path) -> None:
    """data values must be serialisable scalars or containers of scalars."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: service.power_off
          service: remote.send_command
          data:
            entity_id: remote.example_remote
            nested: {bad: null}
"""
    result = esphome_config(tmp_path, body, name="service-tile-bad-data")
    assert result.returncode != 0, "null data value should be rejected"
    assert "data values must be scalars or lists/dicts of scalars" in result.stdout + result.stderr


def test_service_tile_rejects_trailing_newline_in_service(tmp_path: Path) -> None:
    """'$' matches before a final newline; service names need a full match."""
    body = """\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: service.power_off
          service: "remote.send_command\\n"
          data:
            entity_id: remote.example_remote
"""
    result = esphome_config(tmp_path, body, name="service-tile-newline")
    assert result.returncode != 0, "service name with trailing newline should be rejected"
    assert 'service must match "domain.service_name"' in result.stdout + result.stderr


@pytest.mark.parametrize("bad", [".nan", ".inf", "-.inf"])
def test_service_tile_rejects_non_finite_number(tmp_path: Path, bad: str) -> None:
    """Python literals 'nan'/'inf' cannot be parsed by HA's literal_eval."""
    body = f"""\
  cards:
    - type: cardGrid
      sleep_timeout: 0
      entities:
        - entity_id: service.power_off
          service: remote.send_command
          data:
            entity_id: remote.example_remote
            level: {bad}
"""
    result = esphome_config(tmp_path, body, name="service-tile-non-finite")
    assert result.returncode != 0, f"{bad} should be rejected"
    assert "data numbers must be finite" in result.stdout + result.stderr
