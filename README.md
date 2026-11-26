# ESPHome Native Component for NSPanel (beta)

This project is designed to utilise the UI provided by [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui), but use a native [ESPHome](https://github.com/esphome/esphome) component for the backend. My core goals for the project were to:
- Reduce bandwidth consumption - by utilising the efficient TCP communication implemented by ESPHome.
- Remove the need for an MQTT broker and make the device able to function on a minimal level during network outages.
- Make the screen more responsive by having the ESP32 do some of the heavy lifting (the ESP32 is actually quite powerful!)

A special thanks goes to these great projects for making this possible:
- [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui)
- [ESPHome NSPanel Lovelace UI](https://github.com/sairon/esphome-nspanel-lovelace-ui)
- [ESPHome](https://github.com/esphome/esphome)

## Fork and ESPHome version policy

This repository is a fork of [olicooper/esphome-nspanel-lovelace-native](https://github.com/olicooper/esphome-nspanel-lovelace-native). It aims to support current ESPHome releases and maintain backward compatibility for one or two previous monthly releases where practical; compatibility with older releases is not maintained. There is no commitment to support future releases ahead of their release.

The current minimum supported version is ESPHome 2026.11.0; the minimum may increase as this fork moves forward. ESP-IDF is required, and Arduino is not supported.

Continuous integration compiles the `basic`, `advanced`, `codegen` and `codegen-no-tft` configurations on the supported floor (`2026.11.0`) and `stable`. A separate floor-pinned job runs the Python unit and code-generation tests once, and a host job runs the C++ tests. The `CI required` check requires all three to pass (firmware matrix, Python tests, and C++ host tests); configure it as a required status check in branch protection after it appears in GitHub. A manual (`workflow_dispatch`-only) job compiles the codegen fixture against the official `beta` image as an advisory preview: a passing preview does not make beta a supported version.

Because the minimum version is enforced by `cv.require_esphome_version`, the code carries no `ESPHOME_VERSION_CODE` guards for older releases; version-specific compatibility code is removed when the minimum moves up.

## Tests

`tests/host/run.sh` builds and runs the host-side helper tests (string and number parsing) with the same `-fno-exceptions` flags ESP-IDF uses, so they need no device. Both example configurations are compiled against the minimum supported ESPHome version by the Build workflow.

# Installation

The examples use the local component in this checkout. Copy `examples/secrets-example.yaml` to `examples/secrets.yaml` and fill in your values, then use the standard ESPHome build method, e.g. `esphome run --device COM6 examples/basic-example.yaml`. It is possible to do OTA updates with the ESPHome CLI after the initial upload.

For the GUI, this project relies on the HMI TFT firmware from the [ESPHome NSPanel Lovelace UI](https://github.com/sairon/esphome-nspanel-lovelace-ui) project. 
The firmware version reported by the screen needs to be `52` or `53` for it to be compatible with this project.
If you don't have the custom HMI TFT firmware installed already you will need to upload the appropriate `nspanel*.tft` file from `v4.3.3` of the project which can be [found here](https://github.com/joBr99/nspanel-lovelace-ui/tree/v4.3.3/HMI). You can upload the correct TFT firmware after installing this component on the ESP32 by using the `upload_tft` service (seen in the [basic example](examples/basic-example.yaml#L62)) exposed by the device in HomeAssistant.

When uploading a TFT:
- Serve the file from a server that supports HTTP Range requests, which the uploader uses for every block. Home Assistant's `/local` folder (`config/www`) does.
- An upload takes a few minutes. When it finishes, the display restarts by itself and the ESP reboots once the display has started. Don't cut the power before then.
- The [basic example](examples/basic-example.yaml) tries the upload up to 3 times in total, power-cycling the display before each attempt. If an upload is interrupted, the display stays blank until it is flashed again; turning it off and on with the `Screen Power` switch and uploading again recovers it.

# Configuration

A basic configuration can be found in the [basic example](examples/basic-example.yaml), but you'll probably also want to look at the [advanced example](examples/advanced-example.yaml) which shows the features currently available. This is loosely based on the appdaemon configuration format ([found here](https://github.com/joBr99/nspanel-lovelace-ui/blob/v4.3.3/appdaemon/apps-simple.yaml)) to make it easier to transition to this native ESPHome solution, but please don't expect this to translate exactly as it is not possible to make it work in the exact same way given the limitations of the ESP32.

### Arbitrary service tiles

A tile with an `entity_id` in the `service.*` domain can call any Home Assistant service when tapped:

```yaml
- entity_id: service.power_off
  name: Standby
  service: remote.send_command
  data:
    entity_id: remote.example_remote
    command: [standby]
```

`service` is a lowercase `domain.service_name` string and is only valid on a `service.*` tile. `data` is required on those tiles and must contain a string `entity_id`. String values are sent as-is; lists, numbers and booleans are sent through HA's `data_template` as Python literals, so they arrive as their original type. This depends on the Home Assistant version parsing rendered template results; builds without that parsing deliver the literal text instead. Give each distinct action its own `service.*` tile id. See the [advanced example](examples/advanced-example.yaml) for a worked tile.

## PSRAM

The NSPanel has on-board PSRAM which this project makes use of automatically, which means that the `psram` component is unavailable because the PSRAM is configured during code generation by adding specific `sdkconfig_options`. The ESP-IDF framework is required; Arduino is not supported as it is not possible to customise the PSRAM pins for the Arduino framework build.

### Icons

- Icon values can be an icon name or hex value (e.g. `hex:E549`). A list of icons can be found here: https://docs.nspanel.pky.eu/icon-cheatsheet.html
- Icon colours accept an RGB565 number (`0`-`65535`), a quoted 24-bit hex string (`"#RRGGBB"` or `"RRGGBB"`), or an `[r, g, b]` list (each `0`-`255`). An [RGB565 picker](https://chrishewett.com/blog/true-rgb565-colour-picker/) shows a 4-digit RGB565 code such as `#ce79`; use its decimal value `52857` (the short hex form itself is not a colour input).
- Screensaver weather icons use their condition's default colour. To pin them to one colour, set `screensaver.weather.icon_color` to any of the formats above; it applies to the main icon and all four forecast icons.

### Sleep timeouts

The `sleep_timeout` values (top-level and per-card) are limited to `0`-`65` seconds because the HMI firmware protocol caps the screensaver timeout at 65s. Values `66`-`120` were previously accepted for per-card timeouts but are now rejected. `0` disables the screensaver for that timeout.

### Custom translations

`locale.language` is either a bundled language code (e.g. `en`, `de`) or a custom translation file. Custom files are resolved **relative to the directory of the top-level YAML configuration** (e.g. `language: translations/custom.json` next to the config), or as an absolute path. If the file is not found there, the legacy working-directory path is still tried and a warning is logged — place the file next to the configuration instead. The file must be a JSON object of string keys to string values containing all the bundled translation keys (see `components/nspanel_lovelace/translations/en.json`); extra keys are allowed. Translation problems are reported at `locale.language` during `esphome config`, before compilation.


# Help Needed!

As much as I love embedded programming, I am unable to dedicate the time required to make this project great without the help of others.
This is a **beta project** which is not fully functional yet and is subject to change without notice. It has taken a lot of time and effort to get the project to this point and it will take a lot more time to get the project past beta.

There are many UI components missing. The [python build script](components/nspanel_lovelace/__init__.py) has since been hardened — stricter config validation, correct codegen and build-flag handling, with unit tests run against a pinned ESPHome floor (2026.11.0) — and structurally refactored: per-configuration codegen state, separated resource handling ([resources.py](components/nspanel_lovelace/resources.py)) and named codegen helpers, with byte-identical generated output verified. It remains a large single module (~1000 lines), and PRs to the ESPHome repo are still required for deeper code-generation flexibility.

Currently the following features work:
- Screensaver with time, date, weather and status icon display
- Support for `cardGrid`, `cardGrid2`, `cardEntities`, `cardQR`, `cardAlarm`, `cardThermo`, `cardMedia`, `cardPower`
- Most entity types should display on cards. Lights, switches, sensors and scenes have been tested to work, with additional support for the `popupLight` and `popupTimer` pages.

There is currently no support for these cards: `cardUnlock`, `cardChart` - but these are planned for the future.
Please see the [HMI readme](https://github.com/joBr99/nspanel-lovelace-ui/tree/main/HMI) for more info on the cards mentioned above.

## Power Card (`cardPower`)

The power card is designed to display a home energy overview. A central home entity (e.g. a whole-home power sensor) is shown in the middle of the card with up to six surrounding entities (e.g. solar, EV charger, heat pump) displayed around it, each with an animated flow indicator showing the direction and magnitude of power.

### Configuration

| Key | Required | Description |
|-----|----------|-------------|
| `type` | Yes | Must be `cardPower` |
| `home_entity_id` | Yes | A `sensor` entity representing whole-home power consumption/generation |
| `title` | No | Card title. Also used as the label for the home entity (defaults to `Home` if not set) |
| `sleep_timeout` | No | Seconds before returning to the screensaver (default: `10`) |
| `entities` | No | Up to 6 power sensor entities displayed around the home entity |

Each entry in `entities` supports the options `entity_id`, `name`, `icon`, and `icon_color`.

> **Note:** Unused slots around the home entity are padded automatically. Explicit `entity_id: delete` placeholders are also supported and now render with the correct 7-field `cardPower` layout; automatic padding is still the simpler choice.

### Colours

Icons automatically switch between two colours to indicate power flow state:
- **Active** (power ≥ 1 W/0.001 kW): the configured `icon: color`, or HA blue (`17299`) by default
- **Inactive** (zero, unavailable, or unknown): grey (`#808080`, `33808`)

Set `icon: color` per entity for Tesla-style colour coding, e.g. yellow for solar, green for EV, red for grid import. See [icon colour picker](https://chrishewett.com/blog/true-rgb565-colour-picker/) for rgb565 values.

### Units

Both the home entity and the surrounding entities handle `W` and `kW` units automatically:

- **W**: values ≥ 1000 W are displayed as kW (e.g. `1.2kW`); smaller values as whole watts (e.g. `450W`)
- **kW**: displayed with two decimal places (e.g. `2.35kW`)
- **Other / no unit**: the raw state value is shown as-is

The animated speed indicator on each surrounding entity scales with the power value:
- `W` unit: `speed = watts / 20`, clamped to the range `[-120, 120]`
- `kW` unit: `speed = kilowatts × 50`, clamped to the same range
- Negative values animate in the reverse direction (e.g. for battery discharge or export)

Entities reporting `unknown` or `unavailable` show a `-` and a stopped animation.

### Example

```yaml
- type: cardPower
  title: Energy
  home_entity_id: sensor.home_power
  entities:
    - entity_id: sensor.solar_power
      name: Solar
      icon:
        value: solar-power
        color: 65504    # yellow (#FFFF00)
    - entity_id: sensor.ev_charger_power
      name: EV
      icon:
        value: ev-station
        color: 2016     # green (#00FF00)
    - entity_id: sensor.heat_pump_power
      name: Heat pump
      icon:
        value: heat-pump
        color: 64512    # orange (#FF8000)
```

PRs to expand the functionality or fix bugs are very welcome!

# Known Issues

### Timer popup on TFT v4.3.3

`popupTimer` shows a snapshot when opened or reopened, and refreshes when Home Assistant updates the open timer. It does not count down on screen between updates. An active timer's remaining time is calculated from Home Assistant's `finishes_at` and requires a valid configured ESPHome time source.

The TFT owns the 30-second inactivity timeout so local touches, including timer editing, can keep the popup open. Those touches are not reported to ESPHome, and every timer detail update resets the TFT's inactivity counter. Sending a live update every second would prevent the popup from timing out, so the component sends snapshots instead. This does not change the timer running in Home Assistant.

The host test verifies that a 24-hour active timer is sent as 1,440 minutes and 0 seconds, without wrapping at 60 minutes. That verifies the payload, not how it fits or renders on a physical panel. The on-screen 24-hour display and touch timeout have not been verified on hardware.

# Screensaver weather

Configure one Home Assistant weather entity. The component calls
`weather.get_forecasts` with `type: daily` and renders the earliest four
upcoming days in the screensaver, while current conditions (state,
temperature and unit) continue to come from the same entity.

```yaml
screensaver:
  time_id: homeassistant_time
  weather:
    entity_id: weather.home
```

The provider must support daily forecasts. If the reply is unsupported or
fails, current conditions stay intact and the forecast slots are blanked with
a rate-limited diagnostic. No Home Assistant automation, template sensor or
provider credential is needed. The panel uses its normal API action permission
to call `weather.get_forecasts`.

**Known limitation (ESPHome 2026.11.0 and 2026.11.1):** Four forecast requests
that never receive a reply can stop forecast updates until the panel restarts.
The losses do not have to be consecutive. A panel-side timeout or a Home
Assistant reconnect does not remove those pending requests from ESPHome.
The four-request cap prevents them from accumulating indefinitely. This does
not change the current-conditions subscription or require extra Home Assistant
setup. See [the ESPHome upgrade check](AGENTS.md#native-weather-upgrade-check)
for what needs to be fixed.

**Optional restart workaround:** To opt in, set a delay under `weather`:

```yaml
screensaver:
  time_id: homeassistant_time
  weather:
    entity_id: weather.home
    restart_after_forecast_stall: 30min
```

Omit the option or use `0s` to keep the existing polling behavior. When
opted in, the panel can try the remaining free slot after the existing
30-minute backoff without waiting for a reconnect. A timely, valid reply
restores forecasts without restarting. The restart delay starts only when a
fetch fills the fourth slot. Any processed success or error that frees a
slot, even a late reply, cancels it.

After the delay, restart still requires four occupied slots, no active
forecast response window, no TFT upload or reparse, and at least 30 seconds
of continuously observed Home Assistant state-subscription readiness. An
observed disconnect restarts that 30-second window, not the stall countdown.
On ESPHome 2026.11.0 and 2026.11.1 a closing connection can still look ready,
and a reply can be waiting to be processed, so an unnecessary restart is
possible. Once restart is committed, the panel rejects new TFT uploads.
The panel is briefly unavailable during restart, and a persistent fault can
cause another restart after a fresh stall on each boot. This is a workaround,
not a fix; recheck supported outbound callback cleanup when upgrading ESPHome.

Migration from older releases: remove `forecast_method`, replace any
`sensor.*` weather entity with `weather.*`, and disable the old forecast-push
automation only after the upgraded panel has been verified.

# License

Code in this repository is licensed under the GPLv3 license. Third-party code used in this project have their own license terms. Please see [the license document](LICENSE) for more information.