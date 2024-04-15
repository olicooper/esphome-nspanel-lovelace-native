# ESPHome Native Component for NSPanel (beta)

This project is designed to utilise the UI provided by [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui), but use a native [ESPHome](https://github.com/esphome/esphome) component for the backend. My core goals for the project were to:
- Reduce bandwidth consumption - by utilising the efficient TCP communication implemented by ESPHome.
- Remove the need for an MQTT broker and make the device able to function on a minimal level during network outages.
- Make the screen more responsive by having the ESP32 do some of the heavy lifting (the ESP32 is actually quite powerful!)

A special thanks goes to these great projects for making this possible:
- [NSPanel Lovelace UI](https://github.com/joBr99/nspanel-lovelace-ui)
- [ESPHome NSPanel Lovelace UI](https://github.com/sairon/esphome-nspanel-lovelace-ui)
- [ESPHome](https://github.com/esphome/esphome)

# Installation

The installation of ESPHome on the ESP32 follows the standard ESPHome build method e.g. `esphome run --device COM6 basic-example.yaml`. It is possible to do OTA updates with the ESPHome CLI after the initial upload.

For the GUI, this project relies on the HMI TFT firmware from the [ESPHome NSPanel Lovelace UI](https://github.com/sairon/esphome-nspanel-lovelace-ui) project. 
The firmware version reported by the screen needs to be `52` or `53` for it to be compatible with this project.
If you don't have the custom HMI TFT firmware installed already you will need to upload the appropriate `nspanel*.tft` file from `v4.3.3` of the project which can be [found here](https://github.com/joBr99/nspanel-lovelace-ui/tree/v4.3.3/HMI). You can upload the correct TFT firmware after installing this component on the ESP32 by using the `upload_tft` service (seen in the [basic example](basic-example.yaml#L139)) exposed by the device in HomeAssistant.

A basic configuration can be [found here](basic-example.yaml) which shows the features currently available. This is loosely based on the appdaemon configuration format ([found here](https://github.com/joBr99/nspanel-lovelace-ui/blob/v4.3.3/appdaemon/apps-simple.yaml)) to make it easier to transition to this native ESPHome solution, but please don't expect this to translate exactly as it is not possible to make it work in the exact same way given the limitations of the ESP32.

# Help Needed!

As much as I love embedded programming, I am unable to dedicate the time required to make this project great without the help of others.
This is a **beta project** which is not fully functional yet and is subject to change without notice. It has taken a lot of time and effort to get the project to this point and it will take a lot more time to get the project past beta.

There are many UI components missing and the [python build script](components/nspanel_lovelace/__init__.py) is in dire need of refactoring (I butchered it together to get it working but PRs to the ESPHome repo are required to provide more flexibility over code generation).

Currently the following features work:
- Screensaver with time, date, weather and status icon display
- Support for `cardGrid`, `cardEntities`, `cardQR`, `cardAlarm`
- Most entity types should display on cards. Lights, switches, sensors and scenes have been tested to work, with additional support for the `popupLight` page.

There is currently no support for cards such as: `cardMedia`, `cardThermo`, `cardPower` etc. but these are planned for the future.
Please see the [HMI readme](https://github.com/joBr99/nspanel-lovelace-ui/tree/main/HMI) for more info on the cards mentioned above.

PRs to expand the functionality or fix bugs are very welcome!

# License

Code in this repository is licensed under the GPLv3 license. Third-party code used in this project have their own license terms. Please see [the license document](LICENSE) for more information.