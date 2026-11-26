# TFT firmware and upload

The screen's GUI comes from the HMI TFT firmware of [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui), not from this component. The version reported by the panel needs to be `52` or `53` for it to be compatible.

## Firmware version and source

Upload the appropriate `nspanel*.tft` file from [v4.3.3 of the HMI project](https://github.com/joBr99/nspanel-lovelace-ui/tree/v4.3.3/HMI). If the panel already reports `52` or `53`, no upload is needed.

## Uploading

Once this component is running on the ESP32, Home Assistant exposes an `upload_tft` service on the device; see the [basic example](../examples/basic-example.yaml).

- Serve the file from a server that supports HTTP **Range** requests — the uploader uses one for every block. Home Assistant's `/local` folder (`config/www`) works.
- A full upload takes **a few minutes**. When it finishes the display restarts itself, and the ESP reboots once the display has started. Do not cut power before then.

## Interrupted uploads and recovery

The [basic example](../examples/basic-example.yaml) tries the upload up to **3 times** in total, power-cycling the display before each attempt.

An interrupted upload can leave the display blank until it is flashed again. Toggle the `Screen Power` switch off and on, then upload again to recover it.

## Build variants

The component can be compiled without upload support (`USE_NSPANEL_TFT_UPLOAD` disabled). CI covers that path; see [Development and CI](development.md).

[← Back to README](../README.md#install)
