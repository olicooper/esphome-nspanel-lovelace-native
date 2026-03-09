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
If you don't have the custom HMI TFT firmware installed already you will need to upload the appropriate `nspanel*.tft` file from `v4.3.3` of the project which can be [found here](https://github.com/joBr99/nspanel-lovelace-ui/tree/v4.3.3/HMI). You can upload the correct TFT firmware after installing this component on the ESP32 by using the `upload_tft` service (seen in the [basic example](basic-example.yaml#L60)) exposed by the device in HomeAssistant.

# Configuration

A basic configuration can be found in the [basic example](basic-example.yaml), but you'll probably also want to look at the [advanced example](advanced-example.yaml) which shows the features currently available. This is loosely based on the appdaemon configuration format ([found here](https://github.com/joBr99/nspanel-lovelace-ui/blob/v4.3.3/appdaemon/apps-simple.yaml)) to make it easier to transition to this native ESPHome solution, but please don't expect this to translate exactly as it is not possible to make it work in the exact same way given the limitations of the ESP32.

## PSRAM

The NSPanel has on-board PSRAM which this project makes use of automatically, which means that the `psram` component is unavailable because the PSRAM is configured during code generation by adding specific `sdkconfig_options`. Additionally, when using Arduino (which is deprecated) the PSRAM cannot be used as it is not possible to customise the PSRAM pins for the Arduino framework build.

### Icons

- Icon values can be an icon name or hex value (e.g. `hex:E549`). A list of icons can be found here: https://docs.nspanel.pky.eu/icon-cheatsheet.html
- Icon colours need to be a 16-bit number (0-65535) representing a `rgb565` colour code. This website can be used to select your colour https://chrishewett.com/blog/true-rgb565-colour-picker/ - then you take the rgb565 hex value and convert it to a number (e.g. `#ce79` -> `52857`).


# Help Needed!

As much as I love embedded programming, I am unable to dedicate the time required to make this project great without the help of others.
This is a **beta project** which is not fully functional yet and is subject to change without notice. It has taken a lot of time and effort to get the project to this point and it will take a lot more time to get the project past beta.

There are many UI components missing and the [python build script](components/nspanel_lovelace/__init__.py) is in dire need of refactoring (I butchered it together to get it working but PRs to the ESPHome repo are required to provide more flexibility over code generation).

Currently the following features work:
- Screensaver with time, date, weather and status icon display
- Support for `cardGrid`, `cardGrid2`, `cardEntities`, `cardQR`, `cardAlarm`, `cardThermo`, `cardMedia`
- Most entity types should display on cards. Lights, switches, sensors and scenes have been tested to work, with additional support for the `popupLight` and `popupTimer` pages.

