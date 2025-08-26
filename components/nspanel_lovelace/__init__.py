from esphome import automation
import esphome.config_validation as cv
import esphome.config_helpers as ch
import esphome.codegen as cg
import esphome.core as core
import re
import logging
from typing import Union
import os, json

from esphome.components import uart, time, esp32
from esphome.const import (
    __version__ as ESPHOME_VERSION,
    CONF_ID,
    CONF_TRIGGER_ID,
    CONF_TIME_ID,
    CONF_ESPHOME,
    CONF_PLATFORMIO_OPTIONS
)

CODEOWNERS = ["@olicooper"]
DEPENDENCIES = ["uart", "time", "wifi", "api", "esp32", "json"]

def AUTO_LOAD():
    val = ["text_sensor", "json"]
    return val

_LOGGER = logging.getLogger(__name__)

card_ids: dict[str] = {}
entity_ids: dict[str] = {}
entity_id_index = 0
uuid_index = 0
iconJson = None
translationJson = None
make_shared = cg.std_ns.class_("make_shared")
unique_ptr = cg.std_ns.class_("unique_ptr")
nspanel_lovelace_ns = cg.esphome_ns.namespace("nspanel_lovelace")
NSPanelLovelace = nspanel_lovelace_ns.class_("NSPanelLovelace", cg.Component, uart.UARTDevice)
TRANSLATION_ITEM = nspanel_lovelace_ns.enum("translation_item", True)
icon_t = nspanel_lovelace_ns.enum("icon_t", True)
custom_icons: dict[str, list] = {}
custom_icons_index = 0

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

ENTITY_ID_RE = re.compile(r"^(?:(delete)|([\w]+[A-Za-z0-9]\.[\w]+[A-Za-z0-9])|(iText.[^~]*?))$")
## The list of currently supported entities
ENTITY_TYPES = [
    'sensor','binary_sensor','light','switch','scene','timer','weather','navigate',
    'alarm_control_panel','input_boolean','button','input_button','cover','fan',
    'automation','script','climate','media_player','select','input_select',
    'number','input_number','text','input_text','lock','sun','person','vacuum'
]
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

CONF_LOCALE = "locale"
CONF_TEMPERATURE_UNIT = "temperature_unit"
CONF_LANGUAGE = "language"

CONF_SCREENSAVER = "screensaver"
CONF_MODEL = "model"
CONF_SCREENSAVER_DATE_FORMAT = "date_format"
CONF_SCREENSAVER_TIME_FORMAT = "time_format"
CONF_SCREENSAVER_WEATHER = "weather"
CONF_SCREENSAVER_STATUS_ICON_LEFT = "status_icon_left"
CONF_SCREENSAVER_STATUS_ICON_RIGHT = "status_icon_right"
CONF_SCREENSAVER_STATUS_ICON_ALT_FONT = "alt_font" # todo: to_code
CONF_SCREENSAVER_DOUBLE_TAP_TO_UNLOCK = "double_tap_to_unlock"

CONF_CARDS = "cards"
CONF_CARD_TYPE = "type"
CONF_CARD_HIDDEN = "hidden"
CONF_CARD_TITLE = "title"
CONF_CARD_ENTITIES = "entities"
CONF_CARD_ENTITIES_NAME = "name"

CARD_ENTITIES="cardEntities"
CARD_GRID="cardGrid"
CARD_GRID2="cardGrid2"
CARD_QR="cardQR"
CARD_ALARM="cardAlarm"
CARD_THERMO="cardThermo"
CARD_MEDIA="cardMedia"
CARD_TYPE_OPTIONS = [CARD_ENTITIES, CARD_GRID, CARD_GRID2, CARD_QR, CARD_ALARM, CARD_THERMO, CARD_MEDIA]

CONF_CARD_QR_TEXT = "qr_text"
CONF_CARD_ALARM_ENTITY_ID = "alarm_entity_id"
CONF_CARD_ALARM_SUPPORTED_MODES = "supported_modes"
CONF_CARD_THERMO_ENTITY_ID = "thermo_entity_id"
CONF_CARD_MEDIA_ENTITY_ID = "media_entity_id"

