"""Bundled data-file handling for nspanel_lovelace: the icon catalog and
locale translation files. Leaf module: must not import from __init__.py."""
import json
import logging
import os
import re
from pathlib import Path
from typing import Callable

import esphome.config_validation as cv

_LOGGER = logging.getLogger(__name__)

CONF_LOCALE = "locale"
CONF_LANGUAGE = "language"

REQUIRED_TRANSLATION_KEYS = [
    "none","unknown","preset_mode","swing_mode","fan_mode","activity","away","boost",
    "comfort","eco","home","sleep","cool","cooling","dry","drying","fan","heat","heating",
    "heat_cool","idle","auto","fan_only","on","off","currently","state","action","lock","unlock",
    "paused","active","activate","press","run","speed","brightness","color","color_temp",
    "position","start","pause","cancel","finish","arm_home","arm_away","arm_night","arm_vacation",
    "arm_custom_bypass","armed_home","armed_away","armed_night","armed_vacation","armed_custom_bypass",
    "arming","disarming","disarmed","pending","triggered","disarm","tilt_position",
    "above_horizon","below_horizon","not_home","start_cleaning","return_to_base","docked",
    "turn_on","turn_off","month_january","month_jan","month_february","month_feb","month_march","month_mar",
    "month_april","month_apr","month_may","month_june","month_jun","month_july","month_jul","month_august",
    "month_aug","month_september","month_sep","month_october","month_oct","month_november","month_nov",
    "month_december","month_dec","dow_sunday","dow_sun","dow_monday","dow_mon","dow_tuesday","dow_tue",
    "dow_wednesday","dow_wed","dow_thursday","dow_thu","dow_friday","dow_fri","dow_saturday","dow_sat"
]

ICON_HEX_RE = re.compile(r"^[0-9A-Fa-f]{4}$")

iconJson = None

def load_icons():
    """Load the bundled icons.json catalog. The catalog must be a nonempty list
    of objects with a nonempty string 'name' and a 4-character hex 'hex'."""
    global iconJson
    current_directory = os.path.dirname(__file__)
    iconJsonPath = os.path.join(current_directory, 'icons.json')
    _LOGGER.debug(f"[nspanel_lovelace] Attempting to load icons from '{iconJsonPath}'")
    try:
        with open(iconJsonPath, encoding="utf-8") as read_file:
            iconJson = json.load(read_file)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as err:
        raise cv.Invalid(f"Failed to load icons catalog from '{iconJsonPath}': {err}")
    if not isinstance(iconJson, list) or len(iconJson) == 0:
        raise cv.Invalid(f"Icons catalog is empty: '{iconJsonPath}'")
    for entry in iconJson:
        if (not isinstance(entry, dict)
                or not isinstance(entry.get("name"), str) or not entry["name"]
                or not isinstance(entry.get("hex"), str) or not ICON_HEX_RE.match(entry["hex"])):
            raise cv.Invalid(
                f"Icons catalog entry is invalid (need nonempty string 'name' and a "
                f"4-character hex 'hex'): {entry!r}. File location: {iconJsonPath}")
    _LOGGER.info(f"[nspanel_lovelace] Loaded {str(len(iconJson))} icons")

def resolve_translation_path(language: str, config_relative: Callable[[str], Path]) -> tuple[Path, bool]:
    """Resolve a locale language to its translation file.

    Bundled languages (no ".json" suffix) resolve inside the bundled
    translations/ directory. A ".json" language resolves relative to the
    top-level configuration directory (config_relative), or unchanged when
    absolute. When the config-relative path does not exist, the legacy
    working-directory path is returned instead and the boolean is True
    (never for absolute paths or bundled languages).
    """
    if not language.endswith(".json"):
        return Path(__file__).resolve().parent / "translations" / f"{language}.json", False
    if os.path.isabs(language):
        return Path(language), False
    config_path = Path(config_relative(language))
    if config_path.exists():
        return config_path, False
    legacy_path = Path(os.path.abspath(language))
    if legacy_path.exists():
        return legacy_path, True
    return config_path, False

def load_translations(path) -> dict:
    """Load a translation file and validate its shape: a JSON object mapping
    string keys to string values containing every REQUIRED_TRANSLATION_KEY.
    Extra string-valued keys are allowed and preserved (insertion order)."""
    try:
        with open(path, encoding="utf-8") as read_file:
            data = json.load(read_file)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as err:
        raise cv.Invalid(f"Failed to load translation file from '{path}': {err}", [CONF_LOCALE, CONF_LANGUAGE])
    if not isinstance(data, dict):
        raise cv.Invalid(
            f"Translation file '{path}' must be a JSON object of string keys to string values",
            [CONF_LOCALE, CONF_LANGUAGE])
    missingKeys = [k for k in REQUIRED_TRANSLATION_KEYS if k not in data]
    if missingKeys:
        raise cv.Invalid(
            f"Translation file '{path}' is missing the following required keys: {missingKeys}",
            [CONF_LOCALE, CONF_LANGUAGE])
    badKeys = [k for k, v in data.items() if not isinstance(v, str)]
    if badKeys:
        raise cv.Invalid(
            f"Translation file '{path}' has non-string values for the following keys: {badKeys}",
            [CONF_LOCALE, CONF_LANGUAGE])
    _LOGGER.info(f"[nspanel_lovelace] Loaded translation file '{path}' ({len(data)} entries)")
    return data
