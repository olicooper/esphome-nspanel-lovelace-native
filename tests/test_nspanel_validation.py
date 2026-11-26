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


def test_weather_entity_domain_by_forecast_mode(tmp_path: Path) -> None:
    """The weather entity domain depends on forecast_method: the documented
    legacy 'template_sensor' mode accepts sensor.* (README fallback) and
    weather.* (test.yaml), while 'service' mode requires weather.*."""
    def weather_cfg(eid: str, mode: str) -> str:
        return f"""\
  screensaver:
    weather:
      entity_id: {eid}
      forecast_method: {mode}
"""
    for eid, mode in (
        ("sensor.forecast_daily", "template_sensor"),  # documented legacy path
        ("weather.forecast_home", "template_sensor"),  # test.yaml shape
        ("weather.forecast_home", "service"),
    ):
        result = esphome_config(tmp_path, weather_cfg(eid, mode), name=f"w-ok-{mode[:4]}")
        assert result.returncode == 0, (
            f"{eid} + forecast_method {mode} should validate:\n{result.stdout}"
        )
    result = esphome_config(
        tmp_path, weather_cfg("sensor.forecast_daily", "service"), name="w-bad"
    )
    assert result.returncode != 0
    assert "weather entity" in result.stdout + result.stderr


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
