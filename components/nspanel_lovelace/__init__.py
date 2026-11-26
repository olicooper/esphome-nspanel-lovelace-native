from esphome import automation
import esphome.config_validation as cv
import esphome.codegen as cg
import esphome.core as core
import logging
import math
import re
from typing import Union
from pathlib import Path
from fractions import Fraction

from esphome.components import uart, time, esp32, api as esphome_api, sun
from esphome.components.color import hex_color
from esphome.cpp_generator import StringLiteral
from esphome.const import (
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_TIME_ID,
    CONF_ESPHOME,
    CONF_BUILD_FLAGS,
    CONF_PLATFORMIO_OPTIONS
)

from . import resources
from .resources import (
    CONF_LOCALE,
    CONF_LANGUAGE,
    load_icons,
    load_translations,
    resolve_translation_path,
)

DEPENDENCIES = ["uart", "time", "wifi", "api", "esp32", "json"]
CONFLICTS_WITH = ["psram"]
# The HMI protocol caps timeout~ at 65 seconds. Longer idle periods are timed
# by the ESP32 instead: the component sends timeout~0 and hands sleep back to
# the TFT with timeout~1 when the configured deadline is reached.
# https://github.com/joBr99/nspanel-lovelace-ui/blob/22e96f2b3ad0cd3382008eac9b4d6a27982404b8/HMI/README.md?plain=1#L91
MAX_SLEEP_TIMEOUT = 65535  # uint16_t payload in page_base.h

def AUTO_LOAD():
    val = ["text_sensor", "json"]
    return val

_LOGGER = logging.getLogger(__name__)

make_shared = cg.std_ns.class_("make_shared")
unique_ptr = cg.std_ns.class_("unique_ptr")
nspanel_lovelace_ns = cg.esphome_ns.namespace("nspanel_lovelace")
NSPanelLovelace = nspanel_lovelace_ns.class_("NSPanelLovelace", cg.Component, uart.UARTDevice)
TRANSLATION_ITEM = nspanel_lovelace_ns.enum("translation_item", True)
icon_t = nspanel_lovelace_ns.enum("icon_t", True)

ALARM_ARM_ACTION = nspanel_lovelace_ns.enum("alarm_arm_action", True)
ALARM_ARM_OPTIONS = ['arm_home','arm_away','arm_night','arm_vacation','arm_custom_bypass']
ALARM_ARM_DEFAULT_OPTIONS = ALARM_ARM_OPTIONS[:4]

TEMPERATURE_UNIT = nspanel_lovelace_ns.enum("temperature_unit_t", True)
TEMPERATURE_UNIT_OPTIONS = ['celcius','fahrenheit']
TEMPERATURE_UNIT_OPTION_MAP = {
    'celcius': TEMPERATURE_UNIT.celcius,
    'fahrenheit': TEMPERATURE_UNIT.fahrenheit,
}

NSPanelLovelaceMsgIncomingTrigger = nspanel_lovelace_ns.class_(
    "NSPanelLovelaceMsgIncomingTrigger",
    automation.Trigger.template(cg.std_string)
)

# Real HA entity id format: [domain].[name]. The pseudo-entities 'delete',
# 'iText.[text]' and 'navigate.[card id]' are handled separately in valid_entity_id.
ENTITY_ID_RE = re.compile(r"^[\w]+[A-Za-z0-9]\.[\w]+[A-Za-z0-9]$")
SERVICE_RE = re.compile(r"^[a-z_][a-z0-9_]*\.[a-z_][a-z0-9_]*$")
## The list of currently supported entity domains (exact match)
ENTITY_TYPES = [
    'sensor','binary_sensor','light','switch','scene','timer','weather',
    'alarm_control_panel','input_boolean','button','input_button','cover','fan',
    'automation','script','climate','media_player','select','input_select',
    'number','input_number','text','input_text','lock','sun','person','vacuum',
    'service'
]
BUILTIN_ICON_MAP: list = [
    ["E003",icon_t.account],["F098",icon_t.air_humidifier],["E027",icon_t.alert_circle],["E5D5",icon_t.alert_circle_outline],
    ["E041",icon_t.arrow_bottom_left],["E84B",icon_t.arrow_collapse_horizontal],["E044",icon_t.arrow_down],["E84D",icon_t.arrow_expand_horizontal],
    ["E730",icon_t.arrow_left_bold],["E733",icon_t.arrow_right_bold],["E05B",icon_t.arrow_top_right],["E05C",icon_t.arrow_up],
    ["E736",icon_t.arrow_up_bold],["E078",icon_t.battery],["E083",icon_t.battery_charging],["E08D",icon_t.battery_outline],
    ["E09D",icon_t.bell_ring],["E0AB",icon_t.blinds],["F010",icon_t.blinds_open],["E0DD",icon_t.brightness_5],["E0DF",icon_t.brightness_7],
    ["E0EC",icon_t.calendar],["E0EF",icon_t.calendar_clock],["EE8D",icon_t.calendar_sync],["E113",icon_t.cash],["EC4F",icon_t.chart_bell_curve],
    ["E5DF",icon_t.check_circle],["EC53",icon_t.check_network_outline],["E12E",icon_t.checkbox_blank_circle],["E132",icon_t.checkbox_marked_circle],
    ["EAA4",icon_t.circle_slice_8],["EC5E",icon_t.close_network_outline],["E1A0",icon_t.crop_portrait],["E5E6",icon_t.cursor_text],
    ["F845",icon_t.curtains],["F846",icon_t.curtains_closed],["E81A",icon_t.door_closed],["E81B",icon_t.door_open],["E20F",icon_t.fan],
    ["E237",icon_t.fire],["E240",icon_t.flash],["E69D",icon_t.format_color_text],["E6D8",icon_t.garage],["E6D9",icon_t.garage_open],
    ["E646",icon_t.gas_cylinder],["E298",icon_t.gate],["F169",icon_t.gate_open],["E299",icon_t.gauge],["F2A7",icon_t.gesture_tap_button],
    ["E624",icon_t.help_circle_outline],["E2DB",icon_t.home],["E6A0",icon_t.home_outline],["E97D",icon_t.light_switch],["E334",icon_t.lightbulb],
    ["ED1A",icon_t.link_box_outline],["E33D",icon_t.lock],["E33E",icon_t.lock_open],["ED90",icon_t.motion_sensor],["F434",icon_t.motion_sensor_off],
    ["E380",icon_t.movie],["E759",icon_t.music],["E386",icon_t.music_note],["E389",icon_t.music_note_off],["E3CA",icon_t.open_in_app],
    ["E3D2",icon_t.package],["E3D4",icon_t.package_up],["E3D7",icon_t.palette],["E3E3",icon_t.pause],["E409",icon_t.play],
    ["ECB7",icon_t.playlist_music],["E410",icon_t.playlist_play],["EDF1",icon_t.playlist_star],["E424",icon_t.power],["E6A4",icon_t.power_plug],
    ["E6A5",icon_t.power_plug_off],["ECBB",icon_t.progress_alert],["E43C",icon_t.radiobox_blank],["E444",icon_t.ray_vertex],
    ["E6A8",icon_t.robot],["E70C",icon_t.robot_vacuum],["EBC1",icon_t.script_text],["E497",icon_t.shield],["E6BA",icon_t.shield_airplane],
    ["E689",icon_t.shield_home],["E99C",icon_t.shield_lock],["F827",icon_t.shield_moon],["E99D",icon_t.shield_off],["E49C",icon_t.shuffle],
    ["E49D",icon_t.shuffle_disable],["E4A1",icon_t.signal],["EA70",icon_t.smog],["E391",icon_t.smoke_detector],["F92D",icon_t.smoke_detector_alert],
    ["F80A",icon_t.smoke_detector_variant],["F92F",icon_t.smoke_detector_variant_alert],["E716",icon_t.snowflake],["E4C3",icon_t.speaker_off],
    ["E763",icon_t.square],["E762",icon_t.square_outline],["E4DA",icon_t.stop],["E503",icon_t.temperature_celsius],["E504",icon_t.temperature_fahrenheit],
    ["E50E",icon_t.thermometer],["F3AA",icon_t.timer],["E51A",icon_t.timer_outline],["E565",icon_t.vibrate],["E566",icon_t.video],
    ["E58B",icon_t.water],["E58C",icon_t.water_off],["E58D",icon_t.water_percent],["E58F",icon_t.weather_cloudy],["E590",icon_t.weather_fog],
    ["E591",icon_t.weather_hail],["E592",icon_t.weather_lightning],["E67D",icon_t.weather_lightning_rainy],["E593",icon_t.weather_night],
    ["E594",icon_t.weather_partly_cloudy],["EF34",icon_t.weather_partly_snowy_rainy],["E595",icon_t.weather_pouring],["E596",icon_t.weather_rainy],
    ["E597",icon_t.weather_snowy],["E598",icon_t.weather_sunny],["E59A",icon_t.weather_sunset_down],["E59B",icon_t.weather_sunset_up],
    ["E59C",icon_t.weather_windy],["E59D",icon_t.weather_windy_variant],["E5AD",icon_t.window_closed],["E5B0",icon_t.window_open],
    ["F11B",icon_t.window_shutter],["F11D",icon_t.window_shutter_open]
]

CONF_INCOMING_MSG = "on_incoming_msg"
CONF_ICON = "icon"
CONF_ICON_VALUE = "value"
CONF_ICON_COLOR = "color"
CONF_ENTITY_ID = "entity_id"
CONF_SLEEP_TIMEOUT = "sleep_timeout"
CONF_DEFAULT_CARD = "default_card"
CONF_EXP_VERT_UI = "experimental_vertical_ui"