def load_icons():
    global iconJson
    current_directory = os.path.dirname(__file__)
    iconJsonPath = os.path.join(current_directory, 'icons.json')
    _LOGGER.debug(f"[nspanel_lovelace] Attempting to load icons from '{iconJsonPath}'")
    try:
        with open(iconJsonPath, encoding="utf-8") as read_file:
            iconJson = json.load(read_file)
    except (UnicodeDecodeError, OSError):
        import requests
        response = requests.get("https://raw.githubusercontent.com/olicooper/esphome-nspanel-lovelace-native/dev/components/nspanel_lovelace/icons.json")
        if response.ok:
            with open(iconJsonPath, mode="w", encoding="utf-8") as new_file:
                new_file.write(response.text)
            iconJson = json.loads(response.text)
        else:
            raise cv.invalid("Failed to load icons, do you have internet connection?")

    if iconJson is None or len(iconJson) == 0 or len(iconJson[0]["name"]) == 0 or len(iconJson[0]["hex"]) == 0:
        raise cv.Invalid(f"Icons json invalid, please check the file. File location: {iconJsonPath}")
    _LOGGER.info(f"[nspanel_lovelace] Loaded {str(len(iconJson))} icons")

def load_translations(lang: str):
    global translationJson
    current_directory = os.path.dirname(__file__)
    jsonPath = os.path.abspath(lang)
    if not lang.endswith('.json'):
        jsonPath = os.path.join(current_directory, "translations", f"{lang}.json")
    _LOGGER.debug(f"[nspanel_lovelace] Attempting to load translation from '{jsonPath}'")
    try:
        with open(jsonPath, encoding="utf-8") as read_file:
            translationJson = json.load(read_file)
    except (UnicodeDecodeError, OSError):
        raise cv.Invalid(f"Failed to load translation file from '{jsonPath}', does it exist?")
    missingKeys = []
    for k in REQUIRED_TRANSLATION_KEYS:
        if k not in translationJson:
            missingKeys.append(k)
    if len(missingKeys) > 0:
        raise cv.Invalid(f"Translation file missing the following required keys: {missingKeys}")
    _LOGGER.info(f"[nspanel_lovelace] Loaded '{lang}' translation file")

def get_icon(iconHexStr) -> Union[cg.MockObj, None]:
    global custom_icons, custom_icons_index
    if not isinstance(iconHexStr, str):
        return None
    for v in BUILTIN_ICON_MAP:
        if iconHexStr == v[0]:
            return v[1]

    found_icon = custom_icons.get(iconHexStr, None)
    if found_icon is None:
        custom_icons[iconHexStr] = [custom_icons_index, cg.RawExpression(r'u8"\u{0}"'.format(iconHexStr))]
        custom_icons_index += 1
    return cg.RawExpression(f"CUSTOM_ICONS[{custom_icons[iconHexStr][0]}]")

def get_icon_hex(iconLookup: str) -> Union[str, None]:
    if not iconLookup or len(iconLookup) == 0:
        return None
    searchHex = False
    if iconLookup.startswith("hex"):
        iconLookup = iconLookup[4:]
        searchHex = True
    _LOGGER.debug(f"Finding icon: '{iconLookup}'")
    if iconJson is None:
        load_icons()
    if iconJson is not None:
        key = 'hex' if searchHex else 'name'
        for attrs in iconJson:
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

