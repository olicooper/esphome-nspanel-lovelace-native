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
import re
import subprocess
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent
COMPONENTS_DIR = REPO_ROOT / "components"


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
    timezone: Europe/London
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