CONF_TEMPERATURE_UNIT = "temperature_unit"

CONF_SCREENSAVER = "screensaver"
CONF_MODEL = "model"
CONF_SCREENSAVER_DATE_FORMAT = "date_format"
CONF_SCREENSAVER_TIME_FORMAT = "time_format"
CONF_SCREENSAVER_WEATHER = "weather"
CONF_SCREENSAVER_WEATHER_RESTART_AFTER_STALL = "restart_after_forecast_stall"
CONF_SCREENSAVER_STATUS_ICON_LEFT = "status_icon_left"
CONF_SCREENSAVER_STATUS_ICON_RIGHT = "status_icon_right"
CONF_SCREENSAVER_STATUS_ICON_ALT_FONT = "alt_font" # todo: to_code
CONF_SCREENSAVER_DOUBLE_TAP_TO_UNLOCK = "double_tap_to_unlock"
CONF_SCREENSAVER_WEATHER_ICON_COLOR = "icon_color"

CONF_DISPLAY = "display"
CONF_ACTIVE_BRIGHTNESS = "active_brightness"
CONF_INACTIVE_BRIGHTNESS = "inactive_brightness"
CONF_SCHEDULE = "schedule"
CONF_AT = "at"
CONF_SUN_ID = "sun_id"

CONF_CARDS = "cards"
CONF_CARD_TYPE = "type"
CONF_CARD_HIDDEN = "hidden"
CONF_CARD_TITLE = "title"
CONF_CARD_ENTITIES = "entities"
CONF_CARD_ENTITIES_NAME = "name"
CONF_SERVICE = "service"
CONF_DATA = "data"

CARD_ENTITIES="cardEntities"
CARD_GRID="cardGrid"
CARD_GRID2="cardGrid2"
CARD_QR="cardQR"
CARD_ALARM="cardAlarm"
CARD_THERMO="cardThermo"
CARD_MEDIA="cardMedia"
CARD_POWER="cardPower"
CARD_TYPE_OPTIONS = [CARD_ENTITIES, CARD_GRID, CARD_GRID2, CARD_QR, CARD_ALARM, CARD_THERMO, CARD_MEDIA, CARD_POWER]

CONF_CARD_QR_TEXT = "qr_text"
CONF_CARD_ALARM_ENTITY_ID = "alarm_entity_id"
CONF_CARD_ALARM_SUPPORTED_MODES = "supported_modes"
CONF_CARD_THERMO_ENTITY_ID = "thermo_entity_id"
CONF_CARD_MEDIA_ENTITY_ID = "media_entity_id"
CONF_CARD_POWER_HOME_ENTITY_ID = "home_entity_id"

def has_build_define(esphome_config: dict, name: str) -> bool:
    """True when a build flag defines ``name``.

    Flag entries are shell-lexed the way ESPHome's floor version (2026.11.0)
    lexes them before compiling (esphome.platformio.library.lex_build_flags):
    one entry may hold several whitespace-separated flags, and a bare ``-D``
    is re-glued with its argument. A lexed token defines ``name`` when it is
    exactly ``-D<name>`` or ``-D<name>=...`` - no substring matching. Checks
    the modern ``esphome.build_flags`` location and the legacy
    ``platformio_options.build_flags`` location (list or scalar).
    """
    from esphome.platformio.library import lex_build_flags
    locations = [esphome_config.get(CONF_BUILD_FLAGS)]
    pio = esphome_config.get(CONF_PLATFORMIO_OPTIONS)
    if isinstance(pio, dict):
        locations.append(pio.get("build_flags"))
    for loc in locations:
        if not loc:
            continue
        for token in lex_build_flags(loc, "esphome.build_flags"):
            if token == f"-D{name}" or token.startswith(f"-D{name}="):
                return True
    return False

def get_icon_hex(iconLookup: str) -> Union[str, None]:
    if not iconLookup or len(iconLookup) == 0:
        return None
    searchHex = False
    if iconLookup.startswith("hex"):
        iconLookup = iconLookup[4:]
        searchHex = True
    _LOGGER.debug(f"Finding icon: '{iconLookup}'")
    if resources.iconJson is None:
        load_icons()
    if resources.iconJson is not None:
        key = 'hex' if searchHex else 'name'
        for attrs in resources.iconJson:
            if attrs[key] == iconLookup:
                return attrs['hex'].upper()
    return None

def valid_icon_value(value):
    if isinstance(value, str):
        if isinstance(get_icon_hex(value), str):
            return value
        raise cv.Invalid(f"Icon '{value}' not found! Valid example 'weather-sunny' or 'hex:E598'")
    raise cv.Invalid(
        f"Must be a string, got {value}. did you forget putting quotes around the value?"
    )

def valid_uuid(value):
    """Validate that a given config value is a valid uuid."""
    value = cv.string(value)
    if not value:
        return value
    if len(value) <= 30 and re.match(r"^[\w\_]+[\w]$", value):
        return value
    raise cv.Invalid(
        f'Value must be 30 characters or less in length and contain only numbers, letters and underscores e.g. "living_room_light_1"'
    )

def valid_entity_id(entity_type_allowlist: list[str] | None = None, *,
                    allow_navigation: bool = False,
                    allow_text: bool = False,
                    allow_delete: bool = False):
    """Validate that a given entity_id is correctly formatted and present in the entity type
    allowlist. The pseudo-entities 'navigate.[card id]', 'iText.[text]' and 'delete' are only
    accepted where explicitly allowed (generic card tiles)."""
    allowed = ENTITY_TYPES if entity_type_allowlist is None else entity_type_allowlist

    def validator(value):
        value = cv.string_strict(value)
        if not value:
            raise cv.Invalid("entity_id must not be empty")
        if value == 'delete':
            if allow_delete:
                return value
            raise cv.Invalid(
                f'the special "delete" entity is not allowed here. Allowed types are: {allowed}'
            )
        if value.startswith('iText.'):
            if not allow_text:
                raise cv.Invalid(
                    f'iText entities are not allowed here. Allowed types are: {allowed}'
                )
            if '~' in value:
                raise cv.Invalid(
                    f'entity_id "{value}" must match the format "iText.[text to display]" and '
                    'not contain the character "~" (tilde)'
                )
            return value
        if not ENTITY_ID_RE.match(value):
            raise cv.Invalid(
                f'entity_id "{value}" must match the format "[entity type].[entity name]" and '
                'contain only numbers (0-9), letters (A-Z) and underscores (_), '
                'e.g. "light.living_room_light_1".'
            )
        domain = value.split('.', 1)[0]
        if domain == 'navigate':
            if allow_navigation:
                return value
            raise cv.Invalid(
                f'the special "navigate" entity is not allowed here. Allowed types are: {allowed}'
            )
        if domain in allowed:
            return value
        raise cv.Invalid(
            f"entity type '{domain}' is not allowed here. Allowed types are: {allowed}"
        )
    return validator

def valid_service_name(value):
    """HA service names are lowercase dotted domain.service strings."""
    value = cv.string_strict(value)
    if not SERVICE_RE.fullmatch(value):
        raise cv.Invalid('service must match "domain.service_name"')
    return value

def valid_service_data(value):
    """Allow only values that can be sent through the HA action API."""
    if not isinstance(value, dict):
        raise cv.Invalid("data values must be scalars or lists/dicts of scalars")

    def check(item):
        if isinstance(item, float) and not math.isfinite(item):
            raise cv.Invalid("data numbers must be finite")
        if isinstance(item, (str, bool, int, float)):
            return True
        if isinstance(item, (list, tuple)):
            return all(check(element) for element in item)
        if isinstance(item, dict):
            return all(isinstance(key, str) and check(element)
                       for key, element in item.items())
        return False

    for key, item in value.items():
        if not isinstance(key, str):
            raise cv.Invalid("data keys must be strings")
        if not check(item):
            raise cv.Invalid("data values must be scalars or lists/dicts of scalars")
    return value

def validate_service_tile(config):
    """service: and data: belong to service.* tiles and travel together."""
    service = config.get(CONF_SERVICE)
    data = config.get(CONF_DATA)
    entity_id = config.get(CONF_ENTITY_ID, "")
    if service is not None:
        if not entity_id.startswith("service."):
            raise cv.Invalid("service is only valid on a service.* tile")
        target = data.get(CONF_ENTITY_ID) if isinstance(data, dict) else None
        if not isinstance(target, str) or not target:
            raise cv.Invalid("data.entity_id is required for service tiles")
    elif data is not None:
        raise cv.Invalid("data is only valid with service")
    return config

def valid_clock_format(property_name):
    def validator(value):
        value = cv.string_strict(value)
        if not value:
            return value
        if len(value) < 2:
            raise cv.Invalid(f"{property_name} must be at least 2 characters long")
        if len(value) > 32:
            raise cv.Invalid(f"{property_name} must be at most 32 characters long")
        return value
    return validator

def has_card_type(config):
    card_type = config.get(CONF_CARD_TYPE, None)
    if card_type is None:
        raise cv.Invalid(f"type is required for cards, options are {CARD_TYPE_OPTIONS}")
    return config

def ensure_unique(value: list):
    all_values = list(value)
    unique_values = set(value)
    if len(all_values) != len(unique_values):
        raise cv.Invalid("Mapping values must be unique.")
    return value

SCHEMA_LOCALE = cv.Schema({
    cv.Optional(CONF_TEMPERATURE_UNIT): cv.one_of(*TEMPERATURE_UNIT_OPTIONS),
    cv.Optional(CONF_LANGUAGE, default='en'): cv.string_strict,
})