def valid_entity_id(entity_type_allowlist: list[str] = None):
    """Validate that a given entity_id is correctly formatted and present in the entity type allowlist."""
    def validator(value):
        value = cv.string_strict(value)
        if not value:
            return value
        if re.match(ENTITY_ID_RE, value):
            if value == 'delete' or value.startswith('iText.'):
                return value
            if [t for t in ENTITY_TYPES if value.startswith(t)]:
                if entity_type_allowlist is None:
                    return value
                elif [w for w in entity_type_allowlist if value.startswith(w)]:
                    return value
                raise cv.Invalid(
                    f"entity type '{value.split('.')[0]}' is not allowed here. Allowed types are: {entity_type_allowlist}"
                )
            raise cv.Invalid(
                f"entity type '{value.split('.')[0]}' is not allowed here. Allowed types are: {ENTITY_TYPES}"
            )
        if value.startswith('iText.'):
            raise cv.Invalid(
                f'entity_id "{value}" must match the format "iText.[text to display]" and '+
                'and not contain the character "~" (tilde)'
            )
        elif value.startswith('delete'):
            raise cv.Invalid(
                f'When using the special "delete" entity type, you cannot also specify a value'
            )
        raise cv.Invalid(
            f'entity_id "{value}" must match the format "[entity type].[entity name]" and '+
            'contain only numbers (0-9), letters (A-Z) and underscores (_), e.g. "light.living_room_light_1".'
        )
    return validator

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

SCHEMA_ICON = cv.Any(
    valid_icon_value, # icon name
    cv.Schema({
        cv.Optional(CONF_ICON_VALUE): valid_icon_value,
        cv.Optional(CONF_ICON_COLOR): cv.int_range(0, 65535),
    })
)

SCHEMA_STATUS_ICON = cv.Schema({
    cv.Optional(CONF_ENTITY_ID): valid_entity_id(['sensor','binary_sensor','light']),
    cv.Optional(CONF_ICON): SCHEMA_ICON,
    cv.Optional(CONF_SCREENSAVER_STATUS_ICON_ALT_FONT): cv.boolean,
})

SCHEMA_SCREENSAVER = cv.Schema({
    cv.Optional(CONF_ID): valid_uuid,
    cv.Optional(CONF_TIME_ID): cv.use_id(time.RealTimeClock),
    cv.Optional(CONF_SCREENSAVER_DATE_FORMAT, default="%A, %d. %B %Y"): valid_clock_format('Date format'),
    cv.Optional(CONF_SCREENSAVER_TIME_FORMAT, default="%H:%M"): valid_clock_format('Time format'),
    cv.Optional(CONF_SCREENSAVER_DOUBLE_TAP_TO_UNLOCK, default=False): cv.boolean,
    cv.Optional(CONF_SCREENSAVER_WEATHER): cv.Schema({
        cv.Required(CONF_ENTITY_ID): valid_entity_id()
    }),
    cv.Optional(CONF_SCREENSAVER_STATUS_ICON_LEFT): SCHEMA_STATUS_ICON,
    cv.Optional(CONF_SCREENSAVER_STATUS_ICON_RIGHT): SCHEMA_STATUS_ICON,
})

SCHEMA_CARD_ENTITY = cv.Schema({
    cv.Required(CONF_ENTITY_ID): valid_entity_id(),
    cv.Optional(CONF_CARD_ENTITIES_NAME): cv.string,
    cv.Optional(CONF_ICON): SCHEMA_ICON,
})

SCHEMA_CARD_BASE = cv.Schema({
    cv.Optional(CONF_ID): valid_uuid,
    cv.Optional(CONF_CARD_TITLE): cv.string,
    cv.Optional(CONF_CARD_HIDDEN, default=False): cv.boolean,
    # Timeout range from 0s to 65s. 0s means disable screensaver.
    # note: Max is limited by HMI firmware: https://github.com/joBr99/nspanel-lovelace-ui/blob/22e96f2b3ad0cd3382008eac9b4d6a27982404b8/HMI/README.md?plain=1#L91
    cv.Optional(CONF_SLEEP_TIMEOUT, default=10): cv.int_range(0, 120)
})

def add_entity_id(id: str):
    global entity_ids, entity_id_index
    if (entity_ids.get(id) is None):
        entity_ids[id] = f"nspanel_e{entity_id_index}"
        entity_id_index += 1

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
    return [0,0]