There is currently no support for these cards: `cardPower`. `cardUnlock`, `cardChart` - but these are planned for the future.
Please see the [HMI readme](https://github.com/joBr99/nspanel-lovelace-ui/tree/main/HMI) for more info on the cards mentioned above.

PRs to expand the functionality or fix bugs are very welcome!

# Known Issues

### 1. Weather forecast is not displayed on the screensaver when using Home Assistant 2024.4 or later

This issue is due to the `forecast` attribute being removed from weather entities. There is currently no alternative way to fetch this data with the current ESPHome functionality but I hope to get this fixed (see [this feature request](https://github.com/esphome/feature-requests/issues/2703)).
More info on the issue can be [found here](https://github.com/olicooper/esphome-nspanel-lovelace-native/issues/8).

**Solution**: Use the `service` forecast method to manually push weather data to the panel.
1. Update your ESPHome configuration:
    ```yaml
      screensaver:
        weather:
          entity_id: weather.home # Change this to your weather entity
          forecast_method: service
    ```
2. Add an automation to Home Assistant to push the forecast:
    ```yaml
    alias: NSPanel Lovelace Weather Forecast Sync
    description: >-
      This automation fetches the weather forecast and pushes only the upcoming
      entries to the NSPanel. It filters the forecast to only include entries
      starting from the current hour, keeping only datetime, condition, and
      temperature as these are the only attributes used by the nspanel.
    triggers:
      - minutes: /30
        trigger: time_pattern
      - entity_id: weather.home # Change this to your weather entity
        trigger: state
      ## NOTE: This makes sure the forecast is sent when the panel is turned on
      - trigger: state
        entity_id:
          - switch.nspanel_screen_power # Change this to your NSPanel 'screen_power' entity_id
        for:
          hours: 0
          minutes: 0
          seconds: 2
        to:
          - "on"
        alias: When nspanel becomes available
    conditions: []
    actions:
      - target:
          entity_id: weather.home # Change this to your weather entity
        data:
          type: hourly # or daily/twice_daily
        response_variable: forecast_data
        action: weather.get_forecasts
      - variables:
          forecast: >-
            {% set forecast = (forecast_data.values() | first).forecast %}
            {% set ns = namespace(items=[]) %}
            {% for item in forecast if as_timestamp(item.datetime) >= as_timestamp(now()) %}
              {% if (ns.items | length) < 5 %}
                {% set ns.items = ns.items + [{
                  "datetime": item.datetime | as_datetime | as_timestamp | timestamp_custom('%Y-%m-%dT%H:%M:%S'),
                  "condition": item.condition,
                  "temperature": item.temperature | float(1)
                }] %}
              {% endif %}
            {% endfor %}
            {{- ns.items }}
      - action: esphome.nspanel_set_weather_forecast_data # Change this to your NSPanel 'set_weather_forecast_data' action/service name. This can be found in HA developer tools > actions (after the nspanel has been re-configured to use the 'service' forecast_method and has connected to HA).
        data:
          forecast: "{{ forecast }}"
    mode: single
    ```

**Alternative Workaround (deprecated)**: Add a template sensor to your Home Assistant configuration (changing `weather.home` to your actual weather entity_id) then update the `weather` `entity_id` in your esphome config to the `unique_id` seen below (i.e. `sensor.weather_forecast_daily`).

```yaml
template:
  - trigger:
      - platform: time_pattern
        minutes: /30
      - platform: homeassistant
        event: start
      - platform: event
        event_type: event_template_reloaded
    action:
      - service: weather.get_forecasts
        data:
          type: daily
        target:
          entity_id: weather.home # Change this to your weather entity
        response_variable: data
    sensor:
      - name: Weather Forecast Daily
        unique_id: weather_forecast_daily # Use this id in your esphome config (screensaver -> weather -> entity_id)
        state: "{{ states('weather.home') }}" # Change this to your weather entity
        attributes:
          temperature: "{{ state_attr('weather.home', 'temperature') }}" # Change this to your weather entity
          temperature_unit: "{{ state_attr('weather.home', 'temperature_unit') }}" # Change this to your weather entity
          # Change 'weather.home' below to your weather entity
          forecast: >
            {%- with ns = namespace(arr=[],obj={}) -%}
            {%- for n in data['weather.home'].forecast[:5] -%}
              {%- for k, v in n | items -%}
                {%- if k == "datetime" -%}
                  {%- set ns.obj = dict(ns.obj | items, **{k: as_datetime(v, "0" | as_datetime) | as_local | as_timestamp | timestamp_custom('%Y-%m-%dT%H:%M:%S')}) -%}
                {%- elif k == "condition" or k == "temperature" -%}
                  {%- set ns.obj = dict(ns.obj | items, **{k: v}) -%}
                {%- endif -%}
              {%- endfor -%}
              {%- if (ns.obj.items() | length) == 3 and ns.obj.datetime -%}
                {%- set ns.arr = ns.arr + [ns.obj] -%}
              {%- endif -%}
            {%- endfor %}{{ns.arr}}{% endwith -%}
```

# License

Code in this repository is licensed under the GPLv3 license. Third-party code used in this project have their own license terms. Please see [the license document](LICENSE) for more information.