def valid_color(value):
    """RGB565 int, [r, g, b] list or hex string ("#RRGGBB" / "RRGGBB"), returned as RGB565."""
    if isinstance(value, bool):
        raise cv.Invalid("Color must be a number, [r, g, b] list or hex string")
    if isinstance(value, list):
        if len(value) != 3:
            raise cv.Invalid("RGB color must have 3 values, e.g. [255, 255, 255]")
        r, g, b = (cv.uint8_t(v) for v in value)
    elif isinstance(value, str):
        if re.fullmatch(r"#?[0-9A-Fa-f]{6}", value):
            r, g, b = hex_color(value.lstrip("#"))
        else:
            # Quoted RGB565 decimal, e.g. "52857" from the colour picker docs.
            return cv.int_range(0, 65535)(value)
    else:
        return cv.int_range(0, 65535)(value)
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

SCHEMA_ICON = cv.Any(
    valid_icon_value, # icon name
    cv.Schema({
        cv.Optional(CONF_ICON_VALUE): valid_icon_value,
        cv.Optional(CONF_ICON_COLOR): valid_color,
    })
)

SCHEMA_STATUS_ICON = cv.Schema({
    # The StatusIconItem constructors all require an entity, so a provided
    # status icon block without an entity_id is a config error, not a codegen one.
    cv.Required(CONF_ENTITY_ID): valid_entity_id(['sensor','binary_sensor','light','lock']),
    cv.Optional(CONF_ICON): SCHEMA_ICON,
    cv.Optional(CONF_SCREENSAVER_STATUS_ICON_ALT_FONT): cv.boolean,
})

_RESTART_DURATION_UNITS = {
    "ns": "nanoseconds", "us": "microseconds", "\u00b5s": "microseconds",
    "ms": "milliseconds", "s": "seconds", "sec": "seconds", "min": "minutes",
    "h": "hours", "d": "days",
}
_RESTART_DURATION_MS = {
    "nanoseconds": Fraction(1, 1000000), "microseconds": Fraction(1, 1000),
    "milliseconds": 1, "seconds": 1000, "minutes": 60000,
    "hours": 3600000, "days": 86400000,
}


def valid_weather_stall_restart(value):
    # ESPHome's TimePeriod rounds each unit before carrying its fraction to
    # the next unit. Keep the input exact until the final millisecond check.
    if isinstance(value, str):
        match = re.fullmatch(r"([+-]?(?:[0-9]+\.?[0-9]*|\.[0-9]+))\s*(\w+)", value)
        if match:
            amount, unit = match.groups()
            parts = {_RESTART_DURATION_UNITS.get(unit, unit): amount}
        else:
            clock = value.split(":")
            parts = dict(zip(("hours", "minutes", "seconds"), clock)) if len(clock) in (2, 3) and all(
                re.fullmatch(r"[+-]?[0-9]+", part) for part in clock
            ) else {}
    else:
        parts = value if isinstance(value, dict) else {}
    try:
        if not parts:
            raise ValueError
        amounts = []
        for unit, amount in parts.items():
            # YAML floats have already lost their original decimal precision.
            if not re.fullmatch(r"[+-]?(?:[0-9]+\.?[0-9]*|\.[0-9]+)", str(amount)) or isinstance(amount, float):
                raise ValueError
            amounts.append((Fraction(str(amount)), _RESTART_DURATION_MS[unit]))
    except (ValueError, KeyError, OverflowError, ZeroDivisionError) as err:
        raise cv.Invalid("Expected a forecast stall restart duration with a time unit") from err
    milliseconds = sum(amount * scale for amount, scale in amounts)
    if any(amount < 0 for amount, _ in amounts) or milliseconds > 0xFFFFFFFF:
        raise cv.Invalid("Forecast stall restart duration must be between 0 and 4294967295 ms")
    if milliseconds != int(milliseconds):
        raise cv.Invalid("Forecast stall restart duration must use whole milliseconds")
    return core.TimePeriodMilliseconds(milliseconds=int(milliseconds))


def valid_screensaver_weather(config):
    """Native weather retrieval needs one Home Assistant weather entity."""
    return cv.Schema({
        cv.Required(CONF_ENTITY_ID): valid_entity_id(['weather']),
        cv.Optional(CONF_SCREENSAVER_WEATHER_RESTART_AFTER_STALL, default="0s"): valid_weather_stall_restart,
        cv.Optional(CONF_SCREENSAVER_WEATHER_ICON_COLOR): valid_color,
    })(config)

SCHEMA_SCREENSAVER = cv.Schema({
    cv.Optional(CONF_ID): valid_uuid,
    cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Optional(CONF_SCREENSAVER_DATE_FORMAT, default="%A, %d. %B %Y"): valid_clock_format('Date format'),
    cv.Optional(CONF_SCREENSAVER_TIME_FORMAT, default="%H:%M"): valid_clock_format('Time format'),
    cv.Optional(CONF_SCREENSAVER_DOUBLE_TAP_TO_UNLOCK, default=False): cv.boolean,
    cv.Optional(CONF_SCREENSAVER_WEATHER): valid_screensaver_weather,
    cv.Optional(CONF_SCREENSAVER_STATUS_ICON_LEFT): SCHEMA_STATUS_ICON,
    cv.Optional(CONF_SCREENSAVER_STATUS_ICON_RIGHT): SCHEMA_STATUS_ICON,
})


def valid_display_at(value):
    """Accept 'sunrise', 'sunset', or an HH:MM:SS fixed time."""
    if isinstance(value, str):
        lower = value.strip().lower()
        if lower in ("sunrise", "sunset"):
            return lower
    try:
        return cv.time_of_day(value)
    except cv.Invalid:
        pass
    raise cv.Invalid(
        "display schedule 'at' must be 'sunrise', 'sunset', or an HH:MM:SS time"
    )


SCHEMA_DISPLAY_SCHEDULE_ITEM = cv.Schema({
    cv.Required(CONF_AT): valid_display_at,
    cv.Required(CONF_INACTIVE_BRIGHTNESS): cv.int_range(min=0, max=100),
})

SCHEMA_DISPLAY = cv.Schema({
    cv.Optional(CONF_ACTIVE_BRIGHTNESS, default=100): cv.int_range(min=0, max=100),
    cv.Optional(CONF_INACTIVE_BRIGHTNESS, default=50): cv.int_range(min=0, max=100),
    cv.Optional(CONF_SCHEDULE, default=[]): cv.ensure_list(SCHEMA_DISPLAY_SCHEDULE_ITEM),
    cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Optional(CONF_SUN_ID): cv.use_id(sun.Sun),
})

SCHEMA_CARD_ENTITY = cv.All(cv.Schema({
    cv.Required(CONF_ENTITY_ID): valid_entity_id(allow_navigation=True, allow_text=True, allow_delete=True),
    cv.Optional(CONF_CARD_ENTITIES_NAME): cv.string,
    cv.Optional(CONF_ICON): SCHEMA_ICON,
    cv.Optional(CONF_SERVICE): valid_service_name,
    cv.Optional(CONF_DATA): valid_service_data,
}), validate_service_tile)

SCHEMA_CARD_BASE = cv.Schema({
    cv.Optional(CONF_ID): valid_uuid,
    cv.Optional(CONF_CARD_TITLE): cv.string,
    cv.Optional(CONF_CARD_HIDDEN, default=False): cv.boolean,
    # Timeout range 0..MAX_SLEEP_TIMEOUT seconds (0 disables the screensaver)
    cv.Optional(CONF_SLEEP_TIMEOUT, default=10): cv.int_range(0, MAX_SLEEP_TIMEOUT)
})

def get_card_entities_length_limits(card_type: str, model: str = 'eu') -> list[int]:
    if (card_type == CARD_ENTITIES):
        return [1,4] if model in ['eu','us-l'] else [1,6]
    if (card_type == CARD_GRID):
        return [1,6]
    if (card_type == CARD_GRID2):
        return [1,8]
    if (card_type == CARD_QR):
        return [1,2]
    if (card_type == CARD_MEDIA):
        return [0,8]
    if (card_type == CARD_POWER):
        return [0,6]
    return [0,0]