def validate_config(config):
    global card_ids
    model = config[CONF_MODEL]
    if CONF_LANGUAGE not in config[CONF_LOCALE]:
        raise cv.Invalid("A language must be specified in locale")
    # Build a list of custom card ids
    for i, card_config in enumerate(config.get(CONF_CARDS, [])):
        if CONF_ID in card_config:
            card_ids[card_config[CONF_ID]] = i

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

        for entity_config in entities:
            entity_id = entity_config.get(CONF_ENTITY_ID)
            if entity_id.startswith('navigate'):
                entity_arr = entity_id.split('.', 1)
                # if len(entity_arr) != 2:
                #     raise cv.Invalid(f'The entity_id "{entity_id}" format is invalid')
                if card_ids.get(entity_arr[1]) is None:
                    raise cv.Invalid(f'navigation entity_id invalid, no card has the id "{entity_arr[1]}"', err_path)
            # Add all valid HA entities to global entity list for later processing
            # if not (entity_id.startswith('iText') or entity_id.startswith('delete')):
            if not entity_id.startswith('delete'):
                add_entity_id(entity_id)
        if CONF_CARD_ALARM_ENTITY_ID in card_config:
            add_entity_id(card_config.get(CONF_CARD_ALARM_ENTITY_ID))
        if CONF_CARD_THERMO_ENTITY_ID in card_config:
            add_entity_id(card_config.get(CONF_CARD_THERMO_ENTITY_ID))
        if CONF_CARD_MEDIA_ENTITY_ID in card_config:
            add_entity_id(card_config.get(CONF_CARD_MEDIA_ENTITY_ID))

    if CONF_SCREENSAVER in config:
        screensaver_config = config.get(CONF_SCREENSAVER)
        left = screensaver_config.get(CONF_SCREENSAVER_STATUS_ICON_LEFT, None)
        right = screensaver_config.get(CONF_SCREENSAVER_STATUS_ICON_RIGHT, None)
        if left and CONF_ENTITY_ID in left:
            add_entity_id(left.get(CONF_ENTITY_ID))
        if right and CONF_ENTITY_ID in right:
            add_entity_id(right.get(CONF_ENTITY_ID))

    return config

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(NSPanelLovelace),
        # Timeout range from 0s to 65s. 0s means disable screensaver.
        cv.Optional(CONF_SLEEP_TIMEOUT, default=10): cv.int_range(0, 65),
        cv.Optional(CONF_MODEL, default='eu'): cv.one_of('eu', 'us-l', 'us-p'),
        cv.Optional(CONF_DEFAULT_CARD): cv.string_strict,
        cv.Optional(CONF_LOCALE, default={}): SCHEMA_LOCALE,
        cv.Optional(CONF_SCREENSAVER): SCHEMA_SCREENSAVER,
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
            },
            default_type=CARD_GRID))
        ),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    cv.require_esphome_version(2025,5,0),
    #cv.only_with_esp_idf,
    validate_config
)

GlobalConfig = nspanel_lovelace_ns.class_("Configuration")
GlobalConfig.op = "::"

Screensaver = nspanel_lovelace_ns.class_("Screensaver")
EntitiesCard = nspanel_lovelace_ns.class_("EntitiesCard")
GridCard = nspanel_lovelace_ns.class_("GridCard")
# GridCard2 = nspanel_lovelace_ns.class_("GridCard2")
QRCard = nspanel_lovelace_ns.class_("QRCard")
AlarmCard = nspanel_lovelace_ns.class_("AlarmCard")
ThermoCard = nspanel_lovelace_ns.class_("ThermoCard")
MediaCard = nspanel_lovelace_ns.class_("MediaCard")

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
}

def get_new_uuid(prefix: str = ""):
    global uuid_index
    uuid_index += 1
    return prefix + str(uuid_index)

def get_entity_id(entity_id):
    # if entity_id in [None, "", "delete"] or entity_id.startswith("iText"):
    #     return entity_id
    return cg.MockObj(entity_ids[entity_id])

