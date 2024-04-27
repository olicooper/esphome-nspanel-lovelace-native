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
    # NOTE: Cannot support PSRAM on arduino due to non-default pin configuration
    if core.CORE.using_esp_idf:
        val.append("psram")
        return val
    return val

_LOGGER = logging.getLogger(__name__)

entity_ids: dict[str] = {}
entity_id_index = 0
uuid_index = 0
iconJson = None
make_shared = cg.std_ns.class_("make_shared")
unique_ptr = cg.std_ns.class_("unique_ptr")
nspanel_lovelace_ns = cg.esphome_ns.namespace("nspanel_lovelace")
NSPanelLovelace = nspanel_lovelace_ns.class_("NSPanelLovelace", cg.Component, uart.UARTDevice)
DOW = nspanel_lovelace_ns.class_("DayOfWeekMap").enum("dow")

ALARM_ARM_ACTION = nspanel_lovelace_ns.enum("alarm_arm_action", True)
ALARM_ARM_OPTIONS = ['arm_home','arm_away','arm_night','arm_vacation']
ALARM_ARM_OPTION_MAP = {
    'arm_home': [ALARM_ARM_ACTION.arm_home, "Arm Home"],
    'arm_away': [ALARM_ARM_ACTION.arm_away, "Arm Away"],
    'arm_night': [ALARM_ARM_ACTION.arm_night, "Arm Night"],
    'arm_vacation': [ALARM_ARM_ACTION.arm_vacation, "Arm Vacation"],
}

NSPanelLovelaceMsgIncomingTrigger = nspanel_lovelace_ns.class_(
    "NSPanelLovelaceMsgIncomingTrigger",
    automation.Trigger.template(cg.std_string)
)

ENTITY_ID_RE = re.compile(r"^(?:(delete)|([\w]+[A-Za-z0-9]\.[\w]+[A-Za-z0-9])|(iText.[^~]*?))$")
## The list of currently supported entities
ENTITY_TYPES = [
    'sensor','binary_sensor','light','switch','scene','timer','weather','navigate',
    'alarm_control_panel','input_boolean','input_button','cover','fan','automation',
    'script'
]

CARD_ENTITIES="cardEntities"
CARD_GRID="cardGrid"
CARD_GRID2="cardGrid2"
CARD_QR="cardQR"
CARD_ALARM="cardAlarm"

CONF_INCOMING_MSG = "on_incoming_msg"
CONF_ICON = "icon"
CONF_ICON_VALUE = "value"
CONF_ICON_COLOR = "color"
CONF_ENTITY_ID = "entity_id"
CONF_SLEEP_TIMEOUT = "sleep_timeout"

CONF_LOCALE = "locale"
CONF_DAY_OF_WEEK_MAP = "day_of_week_map"
CONF_DOW_SUNDAY = "sunday"
CONF_DOW_MONDAY = "monday"
CONF_DOW_TUESDAY = "tuesday"
CONF_DOW_WEDNESDAY = "wednesday"
CONF_DOW_THURSDAY = "thursday"
CONF_DOW_FRIDAY = "friday"
CONF_DOW_SATURDAY = "saturday"

CONF_SCREENSAVER = "screensaver"
CONF_SCREENSAVER_DATE_FORMAT = "date_format"
CONF_SCREENSAVER_TIME_FORMAT = "time_format"
CONF_SCREENSAVER_WEATHER = "weather"
CONF_SCREENSAVER_STATUS_ICON_LEFT = "status_icon_left"
CONF_SCREENSAVER_STATUS_ICON_RIGHT = "status_icon_right"
CONF_SCREENSAVER_STATUS_ICON_ALT_FONT = "alt_font" # todo: to_code

CONF_CARDS = "cards"
CONF_CARD_TYPE = "type"
CONF_CARD_HIDDEN = "hidden"
CONF_CARD_TITLE = "title"
CONF_CARD_SLEEP_TIMEOUT = "sleep_timeout"
CONF_CARD_ENTITIES = "entities"
CONF_CARD_ENTITIES_NAME = "name"

CONF_CARD_QR_TEXT = "qr_text"
CONF_CARD_ALARM_ENTITY_ID = "alarm_entity_id"
CONF_CARD_ALARM_SUPPORTED_MODES = "supported_modes"