def validate_config(config):
    if core.CORE.using_arduino:
        raise cv.Invalid(
            "nspanel_lovelace requires the ESP-IDF framework (framework: type: esp-idf). "
            "Arduino is not supported since the NSPanel's PSRAM is on non-standard pins "
            "that can't be configured under Arduino."
        )
    model = config[CONF_MODEL]
    # The locale schema defaults language to 'en', so an explicit empty value
    # is the only bad case. Validate the translation file early so a missing
    # file, bad JSON or missing keys fail config validation (at
    # locale.language), not codegen.
    language = config[CONF_LOCALE][CONF_LANGUAGE]
    if not language:
        raise cv.Invalid("A language must be specified in locale", [CONF_LOCALE, CONF_LANGUAGE])
    translation_path, legacy = resolve_translation_path(language, core.CORE.relative_config_path)
    if legacy:
        _LOGGER.warning(
            f"[nspanel_lovelace] translation file '{language}' not found relative to the "
            f"configuration directory; using the legacy working-directory path "
            f"'{translation_path}'. Place the file next to the configuration instead.")
    load_translations(translation_path)

    if CONF_DISPLAY in config:
        display = config[CONF_DISPLAY]
        active = display[CONF_ACTIVE_BRIGHTNESS]
        inactive = display[CONF_INACTIVE_BRIGHTNESS]
        if inactive >= active:
            raise cv.Invalid(
                f"display.inactive_brightness ({inactive}) must be less than "
                f"display.active_brightness ({active})",
                [CONF_DISPLAY, CONF_INACTIVE_BRIGHTNESS])
        schedule = display.get(CONF_SCHEDULE, [])
        has_fixed = any(not isinstance(item[CONF_AT], str) for item in schedule)
        has_sun = any(isinstance(item[CONF_AT], str) for item in schedule)
        if schedule:
            time_id = display.get(CONF_TIME_ID)
            if time_id is None and CONF_SCREENSAVER in config:
                time_id = config[CONF_SCREENSAVER].get(CONF_TIME_ID)
            if time_id is None:
                for cfg_src in (getattr(core.CORE, "config", None), getattr(core.CORE, "raw_config", None)):
                    if not isinstance(cfg_src, dict):
                        continue
                    time_conf = cfg_src.get("time")
                    if isinstance(time_conf, dict):
                        time_id = time_conf.get(CONF_ID)
                    elif isinstance(time_conf, list) and len(time_conf) == 1 and isinstance(time_conf[0], dict):
                        time_id = time_conf[0].get(CONF_ID)
                    if time_id is not None:
                        break
            if time_id is None:
                raise cv.Invalid(
                    "display.schedule requires a time_id "
                    "(set display.time_id or screensaver.time_id)",
                    [CONF_DISPLAY, CONF_SCHEDULE])
            if isinstance(time_id, str):
                time_id = core.ID(time_id, is_declaration=False)
            display[CONF_TIME_ID] = time_id
        if has_sun and CONF_SUN_ID not in display:
            sun_id = None
            for cfg_src in (getattr(core.CORE, "config", None), getattr(core.CORE, "raw_config", None)):
                if not isinstance(cfg_src, dict):
                    continue
                sun_conf = cfg_src.get("sun")
                if isinstance(sun_conf, dict):
                    sun_id = sun_conf.get(CONF_ID)
                elif isinstance(sun_conf, list) and len(sun_conf) == 1 and isinstance(sun_conf[0], dict):
                    sun_id = sun_conf[0].get(CONF_ID)
                if sun_id is not None:
                    break
            if sun_id is None:
                raise cv.Invalid(
                    "display.schedule with sunrise/sunset requires a sun component "
                    "(add sun: or set display.sun_id)",
                    [CONF_DISPLAY, CONF_SUN_ID])
            if isinstance(sun_id, str):
                sun_id = core.ID(sun_id, is_declaration=False)
            display[CONF_SUN_ID] = sun_id
        for i, item in enumerate(schedule):
            item_inactive = item[CONF_INACTIVE_BRIGHTNESS]
            if item_inactive >= active:
                raise cv.Invalid(
                    f"display.schedule[{i}].inactive_brightness ({item_inactive}) must be "
                    f"less than display.active_brightness ({active})",
                    [CONF_DISPLAY, CONF_SCHEDULE, i, CONF_INACTIVE_BRIGHTNESS])

    # Build a local list of custom card ids (validation state never leaves
    # this function), rejecting duplicates
    card_ids: dict[str, int] = {}
    for i, card_config in enumerate(config.get(CONF_CARDS, [])):
        if CONF_ID in card_config:
            card_id = card_config[CONF_ID]
            if card_id in card_ids:
                raise cv.Invalid(
                    f"duplicate id '{card_id}': card ids must be unique",
                    [CONF_CARDS, i, CONF_ID])
            card_ids[card_id] = i

    # The screensaver page id shares the page uuid space with card ids
    if CONF_SCREENSAVER in config and CONF_ID in config[CONF_SCREENSAVER]:
        screensaver_id = config[CONF_SCREENSAVER][CONF_ID]
        if screensaver_id in card_ids:
            raise cv.Invalid(
                f"duplicate id '{screensaver_id}': screensaver and card ids must be unique",
                [CONF_SCREENSAVER, CONF_ID])

    if CONF_DEFAULT_CARD in config and card_ids.get(config[CONF_DEFAULT_CARD]) is None:
        raise cv.Invalid(f"Cannot find a card with the id '{config[CONF_DEFAULT_CARD]}'", [CONF_DEFAULT_CARD])

    for i, card_config in enumerate(config.get(CONF_CARDS, [])):
        err_path = [CONF_CARDS, i]

        if i == 0 and CONF_SCREENSAVER not in config:
            if card_config[CONF_CARD_HIDDEN] == True:
                raise cv.Invalid(f"The first card cannot be hidden if the screensaver is disabled", err_path)
            if card_config[CONF_SLEEP_TIMEOUT] != 0:
                raise cv.Invalid(f"The first card sleep_timeout must be 0 if the screensaver is disabled", err_path)

        entities = card_config.get(CONF_CARD_ENTITIES, [])
        err_path.append(CONF_CARD_ENTITIES)

        length_limits = get_card_entities_length_limits(card_config[CONF_CARD_TYPE], model)
        if len(entities) > 0 and length_limits[1] == 0:
            raise cv.Invalid(f"No entities are allowed for '{card_config[CONF_CARD_TYPE]}' cards", err_path)
        if length_limits[0] == length_limits[1] and len(entities) != length_limits[0]:
            raise cv.Invalid(f"There must be exactly {length_limits[0]} entities for '{card_config[CONF_CARD_TYPE]}' cards", err_path)
        if len(entities) < length_limits[0]:
            raise cv.Invalid(f"There must be at least {length_limits[0]} entities for '{card_config[CONF_CARD_TYPE]}' cards", err_path)
        if len(entities) > length_limits[1]:
            raise cv.Invalid(f"There must be at most {length_limits[1]} entities for '{card_config[CONF_CARD_TYPE]}' cards", err_path)

        for j, entity_config in enumerate(entities):
            entity_id = entity_config.get(CONF_ENTITY_ID)
            domain, _, card_id = entity_id.partition('.')
            if domain == 'navigate':
                # card_id is empty if the value is a bare 'navigate'; that yields a clean
                # dangling-reference error below instead of an IndexError.
                if card_ids.get(card_id) is None:
                    raise cv.Invalid(
                        f'navigation entity_id invalid, no card has the id "{card_id}"',
                        err_path + [j, CONF_ENTITY_ID],
                    )

    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(NSPanelLovelace),
        # Timeout range 0..MAX_SLEEP_TIMEOUT seconds (0 disables the screensaver)
        cv.Optional(CONF_SLEEP_TIMEOUT, default=10): cv.int_range(0, MAX_SLEEP_TIMEOUT),
        cv.Optional(CONF_EXP_VERT_UI, default=False): cv.boolean,
        cv.Optional(CONF_MODEL, default='eu'): cv.one_of('eu', 'us-l', 'us-p'),
        cv.Optional(CONF_DEFAULT_CARD): cv.string_strict,
        cv.Optional(CONF_LOCALE, default={}): SCHEMA_LOCALE,
        cv.Optional(CONF_SCREENSAVER): SCHEMA_SCREENSAVER,
        cv.Optional(CONF_DISPLAY): SCHEMA_DISPLAY,
        cv.Optional(CONF_INCOMING_MSG): automation.validate_automation(
            cv.Schema({
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(NSPanelLovelaceMsgIncomingTrigger),
            })
        ),
        cv.Optional(CONF_CARDS): cv.ensure_list(cv.All(
            has_card_type,
            cv.typed_schema({
                CARD_ENTITIES: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY)
                }),
                CARD_GRID: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY)
                }),
                CARD_GRID2: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY)
                }),
                CARD_QR: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY),
                    cv.Optional(CONF_CARD_QR_TEXT): cv.string_strict
                }),
                CARD_ALARM: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ALARM_ENTITY_ID): valid_entity_id(['alarm_control_panel']),
                    cv.Optional(CONF_CARD_ALARM_SUPPORTED_MODES, default=ALARM_ARM_DEFAULT_OPTIONS): 
                        cv.All(
                            cv.ensure_list(cv.one_of(*ALARM_ARM_OPTIONS)),
                            cv.Length(1, 4, f"Must be a list of up to 4 items from the following list: {ALARM_ARM_OPTIONS}"),
                            ensure_unique
                        )
                }),
                CARD_THERMO: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_THERMO_ENTITY_ID): valid_entity_id(['climate'])
                }),
                CARD_MEDIA: SCHEMA_CARD_BASE.extend({
                    cv.Optional(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY),
                    cv.Required(CONF_CARD_MEDIA_ENTITY_ID): valid_entity_id(['media_player'])
                }),
                CARD_POWER: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_POWER_HOME_ENTITY_ID): valid_entity_id(['sensor']),
                    cv.Optional(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY),
                }),
            }))
        ),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    cv.require_esphome_version(2026,11,0),
    validate_config
)

GlobalConfig = nspanel_lovelace_ns.class_("Configuration")
GlobalConfig.op = "::"

Screensaver = nspanel_lovelace_ns.class_("Screensaver")
EntitiesCard = nspanel_lovelace_ns.class_("EntitiesCard")
GridCard = nspanel_lovelace_ns.class_("GridCard")
QRCard = nspanel_lovelace_ns.class_("QRCard")
AlarmCard = nspanel_lovelace_ns.class_("AlarmCard")
ThermoCard = nspanel_lovelace_ns.class_("ThermoCard")
MediaCard = nspanel_lovelace_ns.class_("MediaCard")
PowerCard = nspanel_lovelace_ns.class_("PowerCard")
PowerCardItem = nspanel_lovelace_ns.class_("PowerCardItem")

DeleteItem = nspanel_lovelace_ns.class_("DeleteItem")
NavigationItem = nspanel_lovelace_ns.class_("NavigationItem")
StatusIconItem = nspanel_lovelace_ns.class_("StatusIconItem")
WeatherItem = nspanel_lovelace_ns.class_("WeatherItem")
EntitiesCardEntityItem = nspanel_lovelace_ns.class_("EntitiesCardEntityItem")
GridCardEntityItem = nspanel_lovelace_ns.class_("GridCardEntityItem")
AlarmButtonItem = nspanel_lovelace_ns.class_("AlarmButtonItem")

