# Repository Agent Instructions

## Upstream synchronization ledger

This is the PhiKapJames fork of `olicooper/esphome-nspanel-lovelace-native`. Fork base is upstream `d12cf3a`; floor is ESPHome 2026.11.0, ESP-IDF only. Approved plans and execution evidence live in the Obsidian vault under `Home-Automation/Repositories/esphome-nspanel-lovelace-native/`.

**Check this ledger before porting anything from upstream.** Already-reviewed PRs do not need a fresh divergence analysis unless upstream changed them. Still re-read the current diff and verify versions before acting — this is a record, not a substitute for inspection.

| Upstream | Verdict | Applied |
|---|---|---|
| #69 Slovak `sk.json` | Bring back — verbatim copy; 106 keys, same key set as `cs.json` | PR #8 |
| #70 screensaver2 / custom screensaver entities / `precision` | **Defer** — ~7-file feature, needs rewrite into the fork's `CodegenContext`; overlaps stage work | — |
| #71 hex / `[r,g,b]` icon colors + screensaver weather `icon_color` | Bring back Python schema/validator + `icon_color_overridden_` guard. Example YAML edits skipped: `examples/` belongs to the separate example-encryption plan | PR #8 |
| #72 color-wheel hue lower half | **Skip** — already present in `helpers.h` | — |
| #73 short day/month name ordering (`89d6213`) | Bring back **in full** (day and month blocks; Monday-only leaves Jan/Feb broken) | PR #8 |
| #74/#76/#78/#79/#83/#84 (⊇#81⊇#80), #82, #77 | Already integrated in fork `dev`, usually hardened; no further work | `05996b6` |
| `5ee8326` cardPower updates | Partial: DeleteItem 6 separators, `PageItem_Value`, prefix units. `cards.cpp` render optimisation deliberately excluded | PR #8 |
| `ef1f706` version floor | **Skip** — fork floor is 2026.11.0 | — |
| `dbf4679` `precision` key | **Defer** with #70 | — |
| `5b013c6` secrets `api_key` | Covered by the example-encryption plan | — |

## Deliberate divergences — preserve when porting

- **Codegen:** the fork uses `CodegenContext` (`ctx.entity()`, `ctx.icon()`, `collect_entity_ids()`) and `valid_entity_id(flags...)`, not upstream's module-level `get_entity_id()` / positional allowlist. Re-express upstream codegen hunks; do not paste them.
- **Parser:** keep `str_to_float()` in `card_items.cpp` / `cards.cpp`. Upstream's raw `std::strtof` was rejected.
- **Host harness:** `tests/host/run.sh` compiles at `-std=gnu++17` while ESPHome uses C++20. Avoid `std::string::starts_with`; use `rfind(s, 0) == 0`. Always run the host suite — it has caught both prior port compile errors.
- **`DeleteItem`:** constructor parameter is named `type` to match upstream `5ee8326`. Do not rename it back to `page_type`.
- **2026.11 floor:** no `ESPHOME_VERSION_CODE` guards, no Arduino path, unconditional `alloc_helpers.h` include. Do not reintroduce version fallbacks.
- **`cards.cpp` PowerCard render:** upstream's padding removal / blanked fields / dropped speed field is intentionally not ported. Its on-wire field contract cannot be validated without a panel.
- **`PowerCard::update_home_value_`** exact-matches `W`/`kW` while tiles prefix-match; upstream does the same. Not a divergence — change only after a panel check.

## Validation boundary

`tests/host/run.sh`, pytest in the 2026.11.0 image, and `esphome config` / `compile` for `ci/basic.yaml` and `ci/advanced.yaml` prove code correctness only. TFT field layout, PSRAM allocation, upload baud/timing and HMI behaviour need a real panel. Transfer, build, OTA and flashing each require separate owner approval.
