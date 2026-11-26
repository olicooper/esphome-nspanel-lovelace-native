"""Pin the Task 1 native action-response contract on the synthetic probe.

Runs ESPHome's own validator and code generator against
``ci/weather-native-probe.yaml``: no hardware, no secrets, no live HA.
Run inside the floor runner image (ghcr.io/esphome/esphome:2026.11.0).
"""

import re
import subprocess
from pathlib import Path

import pytest
import yaml

REPO_ROOT = Path(__file__).resolve().parents[1]
PROBE_YAML = REPO_ROOT / "ci" / "weather-native-probe.yaml"
MAIN_CPP = (REPO_ROOT / "ci" / ".esphome" / "build"
            / "weather-native-probe" / "src" / "main.cpp")


class _ConfigLoader(yaml.SafeLoader):
    pass


_ConfigLoader.add_constructor(
    "!lambda", lambda loader, node: loader.construct_scalar(node))


def run_esphome(*args: str) -> str:
    result = subprocess.run(
        ["esphome", *args], cwd=REPO_ROOT, capture_output=True,
        text=True, timeout=900,
    )
    assert result.returncode == 0, (
        f"esphome {' '.join(args)} failed:\n"
        f"{result.stdout[-4000:]}\n{result.stderr[-4000:]}"
    )
    return result.stdout


@pytest.fixture(scope="module")
def probe_stdout() -> str:
    return run_esphome("config", str(PROBE_YAML))


@pytest.fixture(scope="module")
def probe_config(probe_stdout) -> dict:
    return yaml.load(probe_stdout[probe_stdout.index("esphome:\n"):],
                     Loader=_ConfigLoader)


@pytest.fixture(scope="module")
def generated_main() -> str:
    run_esphome("compile", "--only-generate", str(PROBE_YAML))
    assert MAIN_CPP.is_file(), f"missing generated {MAIN_CPP}"
    return MAIN_CPP.read_text()


def test_probe_pins_captured_daily_forecast_action_response(probe_config):
    """Drop capture_response, a handler, or daily type -> Task 1 proof breaks."""
    actions = probe_config["api"]["actions"]
    by_name = {action["action"]: action for action in actions}
    assert set(by_name) == {"weather_probe_success", "weather_probe_error"}

    for name, action in by_name.items():
        assert len(action["then"]) == 1, name
        call = action["then"][0]["homeassistant.action"]
        assert call["action"] == "weather.get_forecasts", name
        assert call["data"]["type"] == "daily", name
        assert call["capture_response"] is True, name
        assert call["on_success"]["then"], name
        assert call["on_error"]["then"], name

    assert (by_name["weather_probe_success"]["then"][0]
            ["homeassistant.action"]["data"]["entity_id"]) == "weather.demo"
    assert (by_name["weather_probe_error"]["then"][0]
            ["homeassistant.action"]["data"]["entity_id"]) == "weather.lab_nonexistent"


def test_probe_has_no_periodic_request_loop(probe_stdout):
    """A periodic trigger would multiply the owner-accepted callback leak."""
    # The dump keeps file order; check the whole dump, not a slice starting at
    # esphome:, so a trigger declared above that key cannot hide.
    for key in ("interval", "cron", "at"):
        assert not re.search(rf"^{key}:", probe_stdout, re.MULTILINE), key


def test_generated_probe_pins_native_response_signature(generated_main):
    """An ESPHome change to the borrowed JsonObjectConst callback breaks CI."""
    for fragment in ("HomeAssistantServiceCallAction<>",
                     "StatelessLambdaAction<ArduinoJson::JsonObjectConst>"):
        assert fragment in generated_main, fragment
    for fragment, expected in [
        ("set_wants_response()", 2),
        ("get_success_trigger_with_response()", 2),
        ("get_error_trigger()", 2),
        (") Automation<std::string>(", 2),
    ]:
        assert generated_main.count(fragment) == expected, fragment
    for name in ("weather_probe_success", "weather_probe_error"):
        assert f'"{name}"' in generated_main, name
