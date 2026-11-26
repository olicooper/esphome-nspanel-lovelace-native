# Development and CI

## Version policy

This repository is a fork of [olicooper/esphome-nspanel-lovelace-native](https://github.com/olicooper/esphome-nspanel-lovelace-native). It supports current ESPHome releases and maintains backward compatibility with one or two previous monthly releases where practical. No support is promised for releases ahead of their release, and older releases are not maintained.

The minimum supported version is **ESPHome 2026.11.0** and may increase over time. ESP-IDF is required; Arduino is not supported. The minimum is enforced by `cv.require_esphome_version`, so the code carries no version guards for older releases and version-specific compatibility code is removed when the minimum moves up.

## CI matrix

The [Build workflow](../.github/workflows/build.yaml) runs on pushes and pull requests to `dev` that touch the component, examples, CI configs, tests, or the workflow itself. It has three blocking jobs:

- **Host helper tests** — `tests/host/run.sh` builds and runs the C++ parsing, weather and command-handling tests with the same `-fno-exceptions` flags ESP-IDF uses, so no device is needed.
- **Firmware matrix** — compiles the `basic`, `advanced`, `codegen`, and `codegen-no-tft` configurations on the supported floor (`2026.11.0`) and `stable`. The `codegen-no-tft` leg keeps the `USE_NSPANEL_TFT_UPLOAD`-disabled path covered at both versions.
- **Unit tests (floor)** — runs `python -m pytest -q tests` once in the floor-pinned image, covering config validation, code generation, and golden fixtures.

A `CI required` check aggregates the three jobs; configure it as a required status check in branch protection after it appears in GitHub. A manual, `workflow_dispatch`-only job compiles the codegen fixture against the official `beta` image as an **advisory preview**. A passing preview does not make beta a supported version: when `stable` resolves to the floor, the two matrix legs duplicate each other, and the floor stays pinned.

## What the checks do not cover

The host tests, pytest, and `esphome config` / `esphome compile` prove code correctness only. TFT field layout, PSRAM allocation, upload baud rate and timing, and HMI behaviour need a physical panel. Transfer, build, OTA, and flashing each require separate owner approval.

See [TFT firmware and upload](tft-upload.md) for panel-side caveats and [Screensaver weather](screensaver-weather.md#esphome-upgrade-check) for the upgrade-check procedure.

[← Back to README](../README.md#contributing)
