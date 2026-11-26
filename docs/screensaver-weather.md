# Screensaver weather

Configure one Home Assistant weather entity. The component calls `weather.get_forecasts` with `type: daily` and renders the earliest four upcoming days in the screensaver. Current conditions (state, temperature and unit) continue to come from the same entity.

```yaml
screensaver:
  time_id: homeassistant_time
  weather:
    entity_id: weather.home
```

The provider must support daily forecasts. If the reply is unsupported or fails, current conditions stay intact and the forecast slots are blanked with a rate-limited diagnostic. No Home Assistant automation, template sensor, or provider credential is needed: the panel uses its normal API action permission to call `weather.get_forecasts`.

Screensaver weather icons use their condition's default colour. To pin them to one colour, set `screensaver.weather.icon_color`; it applies to the main icon and all four forecast icons. See [Icons and colours](../README.md#icons) for the accepted formats.

## Known limitation: lost forecast replies

On **ESPHome 2026.11.0 and 2026.11.1**, four forecast requests that never receive a reply can stop forecast updates until the panel restarts. The losses do not have to be consecutive. A panel-side timeout or a Home Assistant reconnect does not remove those pending requests from ESPHome. The four-request cap prevents them from accumulating indefinitely. This does not affect the current-conditions subscription or require extra Home Assistant setup.

## Optional restart workaround

To opt in, set a delay under `weather`:

```yaml
screensaver:
  time_id: homeassistant_time
  weather:
    entity_id: weather.home
    restart_after_forecast_stall: 30min
```

Omit the option or use `0s` to keep the existing polling behaviour. When opted in, the panel can try the remaining free slot after the existing **30-minute backoff** without waiting for a reconnect. A timely, valid reply restores forecasts without restarting. The restart delay starts only when a fetch fills the fourth slot. Any processed success or error that frees a slot, even a late reply, cancels it.

After the delay, a restart still requires four occupied slots, no active forecast response window, no TFT upload or reparse, and at least **30 seconds** of continuously observed Home Assistant state-subscription readiness. An observed disconnect restarts that 30-second window, not the stall countdown.

On ESPHome 2026.11.0 and 2026.11.1 a closing connection can still look ready, and a reply can be waiting to be processed, so an unnecessary restart is possible. Once a restart is committed, the panel rejects new TFT uploads. The panel is briefly unavailable during the restart, and a persistent fault can cause another restart after a fresh stall on each boot. This is a workaround, not a fix.

## ESPHome upgrade check

When evaluating an ESPHome upgrade, check the candidate version's release notes and public API or source for supported cancellation, expiry, or disconnect cleanup of **outbound Home Assistant action-response callbacks**. On ESPHome 2026.11.0 and 2026.11.1, a panel-side timeout or reconnect does not clear those callbacks.

Keep direct `weather.*` retrieval without requiring a Home Assistant template sensor or automation. Until supported cleanup is verified, retain the four-slot cap: do not reuse an occupied slot or allow unbounded callback growth. Automatic panel restarts are permitted only through the owner's opt-in `restart_after_forecast_stall` workaround, not as a fix. An inbound action timeout or a guard that only ignores stale replies does not free outbound callbacks.

If the candidate offers supported cleanup, verify in an isolated lab that a new forecast fetch succeeds after four deliberately lost replies without a panel restart, that late replies cannot overwrite newer data, and that pending callback storage stays bounded over repeated loss and reconnect cycles. Record the exact version and evidence before changing the slot policy. If no supported solution is found, record the version checked and leave the follow-up open.

## Migration from older releases

- Remove `forecast_method`.
- Replace any `sensor.*` weather entity with `weather.*`.
- Disable the old forecast-push automation only after the upgraded panel has been verified.

[← Back to README](../README.md#known-issues)