PageType = nspanel_lovelace_ns.enum("page_type", True)

PAGE_MAP = {
    # [config type] : [c++ variable name prefix], [card class], [card type], [entity class]
    CONF_SCREENSAVER: ["nspanel_screensaver", Screensaver, PageType.screensaver, WeatherItem],
    CARD_ENTITIES: ["nspanel_card_", EntitiesCard, PageType.cardEntities, EntitiesCardEntityItem],
    CARD_GRID: ["nspanel_card_", GridCard, PageType.cardGrid, GridCardEntityItem],
    CARD_GRID2: ["nspanel_card_", GridCard, PageType.cardGrid2, GridCardEntityItem],
    CARD_QR: ["nspanel_card_", QRCard, PageType.cardQR, EntitiesCardEntityItem],
    CARD_ALARM: ["nspanel_card_", AlarmCard, PageType.cardAlarm, AlarmButtonItem],
    CARD_THERMO: ["nspanel_card_", ThermoCard, PageType.cardThermo, None],
    CARD_MEDIA: ["nspanel_card_", MediaCard, PageType.cardMedia, GridCardEntityItem],
    CARD_POWER: ["nspanel_card_", PowerCard, PageType.cardPower, PowerCardItem],
}

def collect_entity_ids(config: dict) -> dict[str, str]:
    """Build the ordered entity -> variable map for one validated config, at
    codegen time. Preserves the historical loop order: card tiles in card
    order (exact 'delete' excluded), then each card's alarm/thermo/media/power
    entity, then the screensaver's left and right status icons. navigate and
    iText pseudo-entities are registered like any other tile; first use
    wins; variables are named nspanel_e0, nspanel_e1, ..."""
    entity_ids: dict[str, str] = {}
    index = 0

    def add(entity_id):
        nonlocal index
        if entity_id is not None and entity_id not in entity_ids:
            entity_ids[entity_id] = f"nspanel_e{index}"
            index += 1

    for card_config in config.get(CONF_CARDS, []):
        for entity_config in card_config.get(CONF_CARD_ENTITIES, []):
            entity_id = entity_config.get(CONF_ENTITY_ID)
            if entity_id != 'delete':
                add(entity_id)
        if CONF_CARD_ALARM_ENTITY_ID in card_config:
            add(card_config.get(CONF_CARD_ALARM_ENTITY_ID))
        if CONF_CARD_THERMO_ENTITY_ID in card_config:
            add(card_config.get(CONF_CARD_THERMO_ENTITY_ID))
        if CONF_CARD_MEDIA_ENTITY_ID in card_config:
            add(card_config.get(CONF_CARD_MEDIA_ENTITY_ID))
        if CONF_CARD_POWER_HOME_ENTITY_ID in card_config:
            add(card_config.get(CONF_CARD_POWER_HOME_ENTITY_ID))
    if CONF_SCREENSAVER in config:
        screensaver_config = config.get(CONF_SCREENSAVER)
        left = screensaver_config.get(CONF_SCREENSAVER_STATUS_ICON_LEFT, None)
        right = screensaver_config.get(CONF_SCREENSAVER_STATUS_ICON_RIGHT, None)
        if left and CONF_ENTITY_ID in left:
            add(left.get(CONF_ENTITY_ID))
        if right and CONF_ENTITY_ID in right:
            add(right.get(CONF_ENTITY_ID))
    return entity_ids

class CodegenContext:
    """Per-call codegen state: the entity variable map, the UUID index and
    the custom-icon slots, one instance per to_code() call. Nothing here is
    shared between configurations in the same process."""

    def __init__(self, config: dict):
        self.entity_ids = collect_entity_ids(config)
        self.uuid_index = 0
        self.custom_icons: dict[str, list] = {}  # hex -> [slot, expression]

    def new_uuid(self, prefix: str = "") -> str:
        self.uuid_index += 1
        return prefix + str(self.uuid_index)

    def entity(self, entity_id) -> cg.MockObj:
        return cg.MockObj(self.entity_ids[entity_id])

    def icon(self, iconHexStr) -> Union[cg.MockObj, None]:
        if not isinstance(iconHexStr, str):
            return None
        for v in BUILTIN_ICON_MAP:
            if iconHexStr == v[0]:
                return v[1]
        found_icon = self.custom_icons.get(iconHexStr, None)
        if found_icon is None:
            self.custom_icons[iconHexStr] = [
                len(self.custom_icons),
                cg.RawExpression(r'u8"\u{0}"'.format(iconHexStr))]
        return cg.RawExpression(f"CUSTOM_ICONS[{self.custom_icons[iconHexStr][0]}]")

def generate_icon_config(icon_config, ctx: CodegenContext, parent_class: cg.MockObj = None) -> Union[None, dict]:
    attrs = {
        "value": None,
        "color": None,
    }
    if icon_config is not None:
        if isinstance(icon_config, dict):
            attrs["value"] = icon_config.get(CONF_ICON_VALUE, None)
            attrs["color"] = icon_config.get(CONF_ICON_COLOR, None)
        elif isinstance(icon_config, str):
            attrs["value"] = icon_config
    if isinstance(attrs["value"], str):
        attrs["value"] = ctx.icon(get_icon_hex(attrs["value"]))
        if isinstance(parent_class, cg.MockObj):
            # attrs["value"] is a MockObj (icon_t member) or RawExpression
            # (CUSTOM_ICONS[n]), never a Python str, so the expression API
            # emits it verbatim without cpp_string_escape quoting.
            cg.add(parent_class.set_icon_value(attrs["value"]))
    if isinstance(attrs["color"], int):
        if isinstance(parent_class, cg.MockObj):
            cg.add(parent_class.set_icon_color(attrs["color"]))
    if parent_class is None:
        return attrs

def service_data_maps(data: dict) -> tuple[dict[str, str], dict[str, str]]:
    """Split service data into plain strings and HA-rendered literals.

    HA's data_template renderer parses rendered values with literal_eval, so
    repr() of a non-string YAML value arrives at the service as a native
    list/number/bool instead of a string."""
    plain = {key: value for key, value in data.items() if isinstance(value, str)}
    templated = {key: repr(value) for key, value in data.items()
                 if not isinstance(value, str)}
    return plain, templated

def cpp_string_map(items: dict[str, str]) -> str:
    """C++ map initializer with quoted, escaped keys and values."""
    entries = ", ".join(
        "{" + str(StringLiteral(key)) + ", " + str(StringLiteral(value)) + "}"
        for key, value in items.items()
    )
    return f"std::map<std::string, std::string>{{{entries}}}"

def gen_card_entities(entities_config, card_class: cg.MockObjClass, card_variable: cg.MockObjClass, entity_type: cg.MockObjClass, ctx: CodegenContext):
    for i, entity_config in enumerate(entities_config):
        variable_name = card_variable.__str__() + "_item_" + str(i + 1)
        entity_class = cg.global_ns.class_(variable_name)
        entity_class.op = "->"

        if entity_config.get(CONF_ENTITY_ID, "delete") == 'delete':
            cg.add(cg.RawExpression(
                f"auto {variable_name} = "
                f"{make_shared.template(DeleteItem).__call__(card_class)}"))
            cg.add(card_variable.add_item(entity_class))
            continue

        entity_id = ctx.entity(entity_config.get(CONF_ENTITY_ID))
        display_name = entity_config.get(CONF_CARD_ENTITIES_NAME, None)
        # if display_name != None:
        #     entity_class = cg.new_Pvariable(variable_name, entity_config[CONF_CARD_ENTITIES_ID], entity_config[CONF_CARD_ENTITIES_NAME])
        # else:
        #     entity_class = cg.new_Pvariable(variable_name, entity_config[CONF_CARD_ENTITIES_ID])
        cg.add(cg.RawExpression(
            f"auto {variable_name} = "
            f"{make_shared.template(entity_type).__call__(ctx.new_uuid(), entity_id, display_name)}"))

        generate_icon_config(entity_config.get(CONF_ICON, None), ctx, parent_class=entity_class)

        if CONF_SERVICE in entity_config:
            entity_instance = ctx.entity(entity_config[CONF_ENTITY_ID])
            entity_instance.op = "->"
            data = entity_config.get(CONF_DATA, {})
            plain, templated = service_data_maps(data)
            cg.add(entity_instance.set_ha_service(
                entity_config[CONF_SERVICE],
                cg.RawExpression(cpp_string_map(plain)),
                cg.RawExpression(cpp_string_map(templated))))

        cg.add(card_variable.add_item(entity_class))

def get_status_icon_statement(icon_config, icon_class: cg.MockObjClass, variable_name: str, ctx: CodegenContext, default_icon_value: str = 'alert-circle-outline') -> cg.MockObjClass:
    entity_id = ctx.entity(icon_config.get(CONF_ENTITY_ID))
    default_icon_value = ctx.icon(get_icon_hex(default_icon_value))
    attrs = generate_icon_config(icon_config.get(CONF_ICON, {}), ctx)
    # generate_icon_config converts a configured icon string to an expression
    # (icon_t:: enum or CUSTOM_ICONS[n]), so "configured" must be tested on
    # whether a value exists, not on its type.
    has_icon = attrs["value"] is not None
    has_color = isinstance(attrs["color"], int)
    # Constructor arguments go through the expression API: the uuid (str) is
    # quoted by StringLiteral, while the icon expression (MockObj/RawExpression)
    # and the 'u'-suffixed color emit verbatim, never quoted. A trailing None
    # drops the color argument, preserving all four constructor arities.
    icon_value = attrs["value"] if has_icon else default_icon_value
    color_expr = cg.RawExpression(f'{attrs["color"]}u') if has_color else None
    ctor = make_shared.template(icon_class).__call__(
        ctx.new_uuid(), entity_id, icon_value, color_expr)
    cg.add(cg.RawExpression(f"auto {variable_name} = {ctor}"))
    variable_class = cg.global_ns.class_(variable_name)
    variable_class.op = '->'
    # The constructor arguments are defaults only: on_entity_type_change()
    # replaces the glyph for known entity types and the state callbacks
    # recolor it. Re-apply explicitly configured values through the setters,
    # which set the *_overridden_ flags those callbacks respect, otherwise
    # the configured values are silently discarded at runtime.
    if has_icon:
        cg.add(variable_class.set_icon_value(attrs["value"]))
    if has_color:
        cg.add(variable_class.set_icon_color(attrs["color"]))
    return variable_class

