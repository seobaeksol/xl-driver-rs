# Recommended Rust Porting Approach for `xl-driver`

Date: 2026-04-17

## 1. Purpose of This Document

This document summarizes the **recommended implementation approach** for making `xl-driver` usable from Rust, based on the `vxlapi.h`, `vxlapi64.dll`, vendor manual, and sample code included in `references/xl-driver`.

Here, "porting" does not mean reimplementing the Vector XL Driver itself. It means **building Rust FFI bindings and a safe wrapper on top of the existing `vxlapi64.dll`**.

## 2. Final Recommendation

The recommended approach is as follows:

- Build two Rust crates that wrap `vxlapi64.dll`.
- Limit the MVP scope to `Windows x64 + CAN + CAN FD`.
- Use **dynamic loading** as the default loading model, not static linking.
- Separate raw FFI from the ergonomic public API.
- Exclude Ethernet from the MVP, and prefer the **network-based API** when Ethernet support is added later.

In other words, this should not be treated as a project to translate the entire XL API into Rust at once. It should be treated as a project to **first deliver a production-usable Rust wrapper centered on CAN and CAN FD, and then expand bus-by-bus later**.

## 3. Implementation Assumptions

### Supported Platform

- Operating system: Windows 10/11 64-bit
- Rust target: `x86_64-pc-windows-msvc`
- Default DLL: `vxlapi64.dll`

Currently available references:

- `references/xl-driver/vxlapi.h`
- `references/xl-driver/vxlapi64.dll`
- `references/xl-driver/XL Driver Library - Description.pdf`
- `references/xl-driver/samples/*`

### ABI Based on the Header

The following ABI details were confirmed from `vxlapi.h`:

- Calling convention: `_XL_EXPORT_API == __stdcall`
- Rust equivalent: `extern "system"`
- `XLstatus = short`
- `XLaccess = unsigned __int64`
- `XLportHandle = long`
- `XLhandle = HANDLE`
- `XLstringType = char*`
- `XL_INTERFACE_VERSION = XL_INTERFACE_VERSION_V3`
- `XL_INTERFACE_VERSION_V4` exists separately

Recommended Rust mappings:

```rust
pub type XLstatus = i16;
pub type XLaccess = u64;
pub type XLportHandle = i32;
pub type XLhandle = windows_sys::Win32::Foundation::HANDLE;
```

The most important point is `XLportHandle`. Even on Windows x64, C `long` is still 32-bit, so it must not be mapped to `isize` or `i64` in Rust.

### Dynamic Loading Assumption

`vxlapi.h` officially supports a dynamic loading usage pattern through the `DYNAMIC_XLDRIVER_DLL` macro. As a result, it is natural to design the Rust side around the equivalent of `LoadLibrary` and `GetProcAddress`, and the vendor samples (`xlLoadlib.cpp`, `xlEthBypassDemo.cpp`) follow the same pattern.

This recommended approach follows that model as well.

## 4. Recommended Architecture

### Crate Structure

The recommended structure has two layers.

#### `xl-driver-sys`

Role:

- raw FFI
- DLL loader
- minimal C type, constant, and struct definitions
- concentrated `unsafe` boundary

Responsibilities:

- load `vxlapi64.dll`
- resolve required symbols
- perform raw function calls
- preserve ABI and layout correctness

#### `xl-driver`

Role:

- Rust-style safe public API
- RAII-based lifetime management
- unified error handling
- bus-specific wrappers

Responsibilities:

- `Result`-based error handling
- model channels, ports, and activation state
- provide CAN and CAN FD frame and event wrappers
- hide XL API details such as queue size rules and interface version handling

### Example Directory Layout

```text
crates/
  xl-driver-sys/
    src/
      lib.rs
      loader.rs
      types.rs
      can.rs
      canfd.rs
  xl-driver/
    src/
      lib.rs
      error.rs
      driver.rs
      port.rs
      can.rs
      canfd.rs
```

## 5. MVP Scope

### Included Scope

The MVP release should support only the following:

- common driver and port lifecycle
- CAN
- CAN FD

Functions included in scope:

- Common
  - `xlOpenDriver`
  - `xlCloseDriver`
  - `xlGetDriverConfig`
  - `xlGetApplConfig`
  - `xlSetApplConfig`
  - `xlGetChannelIndex`
  - `xlGetChannelMask`
  - `xlOpenPort`
  - `xlCreatePort`
  - `xlAddChannelToPort`
  - `xlFinalizePort`
  - `xlSetNotification`
  - `xlFlushReceiveQueue`
  - `xlGetReceiveQueueLevel`
  - `xlActivateChannel`
  - `xlDeactivateChannel`
  - `xlClosePort`
  - `xlGetErrorString`
- CAN
  - `xlCanSetChannelBitrate`
  - `xlCanSetChannelOutput`
  - `xlCanTransmit`
  - `xlReceive`
- CAN FD
  - `xlCanFdSetConfiguration`
  - `xlCanTransmitEx`
  - `xlCanReceive`
  - `xlCanGetEventString`