def load_icons():
    global iconJson
    iconJsonPath = core.CORE.relative_build_path("nspanel_icons.json")
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

def valid_entity_id(entity_type_whitelist: list[str] = None):
    """Validate that a given entity_id is correctly formatted and present in the entity type whitelist."""
    def validator(value):
        value = cv.string_strict(value)
        if not value:
            return value
        if re.match(ENTITY_ID_RE, value):
            if value == 'delete' or value.startswith('iText.'):
                return value
            if [t for t in ENTITY_TYPES if value.startswith(t)]:
                if entity_type_whitelist is None:
                    return value
                elif [w for w in entity_type_whitelist if value.startswith(w)]:
                    return value
                raise cv.Invalid(
                    f"entity type '{value.split('.')[0]}' is not allowed here. Allowed types are: {entity_type_whitelist}"
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

def ensure_unique(value: list):
    all_values = list(value)
    unique_values = set(value)
    if len(all_values) != len(unique_values):
        raise cv.Invalid("Mapping values must be unique.")
    return value

SCHEMA_DOW_ITEM = cv.All(
    cv.ensure_list(cv.string_strict), 
    cv.Length(2, 2, 'There must be exactly 2 items specified, the short form then the long form e.g. ["Sun", "Sunday"]'),
)

SCHEMA_DOW_MAP = cv.Schema({
    cv.Optional(CONF_DOW_SUNDAY): SCHEMA_DOW_ITEM,
    cv.Optional(CONF_DOW_MONDAY): SCHEMA_DOW_ITEM,
    cv.Optional(CONF_DOW_TUESDAY): SCHEMA_DOW_ITEM,
    cv.Optional(CONF_DOW_WEDNESDAY): SCHEMA_DOW_ITEM,
    cv.Optional(CONF_DOW_THURSDAY): SCHEMA_DOW_ITEM,
    cv.Optional(CONF_DOW_FRIDAY): SCHEMA_DOW_ITEM,
    cv.Optional(CONF_DOW_SATURDAY): SCHEMA_DOW_ITEM,
})

SCHEMA_LOCALE = cv.Schema({
    cv.Optional(CONF_DAY_OF_WEEK_MAP): SCHEMA_DOW_MAP,
})

SCHEMA_ICON = cv.Any(
    cv.string_strict, # icon name
    cv.Schema({
        cv.Optional(CONF_ICON_VALUE): valid_icon_value,
        cv.Optional(CONF_ICON_COLOR): cv.int_range(0, 65535),
    }),
    msg=f"Must be a string or a dictionary with {CONF_ICON_VALUE} and {CONF_ICON_COLOR} attributes"
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
    cv.Optional(CONF_CARD_HIDDEN, default=False): cv.boolean,
    cv.Optional(CONF_CARD_TITLE): cv.string,
    # timeout range from 2s to 12hr
    cv.Optional(CONF_CARD_SLEEP_TIMEOUT, default=10): cv.int_range(2, 43200)
})

def add_entity_id(id: str):
    global entity_ids, entity_id_index
    entity_ids[id] = f"nspanel_e{entity_id_index}"
    entity_id_index += 1