def configure_platform():
    # note: not using 'psram' dependency because our sdkconfig options conflict
    if has_build_define(core.CORE.config[CONF_ESPHOME], "TEST_DEVICE_MODE"):
        _LOGGER.info(f"[nspanel_lovelace] TEST DEVICE MODE ACTIVE, PSRAM DISABLED")
    else:
        # NSPanel has non-standard PSRAM pins
        cg.add_define("USE_PSRAM")
        esp32.add_idf_sdkconfig_option(
            f"CONFIG_{esp32.get_esp32_variant().upper()}_SPIRAM_SUPPORT", True
        )
        esp32.add_idf_sdkconfig_option("CONFIG_SPIRAM", True)
        esp32.add_idf_sdkconfig_option("CONFIG_SPIRAM_USE", True)
        esp32.add_idf_sdkconfig_option("CONFIG_SPIRAM_USE_MALLOC", True)
        esp32.add_idf_sdkconfig_option("CONFIG_SPIRAM_IGNORE_NOTFOUND", True)
        esp32.add_idf_sdkconfig_option("CONFIG_D0WD_PSRAM_CLK_IO", 5)
        esp32.add_idf_sdkconfig_option("CONFIG_D0WD_PSRAM_CS_IO", 18)
        # Also increase flash & CPU speed as NSPanel hardware supports it
        esp32.add_idf_sdkconfig_option("CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ_240", True)
        esp32.add_idf_sdkconfig_option("CONFIG_SPIRAM_SPEED_80M", True)
        esp32.add_idf_sdkconfig_option("CONFIG_SPIRAM_MODE_QUAD", True)
        esp32.add_idf_sdkconfig_option("CONFIG_ESPTOOLPY_FLASHMODE_QIO", True)
        esp32.add_idf_sdkconfig_option("CONFIG_ESPTOOLPY_FLASHFREQ_80M", True)

        # Enable use of bluetooth_proxy by moving memory allocations to PSRAM
        # see: https://esphome.io/components/bluetooth_proxy
        esp32.add_idf_sdkconfig_option("CONFIG_BT_ALLOCATION_FROM_SPIRAM_FIRST", True)
        esp32.add_idf_sdkconfig_option("CONFIG_BT_BLE_DYNAMIC_ENV_MEMORY", True)

        ## Allow handling of large weather forecast objects
        cg.add_define("ARDUINOJSON_SLOT_ID_SIZE", 2)
        cg.add_define("ARDUINOJSON_ENABLE_STD_STRING", 1)

    ## Explicitly enable services and states (floor version 2026.11.0, see require_esphome_version)
    cg.add_define("USE_API_HOMEASSISTANT_STATES")
    cg.add_define("USE_API_HOMEASSISTANT_SERVICES")


def emit_translations(nspanel, locale_config):
    language = locale_config[CONF_LANGUAGE]
    # Already validated in validate_config; the file is small enough to read again.
    translation_path, _ = resolve_translation_path(language, core.CORE.relative_config_path)
    translationJson = load_translations(translation_path)

    # A custom JSON file has no matching built-in language name
    if '.' in language:
        cg.add(nspanel.set_language("unknown"))
    else:
        cg.add(nspanel.set_language(language))

    cgv = []
    for k,v in translationJson.items():
        if k in resources.REQUIRED_TRANSLATION_KEYS:
            if k in cv.RESERVED_IDS:
                k += '_'
            k = TRANSLATION_ITEM.class_(k)
        cgv.append(cg.ArrayInitializer(k, v))
    cg.add_define("TRANSLATION_MAP_SIZE", len(cgv))
    cg.add_global(cg.RawStatement(
        "constexpr FrozenCharMap<const char *, TRANSLATION_MAP_SIZE> "
        f"esphome::{nspanel_lovelace_ns}::TRANSLATION_MAP {{{cg.ArrayInitializer(*cgv, multiline=True)}}};"))

    if CONF_TEMPERATURE_UNIT in locale_config:
        cg.add(GlobalConfig.set_temperature_unit(TEMPERATURE_UNIT_OPTION_MAP[locale_config[CONF_TEMPERATURE_UNIT]]))

async def emit_screensaver(nspanel, config, ctx) -> Union[str, None]:
    screensaver_config = config.get(CONF_SCREENSAVER, None)
    screensaver_uuid = None
    if screensaver_config is None:
        cg.add(nspanel.set_display_timeout(0))
    else:
        cg.add(cg.RawStatement("{"))

        screensaver_uuid = screensaver_config[CONF_ID] if CONF_ID in screensaver_config else ctx.new_uuid()

        if CONF_TIME_ID in screensaver_config:
            time_ = await cg.get_variable(screensaver_config[CONF_TIME_ID])
            cg.add(nspanel.set_time_id(time_))

        if CONF_SCREENSAVER_DATE_FORMAT in screensaver_config:
            cg.add(nspanel.set_date_format(screensaver_config[CONF_SCREENSAVER_DATE_FORMAT]))
        if CONF_SCREENSAVER_TIME_FORMAT in screensaver_config:
            cg.add(nspanel.set_time_format(screensaver_config[CONF_SCREENSAVER_TIME_FORMAT]))

        if screensaver_config.get(CONF_SCREENSAVER_DOUBLE_TAP_TO_UNLOCK, False):
            cg.add(nspanel.set_double_tap_to_unlock(True))

        screensaver_info = PAGE_MAP[CONF_SCREENSAVER]
        screensaver_class = cg.global_ns.class_(screensaver_info[0])
        screensaver_class.op = "->"

        cg.add(cg.RawExpression(
            f"auto {screensaver_info[0]} = "
            f"{nspanel.create_screensaver.__call__(screensaver_uuid)}"))

        if CONF_SCREENSAVER_STATUS_ICON_LEFT in screensaver_config:
            left_icon_config = screensaver_config[CONF_SCREENSAVER_STATUS_ICON_LEFT]
            iconleft_variable = screensaver_info[0] + "_iconleft"
            iconleft_variable_class = get_status_icon_statement(
                left_icon_config, StatusIconItem, iconleft_variable, ctx)
            if left_icon_config.get(CONF_SCREENSAVER_STATUS_ICON_ALT_FONT, False):
                cg.add(iconleft_variable_class.set_alt_font(True))
            cg.add(screensaver_class.set_icon_left(iconleft_variable_class))

        if CONF_SCREENSAVER_STATUS_ICON_RIGHT in screensaver_config:
            right_icon_config = screensaver_config[CONF_SCREENSAVER_STATUS_ICON_RIGHT]
            iconright_variable = screensaver_info[0] + "_iconright"
            iconright_variable_class = get_status_icon_statement(
                right_icon_config, StatusIconItem, iconright_variable, ctx)
            if right_icon_config.get(CONF_SCREENSAVER_STATUS_ICON_ALT_FONT, False):
                cg.add(iconright_variable_class.set_alt_font(True))
            cg.add(screensaver_class.set_icon_right(iconright_variable_class))

        if CONF_SCREENSAVER_WEATHER in screensaver_config:
            weather_config = screensaver_config[CONF_SCREENSAVER_WEATHER]
            if CONF_ENTITY_ID in weather_config:
                cg.add(nspanel.set_weather_entity_id(weather_config[CONF_ENTITY_ID]))
            icon_color = weather_config.get(CONF_SCREENSAVER_WEATHER_ICON_COLOR, None)
            restart_delay = weather_config[CONF_SCREENSAVER_WEATHER_RESTART_AFTER_STALL].total_milliseconds
            if restart_delay:
                cg.add(nspanel.set_weather_forecast_stall_restart(restart_delay))

            # Native daily-forecast retrieval replaces both legacy forecast modes.
            # One action per slot lets each callback carry its slot number, so a
            # late reply from a timed-out dispatch cannot be mistaken for the
            # current one. Explicit IDs avoid unnamed inline-action collisions.
            cg.add_define("USE_NSPANEL_NATIVE_WEATHER")
            component_id = str(config[CONF_ID])
            for slot in range(4):
                suffix = f"{ctx.uuid_index}_{slot}"
                action_id = cg.unique_global_id(
                    f"nspanel_weather_action_{suffix}",
                    esphome_api.HomeAssistantServiceCallAction)
                success_id = cg.unique_global_id(
                    f"nspanel_weather_success_{suffix}", automation.Automation)
                success_trigger_id = cg.unique_global_id(
                    f"nspanel_weather_success_trigger_{suffix}", automation.Trigger)
                success_lambda_id = cg.unique_global_id(
                    f"nspanel_weather_success_lambda_{suffix}", automation.LambdaAction)
                error_id = cg.unique_global_id(
                    f"nspanel_weather_error_{suffix}", automation.Automation)
                error_trigger_id = cg.unique_global_id(
                    f"nspanel_weather_error_trigger_{suffix}", automation.Trigger)
                error_lambda_id = cg.unique_global_id(
                    f"nspanel_weather_error_lambda_{suffix}", automation.LambdaAction)
                action_cfg = {
                    CONF_ID: core.CORE.config[esphome_api.DOMAIN][CONF_ID],
                    "action": "weather.get_forecasts",
                    "data": {
                        "entity_id": weather_config[CONF_ENTITY_ID],
                        "type": "daily",
                    },
                    "capture_response": True,
                    "on_success": {
                        automation.CONF_TRIGGER_ID: success_trigger_id,
                        automation.CONF_AUTOMATION_ID: success_id,
                        "then": [{"lambda": core.Lambda(
                            f"id({component_id}).apply_weather_forecast(response, {slot}u);"),
                            "type_id": success_lambda_id}],
                    },
                    "on_error": {
                        automation.CONF_TRIGGER_ID: error_trigger_id,
                        automation.CONF_AUTOMATION_ID: error_id,
                        "then": [{"lambda": core.Lambda(
                            f"id({component_id}).handle_weather_forecast_error(error, {slot}u);"),
                            "type_id": error_lambda_id}],
                    },
                }
                validated = esphome_api.HOMEASSISTANT_ACTION_ACTION_SCHEMA(action_cfg)
                weather_action = await esphome_api.homeassistant_service_to_code(
                    validated, action_id, cg.TemplateArguments(), [])
                cg.add(nspanel.set_weather_forecast_action(slot, weather_action))
            screensaver_items = []
            # 1 main weather item + 4 forecast items
            for i in range(0, 5):
                item_variable = f"{screensaver_info[0]}_weather_{i}"
                cg.add(cg.RawExpression(
                    f"auto {item_variable} = "
                    f"{make_shared.template(screensaver_info[3]).__call__(ctx.new_uuid())}"))
                item_class = cg.global_ns.class_(item_variable)
                item_class.op = '->'
                if icon_color is not None:
                    cg.add(item_class.set_icon_color(cg.RawExpression(f'{icon_color}u')))
                screensaver_items.append(item_class)
            cg.add(screensaver_class.add_item_range(screensaver_items))
            # Forecast slots render blank until the first accepted reply; the
            # current-condition item (index 0) is not touched.
            cg.add(nspanel.init_weather_forecast_slots())

        cg.add(cg.RawStatement("}"))
    return screensaver_uuid