def generate_icon_config(icon_config, parent_class: cg.MockObj = None) -> Union[None, dict]:
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
        attrs["value"] = get_icon(get_icon_hex(attrs["value"]))
        if isinstance(parent_class, cg.MockObj):
            # todo: esphome is escaping the icon value (e.g. u8"\uE598") due to cpp_string_escape, so having to build a raw statement instead.
            # cg.add(parent_class.set_icon_value(attrs["value"]))
            cg.add(cg.RawStatement(f'{parent_class.__str__()}->set_icon_value({attrs["value"]});'))
    if isinstance(attrs["color"], int):
        if isinstance(parent_class, cg.MockObj):
            cg.add(parent_class.set_icon_color(attrs["color"]))
    if parent_class is None:
        return attrs

def gen_card_entities(entities_config, card_class: cg.MockObjClass, card_variable: cg.MockObjClass, entity_type: cg.MockObjClass):
    for i, entity_config in enumerate(entities_config):
        variable_name = card_variable.__str__() + "_item_" + str(i + 1)
        entity_class = cg.global_ns.class_(variable_name)
        entity_class.op = "->"

        if entity_config.get(CONF_ENTITY_ID, "delete").startswith('delete'):
            cg.add(cg.RawExpression(
                f"auto {variable_name} = "
                f"{make_shared.template(DeleteItem).__call__(card_class)}"))
            cg.add(card_variable.add_item(entity_class))
            continue

        entity_id = get_entity_id(entity_config.get(CONF_ENTITY_ID))
        display_name = entity_config.get(CONF_CARD_ENTITIES_NAME, None)
        # if display_name != None:
        #     entity_class = cg.new_Pvariable(variable_name, entity_config[CONF_CARD_ENTITIES_ID], entity_config[CONF_CARD_ENTITIES_NAME])
        # else:
        #     entity_class = cg.new_Pvariable(variable_name, entity_config[CONF_CARD_ENTITIES_ID])
        cg.add(cg.RawExpression(
            f"auto {variable_name} = "
            f"{make_shared.template(entity_type).__call__(get_new_uuid(), entity_id, display_name)}"))

        generate_icon_config(entity_config.get(CONF_ICON, None), entity_class)

        cg.add(card_variable.add_item(entity_class))

def get_status_icon_statement(icon_config, icon_class: cg.MockObjClass, default_icon_value: str = 'alert-circle-outline'):
    entity_id = get_entity_id(icon_config.get(CONF_ENTITY_ID))
    default_icon_value = get_icon(get_icon_hex(default_icon_value))
    attrs = generate_icon_config(icon_config.get(CONF_ICON, {}))
    # return icon_class.__call__(get_new_uuid(), entity_id, attrs["value"], attrs["color"])
    # todo: esphome is escaping the icon value (e.g. u8"\uE598") due to cpp_string_escape, so having to build a raw statement instead.
    basicstr = f'{make_shared.template(icon_class)}("{get_new_uuid()}", {entity_id}'
    if isinstance(attrs["value"], str) and isinstance(attrs["color"], int):
        return cg.RawStatement(f'{basicstr}, {attrs["value"]}, {attrs["color"]}u)')
    elif isinstance(attrs["value"], str):
        return cg.RawStatement(f'{basicstr}, {attrs["value"]})')
    elif isinstance(attrs["color"], int):
        return cg.RawStatement(f'{basicstr}, {default_icon_value}, {attrs["color"]}u)')
    else:
        return cg.RawStatement(f'{basicstr}, {default_icon_value})')