def validate_config(config):
    # if int(config[CONF_BERRY_DRIVER_VERSION]) > 0:
    #     if "CustomSend" not in config[CONF_MQTT_SEND_TOPIC]:
    #         # backend uses topic_send.replace("CustomSend", ...) for GetDriverVersion and FlashNextion
    #         raise cv.Invalid(f"{CONF_MQTT_SEND_TOPIC} must contain \"CustomSend\" for correct backend compatibility.\n"
    #                          f"Either change it or set {CONF_BERRY_DRIVER_VERSION} to 0.")
    # _LOGGER.info(config)

    # Build a list of custom card ids
    card_ids = []
    for card_config in config.get(CONF_CARDS, []):
        if CONF_ID in card_config:
            card_ids.append(card_config[CONF_ID])
    # Check that all 'navigate' entity_ids are valid
    for card_config in config.get(CONF_CARDS, []):
        if CONF_CARD_ENTITIES in card_config:
            for entity_config in card_config.get(CONF_CARD_ENTITIES, []):
                entity_id = entity_config.get(CONF_ENTITY_ID)
                if entity_id.startswith('navigate'):
                    entity_arr = entity_id.split('.', 1)
                    # if len(entity_arr) != 2:
                    #     raise cv.Invalid(f'The entity_id "{entity_id}" format is invalid')
                    if entity_arr[1] not in card_ids:
                        raise cv.Invalid(f'navigation entity_id invalid, no card has the id "{entity_arr[1]}"')
                # Add all valid HA entities to global entity list for later processing
                # elif not (entity_id.startswith('iText') or entity_id.startswith('delete')):
                add_entity_id(entity_id)
        if CONF_CARD_ALARM_ENTITY_ID in card_config:
            add_entity_id(card_config.get(CONF_CARD_ALARM_ENTITY_ID))

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
        # cv.GenerateID(CONF_MQTT_PARENT_ID): cv.use_id(mqtt.MQTTClientComponent),
        cv.Optional(CONF_SLEEP_TIMEOUT, default=10): cv.int_range(2, 43200),
        cv.Optional(CONF_LOCALE): SCHEMA_LOCALE,
        cv.Optional(CONF_SCREENSAVER, default={}): SCHEMA_SCREENSAVER,
        cv.Optional(CONF_INCOMING_MSG): automation.validate_automation(
            cv.Schema({
                cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(NSPanelLovelaceMsgIncomingTrigger),
            })
        ),
        cv.Optional(CONF_CARDS): cv.ensure_list(
            cv.typed_schema({
                CARD_ENTITIES: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY)
                }),
                CARD_GRID: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY)
                }),
                CARD_GRID2: SCHEMA_CARD_BASE,
                CARD_QR: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ENTITIES): cv.ensure_list(SCHEMA_CARD_ENTITY),
                    cv.Optional(CONF_CARD_QR_TEXT): cv.string_strict
                }),
                CARD_ALARM: SCHEMA_CARD_BASE.extend({
                    cv.Required(CONF_CARD_ALARM_ENTITY_ID): valid_entity_id(['alarm_control_panel']),
                    cv.Optional(CONF_CARD_ALARM_SUPPORTED_MODES, default=ALARM_ARM_OPTIONS): 
                        cv.All(
                            cv.ensure_list(cv.one_of(*ALARM_ARM_OPTIONS)),
                            cv.Length(1, 4, f"Must be a list of up to 4 items from the following list: {ALARM_ARM_OPTIONS}"),
                            ensure_unique
                        )
                }),
            },
            default_type=CARD_GRID)
        ),
    })
    .extend(uart.UART_DEVICE_SCHEMA)
    .extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    validate_config
)

Screensaver = nspanel_lovelace_ns.class_("Screensaver")
EntitiesCard = nspanel_lovelace_ns.class_("EntitiesCard")
GridCard = nspanel_lovelace_ns.class_("GridCard")
# GridCard2 = nspanel_lovelace_ns.class_("GridCard2")
QRCard = nspanel_lovelace_ns.class_("QRCard")
AlarmCard = nspanel_lovelace_ns.class_("AlarmCard")

NavigationItem = nspanel_lovelace_ns.class_("NavigationItem")
StatusIconItem = nspanel_lovelace_ns.class_("StatusIconItem")
WeatherItem = nspanel_lovelace_ns.class_("WeatherItem")
EntitiesCardEntityItem = nspanel_lovelace_ns.class_("EntitiesCardEntityItem")
GridCardEntityItem = nspanel_lovelace_ns.class_("GridCardEntityItem")
AlarmButtonItem = nspanel_lovelace_ns.class_("AlarmButtonItem")