def emit_cards(nspanel, config, ctx, screensaver_uuid) -> None:
    card_uuids = []
    visible_card_uuids = []
    for card_config in config.get(CONF_CARDS, []):
        if CONF_ID in card_config:
            card_uuids.append(card_config[CONF_ID])
        else:
            card_uuids.append(ctx.new_uuid())
        if card_config[CONF_CARD_HIDDEN] == False:
            visible_card_uuids.append(card_uuids[-1])
    visible_card_count = len(visible_card_uuids)
    visible_index = 0

    navleft_icon_value = ctx.icon(get_icon_hex("arrow-left-bold"))
    navhome_icon_value = ctx.icon(get_icon_hex("home"))
    navright_icon_value = ctx.icon(get_icon_hex("arrow-right-bold"))
    for i, card_config in enumerate(config.get(CONF_CARDS, [])):
        cg.add(cg.RawStatement("{"))

        page_info = PAGE_MAP[card_config[CONF_CARD_TYPE]]
        # card_class = None
        # card_variable = core.ID(
        #     page_info[0] + str(i + 1), 
        #     is_declaration=True, 
        #     type=page_info[1],
        #     is_manual=True)
        card_variable = page_info[0] + str(i + 1)
        card_class = cg.global_ns.class_(card_variable)
        card_class.op = "->"

        title = card_config.get(CONF_CARD_TITLE, "")
        # if title != "":
        #     card_class = cg.new_Pvariable(card_variable, title)
        # else:
        #     card_class = cg.new_Pvariable(card_variable)

        if i == 0 and CONF_SCREENSAVER not in config:
            # Note: If the default (first) card has a timeout, then it will keep rendering 
            #       every time the 'sleepReached' event is sent from the display, so we set it to 0 here instead.
            sleep_timeout = 0
        else:
            sleep_timeout = card_config.get(CONF_SLEEP_TIMEOUT, 10)
        # if isinstance(sleep_timeout, int):
        #     cg.add(card_class.set_sleep_timeout(sleep_timeout))
        #     cg.add(cg.RawExpression(f"{card_variable}->set_sleep_timeout({sleep_timeout})"))

        if card_config[CONF_CARD_TYPE] in [CARD_ALARM, CARD_THERMO, CARD_MEDIA, CARD_POWER]:
            if (card_config[CONF_CARD_TYPE] == CARD_ALARM):
                entity_id_key = CONF_CARD_ALARM_ENTITY_ID
            elif (card_config[CONF_CARD_TYPE] == CARD_THERMO):
                entity_id_key = CONF_CARD_THERMO_ENTITY_ID
            elif (card_config[CONF_CARD_TYPE] == CARD_POWER):
                entity_id_key = CONF_CARD_POWER_HOME_ENTITY_ID
            else:
                entity_id_key = CONF_CARD_MEDIA_ENTITY_ID
            cg.add(cg.RawExpression(
                f"auto {card_variable} = "
                f"{nspanel.create_page.template(page_info[1]).__call__(card_uuids[i], ctx.entity(card_config[entity_id_key]), title, sleep_timeout)}"))
        else:
            cg.add(cg.RawExpression(
                f"auto {card_variable} = "
                f"{nspanel.create_page.template(page_info[1]).__call__(card_uuids[i], title, sleep_timeout)}"))
            # cg.add(cg.variable(card_variable, make_shared.template(page_info[1]).__call__(cg.global_ns.class_(page_info[0] + str(i + 1)))))

        # Special case for pages which use a different underlying type
        if card_config[CONF_CARD_TYPE] == CARD_GRID2:
            cg.add(card_class.set_render_type(page_info[2]))

        if card_config[CONF_CARD_HIDDEN] == True:
            cg.add(card_class.set_hidden(True))
            home_uuid = screensaver_uuid
            if screensaver_uuid is None:
                home_uuid = visible_card_uuids[0] if visible_card_count > 0 else None
            if home_uuid != None:
                navleft_variable = card_variable + "_navhome"
                navleft_statement = cg.RawStatement(f'new {NavigationItem}(\"{ctx.new_uuid()}\", \"{home_uuid}\", {navhome_icon_value})')
                cg.add(cg.RawExpression(
                    f"auto {navleft_variable} = "
                    f"{unique_ptr.template(NavigationItem)}({navleft_statement})"))
                cg.add(card_class.set_nav_left(cg.global_ns.class_(navleft_variable)))
        else:
            # This card is in visible_card_uuids, so the list is non-empty and
            # the modulo lookups are safe (a single visible card self-links).
            prev_card_uuid = visible_card_uuids[(visible_index - 1) % visible_card_count]
            next_card_uuid = visible_card_uuids[(visible_index + 1) % visible_card_count]
            visible_index += 1
            navleft_variable = card_variable + "_navleft"
            navleft_statement = cg.RawStatement(f'new {NavigationItem}(\"{ctx.new_uuid()}\", \"{prev_card_uuid}\", {navleft_icon_value})')
            cg.add(cg.RawExpression(
                f"auto {navleft_variable} = "
                f"{unique_ptr.template(NavigationItem)}({navleft_statement})"))
            cg.add(card_class.set_nav_left(cg.global_ns.class_(navleft_variable)))
            navright_variable = card_variable + "_navright"
            navright_statement = cg.RawStatement(f'new {NavigationItem}(\"{ctx.new_uuid()}\", \"{next_card_uuid}\", {navright_icon_value})')
            cg.add(cg.RawExpression(
                f"auto {navright_variable} = "
                f"{unique_ptr.template(NavigationItem)}({navright_statement})"))
            cg.add(card_class.set_nav_right(cg.global_ns.class_(navright_variable)))

        # todo: create qr page specific function
        if card_config[CONF_CARD_TYPE] == CARD_QR:
            if CONF_CARD_QR_TEXT in card_config:
                cg.add(card_class.set_qr_text(card_config[CONF_CARD_QR_TEXT]))
        elif card_config[CONF_CARD_TYPE] == CARD_ALARM:
            for mode in card_config[CONF_CARD_ALARM_SUPPORTED_MODES]:
                cg.add(card_class.add_arm_button(ALARM_ARM_ACTION.class_(mode)))

        gen_card_entities(
            card_config.get(CONF_CARD_ENTITIES, []), 
            page_info[2], 
            card_class, 
            page_info[3],
            ctx)

        cg.add(cg.RawStatement("}"))

