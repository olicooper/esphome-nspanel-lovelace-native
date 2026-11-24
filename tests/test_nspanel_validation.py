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
import subprocess
import sys
from pathlib import Path

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