PAGE_MAP = {
    # [config type] : [c++ variable name prefix], [card class], [entity class]
    CONF_SCREENSAVER: ["nspanel_screensaver", Screensaver, WeatherItem],
    CARD_ENTITIES: ["nspanel_card_", EntitiesCard, EntitiesCardEntityItem],
    CARD_GRID: ["nspanel_card_", GridCard, GridCardEntityItem],
    # CARD_GRID2: ["nspanel_card_", GridCard2, GridCardEntityItem],
    CARD_QR: ["nspanel_card_", QRCard, EntitiesCardEntityItem],
    CARD_ALARM: ["nspanel_card_", AlarmCard, AlarmButtonItem]
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
        attrs["value"] = get_icon_hex(attrs["value"])
        if isinstance(attrs["value"], str):
            # Make sure it is properly formatted for c++
            attrs["value"] = r'u8"\u{0}"'.format(attrs["value"])
            # todo: esphome is escaping the icon value (e.g. u8"\uE598") due to cpp_string_escape, so having to build a raw statement instead.
            if isinstance(parent_class, cg.MockObj):
                cg.add(cg.RawStatement(f'{parent_class.__str__()}->set_icon_value({attrs["value"]});'))
                # cg.add(parent_class.set_icon_value(attrs["value"]))
    if isinstance(attrs["color"], int):
        if isinstance(parent_class, cg.MockObj):
            cg.add(parent_class.set_icon_color(attrs["color"]))
    if parent_class is None:
        return attrs

def gen_card_entities(entities_config, card_class: cg.MockObjClass, id_prefix: str, entity_type: cg.MockObjClass):
    for i, entity_config in enumerate(entities_config):
        variable_name = id_prefix + "_item_" + str(i + 1)
        entity_class = cg.global_ns.class_(variable_name)
        entity_class.op = "->"

        entity_id = get_entity_id(entity_config.get(CONF_ENTITY_ID, "delete"))
        display_name = entity_config.get(CONF_CARD_ENTITIES_NAME, None)
        # if display_name != None:
        #     entity_class = cg.new_Pvariable(variable_name, entity_config[CONF_CARD_ENTITIES_ID], entity_config[CONF_CARD_ENTITIES_NAME])
        # else:
        #     entity_class = cg.new_Pvariable(variable_name, entity_config[CONF_CARD_ENTITIES_ID])
        cg.add(cg.RawExpression(
            f"auto {variable_name} = "
            f"{make_shared.template(entity_type).__call__(get_new_uuid(), entity_id, display_name)}"))

        generate_icon_config(entity_config.get(CONF_ICON, None), entity_class)

        cg.add(card_class.add_item(entity_class))

def get_status_icon_statement(icon_config, icon_class: cg.MockObjClass, default_icon_value: str = 'alert-circle-outline'):
    entity_id = get_entity_id(icon_config.get(CONF_ENTITY_ID))
    default_icon_value = r'u8"\u{0}"'.format(get_icon_hex(default_icon_value))
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
    nspanel = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(nspanel, config)
    await uart.register_uart_device(nspanel, config)
    cg.add_define("USE_NSPANEL_LOVELACE")
    cg.add_global(nspanel_lovelace_ns.using)

    # NSPanel has non-standard PSRAM pins which are not
    # modifiable when building for Arduino
    if core.CORE.using_esp_idf:
        # FIXME: This doesnt work (sdkconfig file doesnt change), 'to_code' priority wrong?
        # esp32.add_idf_sdkconfig_option("CONFIG_D0WD_PSRAM_CLK_IO", '5')
        # esp32.add_idf_sdkconfig_option("CONFIG_D0WD_PSRAM_CS_IO", '18')
        # todo: this is an arduino define, is this okay to keep using?
        cg.add_define("BOARD_HAS_PSRAM")

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
        core.CORE.add_build_flag("-DUSE_NSPANEL_TFT_UPLOAD")
        if core.CORE.using_arduino:
            cg.add_library("WiFiClientSecure", None)
            cg.add_library("HTTPClient", None)
        elif core.CORE.using_esp_idf:
            esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_INSECURE", True)
            esp32.add_idf_sdkconfig_option(
                "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY", True
            )

    if CONF_SLEEP_TIMEOUT in config:
        cg.add(nspanel.set_display_timeout(config[CONF_SLEEP_TIMEOUT]))

    if CONF_LOCALE in config:
        locale_config = config[CONF_LOCALE]
        if CONF_DAY_OF_WEEK_MAP in locale_config:
            dow_config = locale_config[CONF_DAY_OF_WEEK_MAP]
            if CONF_DOW_SUNDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.sunday, dow_config[CONF_DOW_SUNDAY]))
            if CONF_DOW_MONDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.monday, dow_config[CONF_DOW_MONDAY]))
            if CONF_DOW_TUESDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.tuesday, dow_config[CONF_DOW_TUESDAY]))
            if CONF_DOW_WEDNESDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.wednesday, dow_config[CONF_DOW_WEDNESDAY]))
            if CONF_DOW_THURSDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.thursday, dow_config[CONF_DOW_THURSDAY]))
            if CONF_DOW_FRIDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.friday, dow_config[CONF_DOW_FRIDAY]))
            if CONF_DOW_SATURDAY in dow_config:
                cg.add(nspanel.set_day_of_week_override(
                    DOW.saturday, dow_config[CONF_DOW_SATURDAY]))

    for conf in config.get(CONF_INCOMING_MSG, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], nspanel)
        await automation.build_automation(trigger, [(cg.std_string, "x")], conf)

    for key, value in entity_ids.items():
        cg.add(cg.RawExpression(f"auto {value} = {nspanel.create_entity(key)}"))

    screensaver_config = config.get(CONF_SCREENSAVER, None)
    screensaver_uuid = None
    if screensaver_config is not None:
        cg.add(cg.RawStatement("{"))

        screensaver_uuid = screensaver_config[CONF_ID] if CONF_ID in screensaver_config else get_new_uuid()

        if CONF_TIME_ID in screensaver_config:
            time_ = await cg.get_variable(screensaver_config[CONF_TIME_ID])
            cg.add(nspanel.set_time_id(time_))

        if CONF_SCREENSAVER_DATE_FORMAT in screensaver_config:
            cg.add(nspanel.set_date_format(screensaver_config[CONF_SCREENSAVER_DATE_FORMAT]))
        if CONF_SCREENSAVER_TIME_FORMAT in screensaver_config:
            cg.add(nspanel.set_time_format(screensaver_config[CONF_SCREENSAVER_TIME_FORMAT]))

        screensaver_info = PAGE_MAP[CONF_SCREENSAVER]
        screensaver_class = cg.global_ns.class_(screensaver_info[0])
        screensaver_class.op = "->"

        cg.add(cg.RawExpression(
            f"auto {screensaver_info[0]} = "
            f"{nspanel.insert_page.template(screensaver_info[1]).__call__(0, screensaver_uuid)}"))

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
                screensaver_items.append(make_shared.template(screensaver_info[2]).__call__(get_new_uuid()))
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
    navleft_icon_value = r'u8"\u{0}"'.format(get_icon_hex("arrow-left-bold"))
    navhome_icon_value = r'u8"\u{0}"'.format(get_icon_hex("home"))
    navright_icon_value = r'u8"\u{0}"'.format(get_icon_hex("arrow-right-bold"))
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

        sleep_timeout = card_config.get(CONF_CARD_SLEEP_TIMEOUT, 10)
        # if isinstance(sleep_timeout, int):
        #     cg.add(card_class.set_sleep_timeout(sleep_timeout))
        #     cg.add(cg.RawExpression(f"{card_variable}->set_sleep_timeout({sleep_timeout})"))

        if card_config[CONF_CARD_TYPE] == CARD_ALARM:
            cg.add(cg.RawExpression(
                f"auto {card_variable} = "
                f"{nspanel.create_page.template(page_info[1]).__call__(card_uuids[i], get_entity_id(card_config[CONF_CARD_ALARM_ENTITY_ID]), title, sleep_timeout)}"))
        else:
            cg.add(cg.RawExpression(
                f"auto {card_variable} = "
                f"{nspanel.create_page.template(page_info[1]).__call__(card_uuids[i], title, sleep_timeout)}"))
            # cg.add(cg.variable(card_variable, make_shared.template(page_info[1]).__call__(cg.global_ns.class_(page_info[0] + str(i + 1)))))

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
        if card_config[CONF_CARD_TYPE] == CARD_ALARM:
            for mode in card_config[CONF_CARD_ALARM_SUPPORTED_MODES]:
                cg.add(card_class.set_arm_button(ALARM_ARM_OPTION_MAP[mode][0], ALARM_ARM_OPTION_MAP[mode][1]))

        gen_card_entities(
            card_config.get(CONF_CARD_ENTITIES, []), 
            card_class, 
            card_variable, 
            page_info[2])

        cg.add(cg.RawStatement("}"))


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