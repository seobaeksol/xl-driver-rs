# Validate Phase 2 classic CAN runtime path

- Status: done
- Scope: close the remaining Phase 2 gap by validating the classic CAN workflow in a credible runtime environment and tightening the safe port-opening path based on that result
- Links: `AGENTS.md`, `docs/architecture/porting-plan.md`, `docs/progress.md`, `references/xl-driver/vxlapi.h`, `references/xl-driver/samples/xlCANdemo/xlCANdemo.c`

## Steps

- [x] create the task record and keep it current
- [x] add a safe channel discovery surface so classic CAN ports can be opened from actual driver metadata
- [x] change the safe CAN port-opening path to use `xlCreatePort` + `xlAddChannelToPort` + `xlFinalizePort`
- [x] add runnable and testable validation entrypoints for classic CAN
- [x] run runtime validation in a credible CAN environment and update the dashboard

## Verification

- Ran `cargo fmt --all -- --check`.
- Ran `cargo check --workspace --all-targets`.
- Ran `cargo clippy --workspace --all-targets -- -D warnings`.
- Ran `cargo test --workspace --all-targets`.
- Ran `$env:XL_DRIVER_SYS_TEST_DLL=(Resolve-Path .\references\xl-driver\vxlapi64.dll).Path; cargo test -p xl-driver-sys smoke_loads_configured_or_default_dll_and_opens_driver -- --ignored`.
- Ran `$env:XL_DRIVER_SYS_TEST_DLL=(Resolve-Path .\references\xl-driver\vxlapi64.dll).Path; cargo run -p xl-driver --example list_channels`.
- Ran `$env:XL_DRIVER_SYS_TEST_DLL=(Resolve-Path .\references\xl-driver\vxlapi64.dll).Path; $env:XL_DRIVER_CAN_TEST_CHANNEL='19'; cargo test -p xl-driver classic_can_port_can_activate_transmit_and_observe_events --test classic_can_hardware -- --ignored --nocapture`.

## Outcome

- Added `Driver::channels()` and `Driver::can_channels()` so callers can discover real XL channels before opening ports.
- Tightened classic CAN port opening to request init access through `xlCreatePort`, `xlAddChannelToPort`, and `xlFinalizePort`, which matches the vendor CAN sample flow and avoids depending on preconfigured app-channel ownership.
- Added runnable examples under `crates/xl-driver/examples/` for channel listing and classic CAN smoke validation.
- Added an ignored hardware validation test for the end-to-end classic CAN path.
- Confirmed Phase 2 runtime behavior in this environment by exercising the classic CAN path on Vector virtual CAN channel `19`, including port open, notification setup, activation, transmit, and event observation.
