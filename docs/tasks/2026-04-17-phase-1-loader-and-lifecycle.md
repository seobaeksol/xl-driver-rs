# Implement Phase 1 loader and lifecycle foundation

- Status: done
- Scope: implement the Phase 1 `xl-driver-sys` dynamic loader and common lifecycle raw bindings, then expose a minimal `Driver::open()` safe wrapper with proportional verification
- Links: `AGENTS.md`, `docs/architecture/porting-plan.md`, `docs/process/development-guide.md`, `references/xl-driver/vxlapi.h`

## Steps

- [x] add the Phase 1 task record and keep it current
- [x] implement the dynamic loader and common lifecycle symbols in `xl-driver-sys`
- [x] add minimal safe driver open/close behavior in `xl-driver`
- [x] run formatting, linting, tests, and smoke verification where possible

## Verification

- Ran `cargo fmt --all`.
- Ran `cargo check --workspace --all-targets`.
- Ran `cargo clippy --workspace --all-targets -- -D warnings`.
- Ran `cargo test --workspace --all-targets`.
- Ran `cargo test -p xl-driver-sys smoke_loads_reference_dll_and_opens_driver -- --ignored`.

## Outcome

- Added dynamic loading for the Phase 1 lifecycle symbol set in `xl-driver-sys` using `libloading`.
- Added raw ABI validation for core widths plus `XLcanFdConf`, `XLchannelConfig`, and `XLdriverConfig` layout-sensitive types.
- Added a minimal safe `Driver::open()` / `Driver::open_with_location()` API in `xl-driver` with RAII cleanup and clearer loader/runtime error reporting.
- Confirmed that the repository's reference `vxlapi64.dll` loads successfully and that `xlOpenDriver` / `xlCloseDriver` succeed in this environment.
