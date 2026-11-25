"""Unit tests for the nspanel_lovelace has_build_define helper.

A flag is set when a build-flag entry is exactly ``-D<name>`` or
``-D<name>=...`` (no substring matching). Both the modern
``esphome.build_flags`` location and the legacy
``platformio_options.build_flags`` location are checked. A scalar value is
a single entry (ESPHome's floor behaviour), not split on whitespace.

Run inside the approved ESPHome 2026.11.0 runner image.
"""

import importlib
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parent.parent


def load_component():
    if str(REPO_ROOT) not in sys.path:
        sys.path.insert(0, str(REPO_ROOT))
    return importlib.import_module("components.nspanel_lovelace")


@pytest.mark.parametrize("esphome_config, name, expected", [
    # modern location: esphome.build_flags
    ({"build_flags": ["-DTEST_DEVICE_MODE"]}, "TEST_DEVICE_MODE", True),
    ({"build_flags": ["-DTEST_DEVICE_MODE=1"]}, "TEST_DEVICE_MODE", True),
    ({"build_flags": ["-DTEST_DEVICE_MODE_EXTRA"]}, "TEST_DEVICE_MODE", False),
    ({"build_flags": ["-DOTHER"]}, "TEST_DEVICE_MODE", False),
    ({"build_flags": []}, "TEST_DEVICE_MODE", False),
    ({"build_flags": ""}, "TEST_DEVICE_MODE", False),
    ({"build_flags": "-DTEST_DEVICE_MODE"}, "TEST_DEVICE_MODE", True),
    # a scalar is one entry (no whitespace splitting)
    ({"build_flags": "-DTEST_DEVICE_MODE -DOTHER"}, "TEST_DEVICE_MODE", False),
    # legacy location: platformio_options.build_flags, list or scalar
    ({"platformio_options": {"build_flags": ["-DTEST_DEVICE_MODE"]}}, "TEST_DEVICE_MODE", True),
    ({"platformio_options": {"build_flags": "-DTEST_DEVICE_MODE=1"}}, "TEST_DEVICE_MODE", True),
    ({"platformio_options": {"build_flags": "-DTEST_DEVICE_MODE_EXTRA"}}, "TEST_DEVICE_MODE", False),
    ({"platformio_options": {}}, "TEST_DEVICE_MODE", False),
    # empty / missing mappings
    ({}, "TEST_DEVICE_MODE", False),
    # the same rules apply to other flag names
    ({"build_flags": ["-DDISABLE_NSPANEL_TFT_UPLOAD"]}, "DISABLE_NSPANEL_TFT_UPLOAD", True),
    ({"platformio_options": {"build_flags": ["-DDISABLE_NSPANEL_TFT_UPLOAD"]}}, "DISABLE_NSPANEL_TFT_UPLOAD", True),
    ({"build_flags": ["-DDISABLE_NSPANEL_TFT_UPLOAD_EXTRA"]}, "DISABLE_NSPANEL_TFT_UPLOAD", False),
])
def test_nspanel_build_flags(esphome_config, name, expected) -> None:
    nsl = load_component()
    assert nsl.has_build_define(esphome_config, name) is expected