### Excluded Scope

The MVP should not implement the following:

- LIN
- Ethernet
- FlexRay
- MOST/MOST150
- DAIO
- A429
- advanced TimeSync features

The reason is straightforward: common lifecycle support plus CAN and CAN FD already provides meaningful value, and it is enough to validate the ABI, layout, and loader architecture.

### Selective Expansion Principles After MVP

Completing the MVP does not mean the remaining buses should be implemented one by one automatically. Expansion should proceed **selectively**, based on the criteria below.

Primary evaluation criteria:

- can the current common layer be reused as-is
- is the additional ABI and event-structure complexity manageable
- is there real hardware or a credible validation environment available
- does Vector documentation recommend a non-legacy API path for that bus

Approval criteria for expansion:

- the MVP API surface is already stable
- the new bus can be added without breaking the common loader, error, or port abstractions
- the required structs and event layouts for the new bus can be validated
- a minimal smoke test and a basic send/receive or status-query test can be defined for that bus

Reasons to defer expansion:

- there is no hardware available for runtime validation
- the documentation and header exist, but the event structures are too large or too complex
- only a legacy API path exists and it is not the recommended path for new projects
- the priority is low from a general library perspective rather than tied to a specific real need

### Recommended Expansion Order

After the MVP, the default recommended order is:

1. LIN
2. Ethernet network-based
3. basic TimeSync features
4. FlexRay
5. DAIO / A429 / MOST-family buses

This order is recommended for the following reasons:

- LIN reuses much of the common lifecycle while having clear bus-specific constraints, which makes it good for strengthening the wrapper design.
- Ethernet may be valuable, but it should prefer the network-based API over the channel-based API, so it needs an additional design pass.
- FlexRay, MOST, and A429 have much broader and more complex APIs and event models, so they are heavy targets immediately after the MVP.

## 6. Implementation Strategy

### 6.1 Dynamic Loading Only

The initial version should support dynamic loading only.

Reasons:

- it is the usage model officially supported by the vendor
- DLL path issues can be controlled at runtime
- there is no need to depend on import libraries for static linking
- it reflects the actual Vector Driver installation state in the user environment

Recommended policy:

- default DLL name: `vxlapi64.dll`
- optionally support explicit DLL path override
- provide clear error messages when loading fails

### 6.2 Write Raw FFI Manually

For the initial MVP, the required function signatures should be written **manually**.

Reasons:

- the number of required functions is limited
- the core ABI-sensitive types can be controlled directly
- there is no need to depend on a large generated `bindgen` output
- sensitive ABI details such as `XLportHandle`, `HANDLE`, packing, and unions can be reviewed directly

### 6.3 Use `bindgen` as a Supporting Tool

`bindgen` should be used only for:

- checking struct layouts
- checking for missing constants or enums
- generating allowlist-based output when expanding scope later

In other words, `bindgen` should be treated as a **header verification tool**, not the foundation of the initial public API.

### 6.4 Source of Truth

The canonical header should be fixed to:

- `references/xl-driver/vxlapi.h`

The DLL version and header version must be managed together. If the header changes, raw type mappings and layout validation must be rerun.

## 7. Rust API Design

### 7.1 Public Types

The public API should not expose raw types directly. It should wrap them in meaningful Rust-level abstractions.

Expected types:

- `Driver`
- `ChannelInfo`
- `Port`
- `CanPort`
- `CanFdPort`
- `CanFrame`
- `CanFdFrame`
- `XlError`

### 7.2 Error Model

Errors should preserve `XLstatus` internally while exposing a Rust-style error type to users.

Example:

```rust
pub struct XlError {
    pub code: i16,
    pub message: String,
}
```

Recommended rules:

- every public API returns `Result<T, XlError>`
- generate error messages with `xlGetErrorString()` when possible
- preserve the raw status code for debugging

### 7.3 Lifetime Management

Resources should be closed via RAII.

- `Driver::open()` -> `xlOpenDriver`
- `Drop for Driver` -> `xlCloseDriver`
- `Port::open_*()` -> `xlOpenPort` or the `xlCreatePort` family
- `Drop for Port` -> `xlClosePort`
- activation state should be modeled via `activate()`/`deactivate()` or separated types

For the initial implementation, a simpler `Port` API is more realistic than a full type-state design.

### 7.4 Notification Model

The XL API uses `xlSetNotification()` and Win32 wait handles. For the MVP, the appropriate model is:

- provide a blocking receive API
- use `WaitForSingleObject` internally
- leave async runtime integration for later

In other words, the first version should be designed around `recv_timeout()`.

### 7.5 Hide Queue Size Differences

Queue size is interpreted either as an event count or as a byte size depending on the bus and interface version. That distinction should be hidden in the public API.

Example:

```rust
pub enum CanQueueSize {
    Events(u32),
}

pub enum CanFdQueueSize {
    Bytes(u32),
}
```

Or, even more simply, the library may provide fixed per-bus defaults.

## 8. MVP Public API Shape