async def to_code(config):
    # note: not using 'psram' dependency because our sdkconfig options conflict
    is_test_mode = [string for string in
                    core.CORE.config[CONF_ESPHOME][CONF_PLATFORMIO_OPTIONS] 
                    if string.endswith("TEST_DEVICE_MODE")]
    if is_test_mode:
        _LOGGER.info(f"[nspanel_lovelace] TEST DEVICE MODE ACTIVE, PSRAM DISABLED")
    # NSPanel has non-standard PSRAM pins which are not modifiable when building for Arduino
    elif core.CORE.using_esp_idf:
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
        esp32.add_idf_sdkconfig_option("CONFIG_ESP32_DEFAULT_CPU_FREQ_240", True)
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

    ## Explicitly enable services and states in ESPHome v2025.8.0+
    if cv.Version.parse(ESPHOME_VERSION) >= cv.Version(2025,8,0):
        cg.add_define("USE_API_HOMEASSISTANT_STATES")
        cg.add_define("USE_API_HOMEASSISTANT_SERVICES")

    nspanel = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(nspanel, config)
    await uart.register_uart_device(nspanel, config)
    cg.add_global(nspanel_lovelace_ns.using)

    enable_tft_upload = True
    if 'build_flags' in core.CORE.config[CONF_ESPHOME][CONF_PLATFORMIO_OPTIONS]:
        if [s for s in core.CORE.config[CONF_ESPHOME][CONF_PLATFORMIO_OPTIONS]
            ['build_flags'] if 'DISABLE_NSPANEL_TFT_UPLOAD' in s]:
            enable_tft_upload = False

    # TODO: check that upload_tft() function is not used anywhere
    if enable_tft_upload:
        # FIXME when using add_define, the code wont build because of
        #       an undefined reference to 'upload_tft' during linking
        # cg.add_define("USE_NSPANEL_TFT_UPLOAD")
        # core.CORE.add_define("USE_NSPANEL_TFT_UPLOAD")
        cg.add_build_flag("-DUSE_NSPANEL_TFT_UPLOAD")
        if core.CORE.using_arduino:
            cg.add_library("WiFiClientSecure", None)
            cg.add_library("HTTPClient", None)
        elif core.CORE.using_esp_idf:
            esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_INSECURE", True)
            esp32.add_idf_sdkconfig_option(
                "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY", True
            )

    if CONF_SCREENSAVER in config:
        cg.add(nspanel.set_display_timeout(config[CONF_SLEEP_TIMEOUT]))

    locale_config = config[CONF_LOCALE]
    global translationJson
    load_translations(locale_config[CONF_LANGUAGE])
    
    # file specified, translation unknown
    if '.' in locale_config[CONF_LANGUAGE] or len(locale_config[CONF_LANGUAGE]) == 0:
        cg.add(nspanel.set_language("unknown"))
    else:
        cg.add(nspanel.set_language(locale_config[CONF_LANGUAGE]))

    cgv = []
    for k,v in translationJson.items():
        if k in REQUIRED_TRANSLATION_KEYS:
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

    for conf in config.get(CONF_INCOMING_MSG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], nspanel)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for key, value in entity_ids.items():
        cg.add(cg.RawExpression(f"auto {value} = {nspanel.create_entity(key)}"))

    screensaver_config = config.get(CONF_SCREENSAVER, None)
    screensaver_uuid = None
    if screensaver_config is None:
        cg.add(nspanel.set_display_timeout(0))
    else:
        cg.add(cg.RawStatement("{"))

        screensaver_uuid = screensaver_config[CONF_ID] if CONF_ID in screensaver_config else get_new_uuid()

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
            screensaver_left_icon = get_status_icon_statement(
                left_icon_config, 
                StatusIconItem)
            iconleft_variable = screensaver_info[0] + "_iconleft"
            cg.add(cg.RawExpression(f"auto {iconleft_variable} = {screensaver_left_icon}"))
            iconleft_variable_class = cg.global_ns.class_(iconleft_variable)
            iconleft_variable_class.op = '->'
            if left_icon_config.get(CONF_SCREENSAVER_STATUS_ICON_ALT_FONT, False):
                cg.add(iconleft_variable_class.set_alt_font(True))
            cg.add(screensaver_class.set_icon_left(iconleft_variable_class))

        if CONF_SCREENSAVER_STATUS_ICON_RIGHT in screensaver_config:
            right_icon_config = screensaver_config[CONF_SCREENSAVER_STATUS_ICON_RIGHT]
            screensaver_right_icon = get_status_icon_statement(
                right_icon_config, 
                StatusIconItem)
            iconright_variable = screensaver_info[0] + "_iconright"
            cg.add(cg.RawExpression(f"auto {iconright_variable} = {screensaver_right_icon}"))
            iconright_variable_class = cg.global_ns.class_(iconright_variable)
            iconright_variable_class.op = '->'
            if right_icon_config.get(CONF_SCREENSAVER_STATUS_ICON_ALT_FONT, False):
                cg.add(iconright_variable_class.set_alt_font(True))
            cg.add(screensaver_class.set_icon_right(iconright_variable_class))

        if CONF_SCREENSAVER_WEATHER in screensaver_config:
            entity_id = screensaver_config[CONF_SCREENSAVER_WEATHER][CONF_ENTITY_ID]
            cg.add(nspanel.set_weather_entity_id(entity_id))
            screensaver_items = []
            # 1 main weather item + 4 forecast items
            for i in range(0,5):
                screensaver_items.append(make_shared.template(screensaver_info[3]).__call__(get_new_uuid()))
            cg.add(screensaver_class.add_item_range(screensaver_items))

        cg.add(cg.RawStatement("}"))

    card_uuids = []
    visible_card_uuids = []
    for card_config in config.get(CONF_CARDS, []):
        if CONF_ID in card_config:
            card_uuids.append(card_config[CONF_ID])
        else:
            card_uuids.append(get_new_uuid())
        if card_config[CONF_CARD_HIDDEN] == False:
            visible_card_uuids.append(card_uuids[-1])
    visible_card_count = len(visible_card_uuids)
    visible_index = 0

    prev_card_uuid = next_card_uuid = None
    navleft_icon_value = get_icon(get_icon_hex("arrow-left-bold"))
    navhome_icon_value = get_icon(get_icon_hex("home"))
    navright_icon_value = get_icon(get_icon_hex("arrow-right-bold"))
    for i, card_config in enumerate(config.get(CONF_CARDS, [])):
        cg.add(cg.RawStatement("{"))
        prev_card_uuid = visible_card_uuids[visible_index - 1]
        if visible_index == (visible_card_count - 1):
            next_card_uuid = visible_card_uuids[0]
        elif (visible_index < (visible_card_count - 1)):
            next_card_uuid = visible_card_uuids[visible_index + 1]

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

        if card_config[CONF_CARD_TYPE] in [CARD_ALARM, CARD_THERMO, CARD_MEDIA]:
            if (card_config[CONF_CARD_TYPE] == CARD_ALARM):
                entity_id_key = CONF_CARD_ALARM_ENTITY_ID
            elif (card_config[CONF_CARD_TYPE] == CARD_THERMO):
                entity_id_key = CONF_CARD_THERMO_ENTITY_ID
            else:
                entity_id_key = CONF_CARD_MEDIA_ENTITY_ID
            cg.add(cg.RawExpression(
                f"auto {card_variable} = "
                f"{nspanel.create_page.template(page_info[1]).__call__(card_uuids[i], get_entity_id(card_config[entity_id_key]), title, sleep_timeout)}"))
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
                navleft_statement = cg.RawStatement(f'new {NavigationItem}(\"{get_new_uuid()}\", \"{home_uuid}\", {navhome_icon_value})')
                cg.add(cg.RawExpression(
                    f"auto {navleft_variable} = "
                    f"{unique_ptr.template(NavigationItem)}({navleft_statement})"))
                cg.add(card_class.set_nav_left(cg.global_ns.class_(navleft_variable)))
        else:
            visible_index += 1
            navleft_variable = card_variable + "_navleft"
            navleft_statement = cg.RawStatement(f'new {NavigationItem}(\"{get_new_uuid()}\", \"{prev_card_uuid}\", {navleft_icon_value})')
            cg.add(cg.RawExpression(
                f"auto {navleft_variable} = "
                # todo: esphome is escaping the icon value (e.g. u8"\uE598") due to cpp_string_escape, so having to build a raw statement instead.
                # f"{unique_ptr.template(NavigationItem).__call__(NavigationItem.new(get_new_uuid(), prev_card_uuid, navleft_icon_value))}"))
                f"{unique_ptr.template(NavigationItem)}({navleft_statement})"))
            cg.add(card_class.set_nav_left(cg.global_ns.class_(navleft_variable)))
            navright_variable = card_variable + "_navright"
            navright_statement = cg.RawStatement(f'new {NavigationItem}(\"{get_new_uuid()}\", \"{next_card_uuid}\", {navright_icon_value})')
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
            page_info[3])

        cg.add(cg.RawStatement("}"))

    if CONF_DEFAULT_CARD in config:
        # index = card_ids.get(config[CONF_DEFAULT_CARD])
        # if isinstance(index, int):
        if config[CONF_DEFAULT_CARD] in card_ids:
            # Note: can only be called after all the default page has been created
            cg.add(nspanel.set_default_page(config[CONF_DEFAULT_CARD]))

    global custom_icons
    icon_arr: list[str] = []
    for k,v in custom_icons.items():
        icon_arr.append(v[1])
    cg.add_define("CUSTOM_ICONS_SIZE", len(icon_arr))
    cg.add_global(cg.RawStatement(
        f"constexpr std::array<const esphome::{nspanel_lovelace_ns}::icon_char_t*, CUSTOM_ICONS_SIZE> "
        f"CUSTOM_ICONS {{{cg.ArrayInitializer(*icon_arr)}}};"))

    cg.add_define("USE_NSPANEL_LOVELACE")


