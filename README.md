# ESPHome Native Component for NSPanel (beta)

Uses the UI provided by [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui) with a native [ESPHome](https://github.com/esphome/esphome) component as the backend. Core goals:

- Reduce bandwidth consumption by using ESPHome's efficient TCP communication.
- Remove the need for an MQTT broker and let the device function at a minimal level during network outages.
- Make the screen more responsive by having the ESP32 do the heavy lifting.

## Contents

- [What it is](#what-it-is)
- [Requirements](#requirements)
- [Install](#install)
- [Configure](#configure)
- [Feature reference](#feature-reference)
- [Contributing](#contributing)
- [Known issues](#known-issues)
- [License](#license)

## What it is

This is a fork of [olicooper/esphome-nspanel-lovelace-native](https://github.com/olicooper/esphome-nspanel-lovelace-native), maintained to support current ESPHome releases. It is a **beta project**, not fully functional yet, and configuration is subject to change without notice.

Thanks to [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui), [ESPHome NSPanel Lovelace UI](https://github.com/sairon/esphome-nspanel-lovelace-ui), and [ESPHome](https://github.com/esphome/esphome).

## Requirements

- **ESPHome 2026.11.0 or newer.** The minimum may increase as the fork moves forward; older releases are not maintained.
- **ESP-IDF**, not Arduino. The NSPanel's PSRAM is on non-standard pins that ESP-IDF can configure.
- **HMI TFT firmware version `52` or `53`** on the screen. See [TFT firmware and upload](docs/tft-upload.md).
- **Home Assistant** for entity states, weather forecasts, and service calls.

The fork supports one or two previous monthly releases where practical, with no commitment to future releases ahead of their release. Version policy and CI coverage are in [Development and CI](docs/development.md).

## Install

The examples use the local component in this checkout. Copy `examples/secrets-example.yaml` to `examples/secrets.yaml`, fill in your values, then build and upload with the standard ESPHome CLI, for example:

```sh
esphome run --device COM6 examples/basic-example.yaml
```

OTA updates work with the ESPHome CLI after the initial upload.

If the panel does not already report HMI firmware `52` or `53`, upload the matching `nspanel*.tft` file from v4.3.3 of the HMI project using Home Assistant's `upload_tft` service. The upload needs a HTTP Range-capable server, takes a few minutes, and can leave the screen blank if interrupted — [TFT firmware and upload](docs/tft-upload.md) has the timing, retry, and recovery details.

## Configure

Start with the [basic example](examples/basic-example.yaml), then see the [advanced example](examples/advanced-example.yaml) for the currently available features. The format is loosely based on the [AppDaemon configuration](https://github.com/joBr99/nspanel-lovelace-ui/blob/v4.3.3/appdaemon/apps-simple.yaml) to ease migration, but it cannot translate exactly given the ESP32's limitations.

### PSRAM

On-board PSRAM is used automatically and configured during code generation with `sdkconfig_options`, so the `psram` component is unavailable. ESP-IDF is required; Arduino cannot customise the PSRAM pins.

### Icons

- Icon values can be an icon name or a hex value (e.g. `hex:E549`). See the [icon cheatsheet](https://docs.nspanel.pky.eu/icon-cheatsheet.html).
- Icon colours accept an RGB565 number (`0`–`65535`), a quoted 24-bit hex string (`"#RRGGBB"` or `"RRGGBB"`), or an `[r, g, b]` list (each `0`–`255`). An [RGB565 picker](https://chrishewett.com/blog/true-rgb565-colour-picker/) shows a 4-digit code such as `#ce79`; use its decimal value `52857` (the short hex form itself is not a colour input).
- Screensaver weather icons use their condition's default colour. Set `screensaver.weather.icon_color` to pin all five icons to one colour; see [Screensaver weather](docs/screensaver-weather.md).

### Sleep timeouts

`sleep_timeout` (top-level and per-card) accepts `0`–`65535` seconds. Up to `65s` the TFT's own `timeout~N` command is used; above `65s` the component sends `timeout~0`, times the idle period on the ESP32, then sends `timeout~1` at the deadline so the TFT still dims and emits `sleepReached`. `0` disables the screensaver for that timeout. The long-timeout path is not hardware-verified yet: the TFT must still dim and report sleep after the one-second hand-off.

### Custom translations

`locale.language` is a bundled language code (e.g. `en`, `de`) or a custom translation file. Custom files resolve **relative to the directory of the top-level YAML configuration** (e.g. `language: translations/custom.json` next to the config) or by absolute path. The legacy working-directory path is still tried with a warning if the file is not found there — place it next to the configuration instead. The file must be a JSON object of string keys to string values containing all the bundled keys (see `components/nspanel_lovelace/translations/en.json`); extra keys are allowed. Problems are reported at `locale.language` during `esphome config`.

## Feature reference

- [Power Card (`cardPower`)](docs/card-power.md) — home energy overview with up to six surrounding entities.
- [Screensaver weather](docs/screensaver-weather.md) — daily forecasts, the lost-reply limitation, and the opt-in restart workaround.
- [Arbitrary service-call tiles](docs/service-call-tiles.md) — call any Home Assistant service from a `service.*` tile.

Working cards: `cardGrid`, `cardGrid2`, `cardEntities`, `cardQR`, `cardAlarm`, `cardThermo`, `cardMedia`, `cardPower`. Most entity types display on cards; lights, switches, sensors and scenes are tested, along with `popupLight` and `popupTimer`. `cardUnlock` and `cardChart` are not supported yet. See the [HMI readme](https://github.com/joBr99/nspanel-lovelace-ui/tree/main/HMI) for card details.

## Contributing

PRs to expand functionality or fix bugs are welcome. The Python build script has been hardened and split into named codegen helpers with separated resource handling, but it remains a large single module; deeper code-generation flexibility still requires PRs to ESPHome itself.

The [Build workflow](.github/workflows/build.yaml) compiles four configurations (`basic`, `advanced`, `codegen`, `codegen-no-tft`) on ESPHome 2026.11.0 and `stable`, plus host C++ tests and a floor-pinned pytest suite. A manual beta job is advisory only. See [Development and CI](docs/development.md) for the matrix, gates, and validation boundary.

## Known issues

- **Screensaver weather slots:** four forecast requests that never receive a reply can stall forecast updates until restart on ESPHome 2026.11.0/2026.11.1. An opt-in restart workaround exists; migration and upgrade-check steps are in [Screensaver weather](docs/screensaver-weather.md).
- **Timer popup on TFT v4.3.3:** `popupTimer` shows snapshots rather than counting down on screen. Remaining time is computed from `finishes_at`, so a valid ESPHome `time` source is required. The TFT owns the 30-second inactivity timeout because local touches are not reported to ESPHome. The host test checks the payload only; 24-hour on-screen rendering and touch timeout have not been verified on hardware.

## License

Code in this repository is licensed under the GPLv3 license. Third-party code used in this project has its own license terms. See [the license document](LICENSE) for more information.
