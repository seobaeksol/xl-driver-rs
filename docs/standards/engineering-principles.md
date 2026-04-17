# Engineering Principles

## Purpose

This project exists to provide a reliable Rust interface over the Vector XL Driver, not to reimplement the driver itself.

The project should optimize for:

- ABI correctness
- predictable behavior
- small and reviewable changes
- explicit scope control
- maintainable public APIs

## Project Position

The current project direction is:

- platform: Windows x64
- driver access: dynamic loading of `vxlapi64.dll`
- architecture: raw FFI crate plus safe wrapper crate
- MVP scope: common lifecycle + CAN + CAN FD

Anything outside that scope is opt-in future work, not an implicit obligation.

## Core Principles

### 1. Correctness before coverage

Do not add broad API coverage at the cost of uncertain ABI handling.

A smaller surface that is correct is preferable to a larger surface that is only partially verified.

### 2. Unsafe must be contained

`unsafe` is necessary for FFI, but it must be narrow, explicit, and reviewable.

Rules:

- keep most `unsafe` code inside the raw FFI layer
- document the safety assumptions for each non-trivial `unsafe` block
- do not leak raw handles, raw pointers, or layout-sensitive details into the ergonomic public API unless there is no practical alternative

### 3. Explicit constraints are better than implied portability

Do not pretend the project is cross-platform when it is not.

Rules:

- state supported platforms explicitly
- encode platform assumptions in code and documentation
- fail clearly when runtime requirements are missing

### 4. Scope must remain deliberate

The project should not silently expand from MVP into "support everything in XL API."

Rules:

- new bus support requires an explicit decision
- complexity must be paid for only when there is clear demand and a test path
- unfinished or speculative support is worse than a clean omission

### 5. The public API must trade a little flexibility for a lot of clarity

The wrapper layer should feel Rust-native.

Rules:

- prefer typed operations over loosely structured parameter bags
- prefer explicit errors over silent fallback behavior
- prefer RAII and ownership-based cleanup over manual shutdown sequences in user code

### 6. Behavior must be diagnosable

When something fails, developers should be able to understand why without reverse engineering the wrapper.

Rules:

- preserve raw XL status codes
- expose readable error messages where possible
- avoid swallowing driver errors
- avoid hidden retries or hidden mode switches

### 7. Documentation is part of the implementation

For this project, design intent is not optional context. It is part of the codebase.

Rules:

- document non-obvious architectural decisions
- update docs when scope, API contracts, or verification expectations change
- prefer small authoritative documents over scattered unwritten conventions

## Decision Rules

When there is a tradeoff, prefer the option that best satisfies the following order:

1. ABI and memory-layout correctness
2. safety and explicit ownership
3. debuggability
4. small change surface
5. ergonomic API quality
6. breadth of feature coverage

## What We Do Not Optimize For

The project does not currently optimize for:

- maximum XL API coverage as fast as possible
- cross-platform support
- async-first design
- backward compatibility with speculative APIs that have not stabilized in this repository
- minimizing code size at the expense of readability in FFI-sensitive code

## Definition of Good Output

A high-quality change in this repository usually has these properties:

- it keeps the ABI mapping exact
- it adds the smallest necessary surface area
- it moves `unsafe` inwards, not outwards
- it includes verification proportional to risk
- it leaves behind documentation that makes the next change easier
