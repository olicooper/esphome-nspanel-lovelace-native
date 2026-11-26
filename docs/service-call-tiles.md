# Arbitrary service-call tiles

A tile with an `entity_id` in the `service.*` domain can call any Home Assistant service when tapped:

```yaml
- entity_id: service.power_off
  name: Standby
  service: remote.send_command
  data:
    entity_id: remote.example_remote
    command: [standby]
```

Give each distinct action its own `service.*` tile id. A worked tile is in the [advanced example](../examples/advanced-example.yaml).

## Configuration rules

| Key | Rule |
|-----|------|
| `entity_id` | Must use the `service.*` domain |
| `service` | Required on a service tile. Lowercase `domain.service_name` matching `[a-z_][a-z0-9_]*\.[a-z_][a-z0-9_]*` (letters, digits and underscores only) |
| `data` | Required on a service tile; must be a mapping with a non-empty string `entity_id` |

`service` and `data` travel together: `service` is invalid on a non-`service.*` tile, and `data` is invalid without `service`. Invalid values, missing `data.entity_id`, non-string keys, non-finite numbers, and `null` values are rejected at `esphome config` time. A `service.*` tile with neither `service` nor `data` passes validation and does nothing when tapped.

## String and non-string data

String values in `data` are sent to Home Assistant unchanged. Non-string values (lists, numbers, booleans, and nested mappings) are sent through HA's `data_template` as Python literals, so they arrive at the service with their original type — for example `command: [standby]` arrives as a one-item list.

This depends on the Home Assistant version parsing rendered template results. Builds without that parsing deliver the literal text instead of the original value. If a list-valued service argument matters to your setup, confirm it against your Home Assistant version before relying on it.

[← Back to README](../README.md#feature-reference)