def _display_schedule_eval_body(component_id, schedule, time_id, sun_id):
    """C++ lambda body that applies the currently active schedule entry."""
    lines = [
        f"auto now = id({time_id}).now();",
        "if (!now.is_valid()) return;",
        "int now_sec = now.hour * 3600 + now.minute * 60 + now.second;",
        "int best_sec = -1;",
        "int best_inactive = -1;",
    ]
    for item in schedule:
        inactive = item[CONF_INACTIVE_BRIGHTNESS]
        at = item[CONF_AT]
        if isinstance(at, str):
            method = "sunrise" if at == "sunrise" else "sunset"
            lines.extend([
                "{",
                f"  auto ev = id({sun_id}).{method}({sun.DEFAULT_ELEVATION});",
                "  if (ev.has_value()) {",
                "    int sec = ev->hour * 3600 + ev->minute * 60 + ev->second;",
                "    if (sec <= now_sec && sec >= best_sec) {",
                f"      best_sec = sec; best_inactive = {inactive};",
                "    }",
                "  }",
                "}",
            ])
        else:
            sec = at["hour"] * 3600 + at["minute"] * 60 + at.get("second", 0)
            lines.extend([
                "{",
                f"  int sec = {sec};",
                "  if (sec <= now_sec && sec >= best_sec) {",
                f"    best_sec = sec; best_inactive = {inactive};",
                "  }",
                "}",
            ])
    # wrap-around: if nothing matched today, pick the latest entry overall
    lines.extend([
        "if (best_sec < 0) {",
        "  int wrap_sec = -1;",
    ])
    for item in schedule:
        inactive = item[CONF_INACTIVE_BRIGHTNESS]
        at = item[CONF_AT]
        if isinstance(at, str):
            method = "sunrise" if at == "sunrise" else "sunset"
            lines.extend([
                "  {",
                f"    auto ev = id({sun_id}).{method}({sun.DEFAULT_ELEVATION});",
                "    if (ev.has_value()) {",
                "      int sec = ev->hour * 3600 + ev->minute * 60 + ev->second;",
                "      if (sec >= wrap_sec) {",
                f"        wrap_sec = sec; best_inactive = {inactive};",
                "      }",
                "    }",
                "  }",
            ])
        else:
            sec = at["hour"] * 3600 + at["minute"] * 60 + at.get("second", 0)
            lines.extend([
                "  {",
                f"    int sec = {sec};",
                "    if (sec >= wrap_sec) {",
                f"      wrap_sec = sec; best_inactive = {inactive};",
                "    }",
                "  }",
            ])
    lines.extend([
        "}",
        "if (best_inactive >= 0) {",
        f"  id({component_id}).set_display_inactive_dim(static_cast<uint8_t>(best_inactive));",
        "}",
    ])
    return "\n".join(lines)


async def emit_display(nspanel, config):
    display = config.get(CONF_DISPLAY)
    if display is None:
        return

    cg.add(nspanel.set_display_dim_defaults(
        display[CONF_ACTIVE_BRIGHTNESS], display[CONF_INACTIVE_BRIGHTNESS]))

    schedule = display.get(CONF_SCHEDULE, [])
    if not schedule:
        return

    time_id = display.get(CONF_TIME_ID)
    if time_id is None and CONF_SCREENSAVER in config:
        time_id = config[CONF_SCREENSAVER].get(CONF_TIME_ID)
    time_var = await cg.get_variable(time_id) if time_id is not None else None
    sun_var = await cg.get_variable(display[CONF_SUN_ID]) if CONF_SUN_ID in display else None

    has_fixed = any(not isinstance(item[CONF_AT], str) for item in schedule)
    if has_fixed:
        cg.add_define("USE_TIME_TRIGGERS")

    component_id = str(config[CONF_ID])
    for i, item in enumerate(schedule):
        inactive = item[CONF_INACTIVE_BRIGHTNESS]
        at = item[CONF_AT]
        if isinstance(at, str):
            trigger_id = cg.unique_global_id(
                f"nspanel_display_sun_{i}", sun.SunTrigger)
            trigger = cg.new_Pvariable(trigger_id)
            core.CORE.component_ids.add(str(trigger_id))
            await cg.register_component(trigger, {CONF_ID: trigger_id})
            await cg.register_parented(trigger, sun_var)
            cg.add(trigger.set_sunrise(at == "sunrise"))
            cg.add(trigger.set_elevation(sun.DEFAULT_ELEVATION))
            conf = {
                CONF_TRIGGER_ID: trigger_id,
                automation.CONF_AUTOMATION_ID: cg.unique_global_id(
                    f"nspanel_display_sun_auto_{i}", automation.Automation),
                "then": [{
                    "lambda": core.Lambda(
                        f"id({component_id}).set_display_inactive_dim({inactive});"),
                    "type_id": cg.unique_global_id(
                        f"nspanel_display_sun_lambda_{i}", automation.LambdaAction),
                }],
            }
            await automation.build_automation(trigger, [], conf)
        else:
            trigger_id = cg.unique_global_id(
                f"nspanel_display_time_{i}", time.CronTrigger)
            trigger = cg.new_Pvariable(trigger_id, time_var)
            core.CORE.component_ids.add(str(trigger_id))
            await cg.register_component(trigger, {CONF_ID: trigger_id})
            cg.add(trigger.add_seconds([at.get("second", 0)]))
            cg.add(trigger.add_minutes([at["minute"]]))
            cg.add(trigger.add_hours([at["hour"]]))
            cg.add(trigger.add_days_of_month(list(range(1, 32))))
            cg.add(trigger.add_months(list(range(1, 13))))
            cg.add(trigger.add_days_of_week(list(range(1, 8))))
            conf = {
                CONF_TRIGGER_ID: trigger_id,
                automation.CONF_AUTOMATION_ID: cg.unique_global_id(
                    f"nspanel_display_time_auto_{i}", automation.Automation),
                "then": [{
                    "lambda": core.Lambda(
                        f"id({component_id}).set_display_inactive_dim({inactive});"),
                    "type_id": cg.unique_global_id(
                        f"nspanel_display_time_lambda_{i}", automation.LambdaAction),
                }],
            }
            await automation.build_automation(trigger, [], conf)

    # Apply the correct schedule entry on boot/time sync so reboots don't leave
    # the panel at the default until the next trigger fires.
    if time_var is not None:
        eval_body = _display_schedule_eval_body(
            component_id, schedule, str(time_id),
            str(display[CONF_SUN_ID]) if CONF_SUN_ID in display else None)
        sync_conf = {
            CONF_TRIGGER_ID: cg.unique_global_id(
                "nspanel_display_sync", automation.CallbackAutomation),
            automation.CONF_AUTOMATION_ID: cg.unique_global_id(
                "nspanel_display_sync_auto", automation.Automation),
            "then": [{
                "lambda": core.Lambda(eval_body),
                "type_id": cg.unique_global_id(
                    "nspanel_display_sync_lambda", automation.LambdaAction),
            }],
        }
        await automation.build_callback_automation(
            time_var, "add_on_time_sync_callback", [], sync_conf)

        # Also apply at boot (low priority) in case time is already valid.
        startup_trigger = cg.esphome_ns.class_(
            "StartupTrigger", cg.Component, automation.Trigger.template())
        boot_id = cg.unique_global_id("nspanel_display_boot", startup_trigger)
        boot_trigger = cg.new_Pvariable(boot_id, -100.0)
        core.CORE.component_ids.add(str(boot_id))
        await cg.register_component(boot_trigger, {CONF_ID: boot_id})
        boot_conf = {
            CONF_TRIGGER_ID: boot_id,
            automation.CONF_AUTOMATION_ID: cg.unique_global_id(
                "nspanel_display_boot_auto", automation.Automation),
            "then": [{
                "lambda": core.Lambda(eval_body),
                "type_id": cg.unique_global_id(
                    "nspanel_display_boot_lambda", automation.LambdaAction),
            }],
        }
        await automation.build_automation(boot_trigger, [], boot_conf)


async def to_code(config):
    # One context per call: entity variables, UUIDs and custom-icon slots
    # are allocated for this configuration only (no module-level state).
    ctx = CodegenContext(config)
    configure_platform()

    nspanel = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(nspanel, config)
    await uart.register_uart_device(nspanel, config)
    cg.add_global(nspanel_lovelace_ns.using)

    enable_tft_upload = not has_build_define(
        core.CORE.config[CONF_ESPHOME], "DISABLE_NSPANEL_TFT_UPLOAD")

    # TODO: check that upload_tft() function is not used anywhere
    if enable_tft_upload:
        # FIXME when using add_define, the code wont build because of
        #       an undefined reference to 'upload_tft' during linking
        # cg.add_define("USE_NSPANEL_TFT_UPLOAD")
        # core.CORE.add_define("USE_NSPANEL_TFT_UPLOAD")
        cg.add_build_flag("-DUSE_NSPANEL_TFT_UPLOAD")

        esp32.include_builtin_idf_component("esp_http_client")
        esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_INSECURE", True)
        esp32.add_idf_sdkconfig_option(
            "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY", True
        )

    if CONF_SCREENSAVER in config:
        cg.add(nspanel.set_display_timeout(config[CONF_SLEEP_TIMEOUT]))

    if CONF_EXP_VERT_UI in config:
        cg.add(nspanel.set_global_vertical_ui(config[CONF_EXP_VERT_UI]))

    emit_translations(nspanel, config[CONF_LOCALE])

    await emit_display(nspanel, config)

    for conf in config.get(CONF_INCOMING_MSG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], nspanel)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for key, value in ctx.entity_ids.items():
        cg.add(cg.RawExpression(f"auto {value} = {nspanel.create_entity(key)}"))

    screensaver_uuid = await emit_screensaver(nspanel, config, ctx)

    emit_cards(nspanel, config, ctx, screensaver_uuid)

    if CONF_DEFAULT_CARD in config:
        # Validated in validate_config; can only be called after all the
        # pages have been created.
        cg.add(nspanel.set_default_page(config[CONF_DEFAULT_CARD]))

    icon_arr: list[str] = []
    for k,v in ctx.custom_icons.items():
        icon_arr.append(v[1])
    cg.add_define("CUSTOM_ICONS_SIZE", len(icon_arr))
    cg.add_global(cg.RawStatement(
        f"constexpr std::array<const esphome::{nspanel_lovelace_ns}::icon_char_t*, CUSTOM_ICONS_SIZE> "
        f"CUSTOM_ICONS {{{cg.ArrayInitializer(*icon_arr)}}};"))

    cg.add_define("USE_NSPANEL_LOVELACE")
