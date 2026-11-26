# Power Card (`cardPower`)

The power card displays a home energy overview. A central home entity (for example, a whole-home power sensor) is shown in the middle with up to six surrounding entities (solar, EV charger, heat pump, and so on) arranged around it, each with an animated flow indicator showing the direction and magnitude of power.

## Configuration

| Key | Required | Description |
|-----|----------|-------------|
| `type` | Yes | Must be `cardPower` |
| `home_entity_id` | Yes | A `sensor` entity representing whole-home power consumption/generation |
| `title` | No | Card title. Also used as the label for the home entity (defaults to `Home` if not set) |
| `sleep_timeout` | No | Seconds before returning to the screensaver (default: `10`) |
| `entities` | No | Up to 6 power sensor entities displayed around the home entity |

Each entry in `entities` supports `entity_id`, `name`, and `icon`. `icon` is either an icon name or a mapping with `value` and `color`, as in the example below.

> **Note:** Unused slots around the home entity are padded automatically. Explicit `entity_id: delete` placeholders are also supported and render with the correct 7-field `cardPower` layout; automatic padding is still the simpler choice.

## Colours

Icons switch between two colours to indicate power flow state:

- **Active**: `|value| ≥ 1` in the entity's reported unit (≥ 1 W for a W sensor, ≥ 1 kW for a kW sensor) — the configured `icon: color`, or HA blue (`17299`) by default
- **Inactive** (zero, non-numeric, unknown or unavailable): grey (`#808080`, `33808`)

Set `icon: color` per entity for Tesla-style colour coding — yellow for solar, green for EV, red for grid import, and so on. See the [RGB565 colour picker](https://chrishewett.com/blog/true-rgb565-colour-picker/) for values.

## Units and animation speed

Both the home entity and the surrounding entities handle `W` and `kW` automatically:

- **W**: values ≥ 1000 W display as kW (e.g. `1.2kW`); smaller values as whole watts (e.g. `450W`)
- **kW**: displayed with two decimal places (e.g. `2.35kW`)
- **Other / no unit**: the raw state value is shown as-is

The animated speed indicator on each surrounding entity scales with the power value:

- `W` unit: `speed = watts / 20`, clamped to `[-120, 120]`
- `kW` unit: `speed = kilowatts × 50`, clamped to the same range
- Negative values animate in the reverse direction (battery discharge or export)

Entities reporting `unknown` or `unavailable` show a `-` and a stopped animation.

## Example

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

[← Back to README](../README.md#feature-reference)