The initial user experience only needs to be roughly this level:

```rust
let driver = Driver::open()?;

let port = driver.open_can_port(
    "xl-driver-rs",
    &[channel_index],
    500_000,
)?;

port.activate()?;
port.set_notification()?;
port.transmit(frame)?;

while let Some(event) = port.recv_timeout(Duration::from_millis(100))? {
    println!("{event:?}");
}
```

CAN FD should branch through a separate port type or a separate open function.

```rust
let port = driver.open_canfd_port("xl-driver-rs", &[channel_index], canfd_config)?;
```

## 9. Verification Strategy

### 9.1 Compile-Time Validation

The following validations should be included:

- `size_of::<XLportHandle>()`
- `size_of::<XLaccess>()`
- `size_of::<XLcanFdConf>()`
- `size_of::<XLevent>()`
- `size_of::<XLcanRxEvent>()`
- field offset validation via `offset_of!` where needed

ABI mismatches should be caught at this stage whenever possible.

### 9.2 Runtime Smoke Tests

Minimum smoke test:

- DLL loads successfully
- required symbols resolve successfully
- `xlOpenDriver` and `xlCloseDriver` succeed

### 9.3 Functional Tests

Recommended test order:

1. driver open/close
2. channel enumeration
3. CAN open/activate/deactivate
4. CAN transmit/receive
5. CAN FD config/transmit/receive

If possible, run loopback or real bus send/receive tests on actual Vector hardware.

## 10. Step-by-Step Roadmap

### Phase 0. Foundation

Tasks:

- add the crate skeleton to the workspace
- configure builds for `x86_64-pc-windows-msvc`
- freeze the local `vxlapi.h` and DLL version

Completion criteria:

- the empty crates build successfully on Windows x64

### Phase 1. `xl-driver-sys`

Tasks:

- define core types
- implement the DLL loader
- add raw bindings for common lifecycle APIs

Completion criteria:

- DLL loading and `xlOpenDriver`/`xlCloseDriver` smoke tests pass

### Phase 2. CAN

Tasks:

- configure CAN bitrate
- transmit CAN frames
- receive events through `xlReceive`
- integrate `xlSetNotification`

Completion criteria:

- CAN send/receive works on real hardware or loopback

### Phase 3. CAN FD

Tasks:

- map `XLcanFdConf`
- add `xlCanFdSetConfiguration`
- add `xlCanTransmitEx`
- add `xlCanReceive`

Completion criteria:

- CAN FD send/receive works

### Phase 4. Expansion

Priority:

1. LIN
2. Ethernet network-based
3. other buses

Phase 4 is not automatic. Only choose the next bus when the following conditions are met:

- there is a clear real need
- hardware or a validation environment is available
- required changes to the common layer are local and contained
- the minimum useful feature slice can be clearly defined from the header and samples

## 11. Key Risks and Mitigations

| Risk | Impact | Mitigation |
| --- | --- | --- |
| misunderstanding the width of `XLportHandle` | runtime misbehavior | fix it to `i32` and validate at compile time |
| confusing `HANDLE` with `XLportHandle` | notification/port handling errors | keep them as distinct Rust types |
| missing `__stdcall` | calling convention mismatch | declare all raw XL function pointers as `extern "system"` |
| struct packing mismatch | event decoding failure | use `#[repr(C)]` and layout validation |
| misunderstanding queue size units | port open failure or receive errors | hide unit differences in bus-specific wrappers |
| misunderstanding init access rules | bus-specific failures | reflect bus constraints in the wrapper design |
| DLL path issues | initialization failure | support default path, override, and clear diagnostics |
| trying to implement the entire API at once | schedule failure | forbid new bus work before MVP completion, and require explicit approval per bus afterward |

## 12. Final Recommendations

The execution order should remain fixed as follows:

1. build `xl-driver-sys` first
2. support dynamic loading only
3. implement the MVP as common lifecycle + CAN + CAN FD
4. write raw FFI manually
5. use `bindgen` only for validation and later expansion
6. provide the safe public API in `xl-driver`
7. expand later based on bus-specific demand and validation feasibility
8. when Ethernet is added, prefer the network-based API over the channel-based API

This approach is implementable immediately with the references currently available in the repository, and it keeps scope under control.

## 13. References

### Local References

- `references/xl-driver/vxlapi.h`
- `references/xl-driver/vxlapi64.dll`
- `references/xl-driver/XL Driver Library - Description.pdf`
- `references/xl-driver/samples/xlCANdemo/xlCANdemo.c`
- `references/xl-driver/samples/xlCANcontrol/xlLoadlib.cpp`
- `references/xl-driver/samples/xlEthBypassDemo/xlEthBypassDemo.cpp`

### External References

- `pyvxlapi`: <https://github.com/mikisama/pyvxlapi>
- `automotive` crate docs: <https://docs.rs/automotive/latest/automotive/>
- `libloading` docs: <https://docs.rs/crate/libloading/latest>
- `windows-sys` docs: <https://docs.rs/crate/windows-sys/latest/features>