## see: https://esphome.io/components/wifi
# c++
#   void add_sta(const WiFiAP &ap);

# wifi_ns = cg.esphome_ns.namespace("wifi")
# ManualIP = wifi_ns.struct("ManualIP")
# WiFiAP = wifi_ns.struct("WiFiAP")
# def wifi_network(config, ap, static_ip):
#     if CONF_SSID in config:
#         cg.add(ap.set_ssid(config[CONF_SSID]))
#     if CONF_PASSWORD in config:
#         cg.add(ap.set_password(config[CONF_PASSWORD]))
#     if static_ip is not None:
#         cg.add(ap.set_manual_ip(manual_ip(static_ip)))
# def manual_ip(config):
#     if config is None:
#         return None
#     return cg.StructInitializer(
#         ManualIP,
#         ("static_ip", safe_ip(config[CONF_STATIC_IP])),
#         ("gateway", safe_ip(config[CONF_GATEWAY])),
#         ("subnet", safe_ip(config[CONF_SUBNET])),
#         ("dns1", safe_ip(config.get(CONF_DNS1))),
#         ("dns2", safe_ip(config.get(CONF_DNS2))),
#     )
## in to_code()
# def add_sta(ap, network):
#     ip_config = network.get(CONF_MANUAL_IP, config.get(CONF_MANUAL_IP))
#     cg.add(var.add_sta(wifi_network(network, ap, ip_config)))
# for network in config.get(CONF_NETWORKS, []):
#     cg.with_local_variable(network[CONF_ID], WiFiAP(), add_sta, network)

# def add_card(card, card):
#     ip_config = network.get(CONF_MANUAL_IP, config.get(CONF_MANUAL_IP))
#     cg.add(var.add_card(wifi_network(network, card, ip_config)))

## other examples:
# cv.validate_id_name

# if value in cv.RESERVED_IDS:
#     raise vol.Invalid(f"ID '{value}' is reserved internally and cannot be used")

# config[CONF_NETWORKS] = cv.ensure_list(WIFI_NETWORK_STA)(network)

# if CORE.using_arduino:
#     cg.add_library("WiFi", None)
# else:
#     if CORE.using_esp_idf: