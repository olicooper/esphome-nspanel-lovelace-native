"""Codegen regression tests for the nspanel_lovelace component.

Runs ``esphome compile --only-generate`` against the synthetic fixture
``ci/codegen.yaml`` and asserts on the generated ``src/main.cpp``.
No hardware and no secrets: the fixture uses only fictional entities and
hardcoded values. Run inside the approved ESPHome 2026.11.0 runner image.
"""

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

    # Both 'hiking' tiles share one CUSTOM_ICONS slot (ED7E) and reference
    # it twice; built-in icons stay on the enum path.
    assert src.count('u8"\\uED7E"') == 1
    assert "#define CUSTOM_ICONS_SIZE 1" in defines
    assert src.count("CUSTOM_ICONS[0]") == 2

    # The hidden card is marked hidden exactly once.
    assert src.count("set_hidden(true)") == 1

    # Visible navigation ring: with two visible cards both prev and next
    # wrap around, so both card uuids appear as navigation targets.
    assert re.search(r'new nspanel_lovelace::NavigationItem\("[0-9]+", "main_card", ', src)
    assert re.search(r'new nspanel_lovelace::NavigationItem\("[0-9]+", "last_card", ', src)
    # The hidden card is not part of the visible ring but links home to the
    # screensaver (uuid "1", allocated first).
    assert re.search(r'new nspanel_lovelace::NavigationItem\("[0-9]+", "1", ', src)

    # Weather uses the raw entity string, not create_entity.
    assert 'set_weather_entity_id("weather.demo_codegen")' in src
    assert "#define USE_NSPANEL_WEATHER_SERVICE" in defines

    # No Python None token may leak into generated C++ (string literals are
    # legitimate, e.g. the translation value "None", so they are stripped).
    stripped = re.sub(r'"(?:\\.|[^"\\])*"', '""', src)
    assert re.search(r"\bNone\b", stripped) is None
