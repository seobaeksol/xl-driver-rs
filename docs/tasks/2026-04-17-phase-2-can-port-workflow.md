# Implement Phase 2 CAN port workflow

- Status: done
- Scope: add the first usable classic CAN workflow by opening a CAN port, configuring bitrate, activating/deactivating channels, wiring notification-based receive behavior, and supporting classic CAN transmit/receive
- Links: `AGENTS.md`, `docs/architecture/porting-plan.md`, `docs/progress.md`, `references/xl-driver/vxlapi.h`, `references/xl-driver/samples/xlCANdemo/xlCANdemo.c`

## Steps

- [x] add the Phase 2 task record and keep it current
- [x] extend `xl-driver-sys` with the raw CAN event types, constants, and function bindings needed for classic CAN
- [x] implement the safe CAN port workflow in `xl-driver`
- [x] run proportional verification and update the progress dashboard

## Verification

- Ran `cargo fmt --all`.
- Ran `cargo check --workspace --all-targets`.
- Ran `cargo clippy --workspace --all-targets -- -D warnings`.
- Ran `cargo test --workspace --all-targets`.
- Ran `$env:XL_DRIVER_SYS_TEST_DLL=(Resolve-Path .\references\xl-driver\vxlapi64.dll).Path; cargo test -p xl-driver-sys smoke_loads_configured_or_default_dll_and_opens_driver -- --ignored`.

## Outcome

- Added the raw classic CAN bindings needed for Phase 2, including `XLevent`, classic CAN message layout, CAN output-mode constants, and the `xlCanSetChannelBitrate`, `xlCanSetChannelOutput`, `xlCanTransmit`, `xlReceive`, and `xlGetEventString` entrypoints.
- Added `Driver::open_can_port(...)` and a safe `Port` workflow for classic CAN port ownership, bitrate setup, activation/deactivation, notification setup, queue inspection, classic CAN transmit, and basic event receive.
- Added unit and layout tests for the new access-mask logic, classic CAN event conversion, queue sizing, and classic CAN raw layouts.
- Confirmed that the expanded loader still resolves successfully and opens/closes the XL driver when an explicit DLL path is provided.
- This implementation slice initially left Phase 2 in progress; the follow-up runtime validation that closed Phase 2 is recorded in `docs/tasks/2026-04-17-phase-2-can-validation.md`.
