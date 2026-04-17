# Decouple loader smoke test from vendored DLL path

- Status: done
- Scope: remove the `xl-driver-sys` smoke test's hard dependency on `references/xl-driver/vxlapi64.dll` while preserving explicit runtime verification behavior
- Links: `AGENTS.md`, `docs/process/development-guide.md`, `crates/xl-driver-sys/src/loader.rs`

## Steps

- [x] add the task record and keep it current
- [x] refactor the ignored smoke test to use an explicit DLL path override or the installed default DLL name
- [x] run targeted verification and record the exact outcome

## Verification

- Ran `cargo fmt --all -- --check`.
- Ran `cargo test -p xl-driver-sys --all-targets`.
- Ran `cargo test -p xl-driver-sys smoke_loads_configured_or_default_dll_and_opens_driver -- --ignored`.
- Ran `$env:XL_DRIVER_SYS_TEST_DLL=(Resolve-Path .\references\xl-driver\vxlapi64.dll).Path; cargo test -p xl-driver-sys smoke_loads_configured_or_default_dll_and_opens_driver -- --ignored`.

## Outcome

- Removed the smoke test's hardcoded dependency on `references/xl-driver/vxlapi64.dll`.
- The ignored smoke test now uses `XL_DRIVER_SYS_TEST_DLL` when provided, otherwise it tries the default runtime DLL name `vxlapi64.dll`.
- In this shell environment, bare `vxlapi64.dll` lookup failed with `LoadLibraryExW failed`, which confirms the test no longer relies on the repo-local reference path implicitly.
- The explicit override path worked and still verified `xlOpenDriver` / `xlCloseDriver` successfully.
