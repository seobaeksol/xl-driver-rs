# Rust Coding Style

## Purpose

This document defines the coding style expected in this repository so that different contributors produce similar code quality and structure.

It is intentionally opinionated in areas that matter for FFI, public API design, and reviewability.

## Formatting and Baseline Conventions

- use default `rustfmt`
- prefer stable Rust
- keep files ASCII unless there is a clear reason not to
- prefer one responsibility per module
- keep function names descriptive rather than abbreviated
- keep public APIs conservative and intentional

Do not hand-format code against `rustfmt` output unless there is a very strong readability reason.

## Crate Boundaries

The repository should distinguish between two layers.

## `xl-driver-sys`

This crate is responsible for:

- FFI types
- FFI constants
- layout-sensitive structs and unions
- DLL loading
- raw function pointers and raw calls

This crate should not:

- expose ergonomic high-level workflows
- hide important driver behavior behind convenience logic
- own business-level retry or fallback policy

## `xl-driver`

This crate is responsible for:

- safe wrappers
- ownership and lifetime management
- Rust-native types
- typed errors
- user-facing docs and examples

This crate should not:

- duplicate raw ABI declarations
- reinterpret raw memory layouts ad hoc
- expose raw handles as the default public abstraction

## Naming

### Modules

- use `snake_case`
- organize by responsibility, not by file size alone
- avoid generic names like `utils.rs` unless the file is truly small and local in scope

### Types

- use `UpperCamelCase` for public Rust types
- use exact C/XL names only in raw FFI types where fidelity matters
- if a raw type is exposed only internally, prefer a clearly prefixed or namespaced location instead of renaming it casually

### Functions

- public safe APIs should describe intent, not transport details
- raw FFI items should keep the original XL names

Examples:

- safe API: `open_can_port`, `recv_timeout`, `set_bitrate`
- raw API: `xlOpenDriver`, `xlCanTransmitEx`

## Type Mapping Rules

Use explicit-width Rust types for C ABI mappings.

Rules:

- never map C integer types by guesswork
- never use `usize`/`isize` where the header defines a fixed-width or platform-specific type
- document unusual mappings in code comments near the type alias

Examples from `vxlapi.h`:

- `XLstatus -> i16`
- `XLaccess -> u64`
- `XLportHandle -> i32`

## FFI Rules

### Calling Convention

- use `extern "system"` for XL API function signatures on Windows
- do not mix calling conventions inside the same raw surface

### Layout

- use `#[repr(C)]` for C-compatible structs
- use `#[repr(transparent)]` only when the semantics truly fit
- keep unions and packing-sensitive definitions local to the raw layer
- validate layout for high-risk types

### Raw Strings

- copy borrowed C strings into owned Rust strings before exposing them publicly
- do not expose borrowed pointers from the driver as stable public references

## Unsafe Code Rules

`unsafe` is allowed only when required by ABI, loading, pointer handling, or validated invariants.

Rules:

- keep each `unsafe` block as small as possible
- add a short `// SAFETY:` comment for non-trivial `unsafe`
- do not wrap large control flows in one `unsafe` block
- move validation outside the `unsafe` block whenever possible

Preferred pattern:

```rust
// SAFETY: `self.raw.xlOpenDriver` is loaded from the expected DLL and has the
// correct calling convention and signature defined by `vxlapi.h`.
let status = unsafe { (self.raw.xlOpenDriver)() };
```

## Error Handling

This is a library project. Panics should be exceptional.

Rules:

- use `Result` for normal failure paths
- preserve raw XL status codes in error types
- include human-readable messages when available
- do not silently downgrade or reinterpret driver failures unless the behavior is explicitly documented

Do not:

- panic because a runtime dependency is missing
- return vague errors when the driver already provided a precise status

## Public API Design

### General

- prefer typed methods over raw parameter forwarding
- expose safe defaults, but do not hide important XL semantics
- prefer ownership-driven cleanup
- avoid APIs that require callers to understand internal ABI details

### Handles

- raw handles should stay internal to wrapper types
- if a raw handle must be exposed, do so intentionally and document the contract

### Defaults

- default values are acceptable only when they are stable, documented, and not surprising
- do not guess bus-specific behavior in a way that can silently misconfigure hardware

## Comments and Documentation

Rules:

- add comments only where the code is not self-evident
- use comments to explain why, not restate what
- write Rustdoc for public types and public methods
- document all public APIs that have important runtime constraints

Every public API should answer at least one of these:

- what resource does it own
- what preconditions does it require
- what error modes are typical
- what cleanup behavior does it guarantee

## Testing Style

Tests should match the risk level of the code.

### Required for raw FFI changes

- layout or size assertions for affected types
- loader tests or smoke tests when practical

### Required for wrapper changes

- unit tests for conversions, validation, and error mapping
- integration tests when behavior spans multiple layers

### Hardware-dependent behavior

- if hardware tests cannot run in the current environment, say so explicitly
- do not fake hardware verification

## Dependency Policy

Prefer small, well-understood dependencies.

Rules:

- add a dependency only when it materially simplifies the code or reduces risk
- avoid stacking convenience crates inside the raw layer
- justify new dependencies that affect FFI, error handling, async runtime, or memory layout

Likely acceptable examples:

- `windows-sys`
- `libloading`

## Consistency Expectations

A contributor should be able to open a file and predict:

- where `unsafe` lives
- how errors are represented
- which layer owns which concern
- how runtime resources are cleaned up

If a change introduces a new pattern, document why the old pattern was not sufficient.
