# AGENTS.md

## Purpose

This file defines the repository-level instructions that agents and contributors must follow when working in this project.

These instructions are binding for implementation behavior inside this repository.

## Required Reads

Before making non-trivial changes, read these documents:

1. `docs/standards/engineering-principles.md`
2. `docs/standards/rust-coding-style.md`
3. `docs/process/development-guide.md`
4. `docs/process/branching-strategy.md`
5. `docs/process/task-tracking.md`
6. `docs/architecture/porting-plan.md`

Also read any relevant existing task file under `docs/tasks/` and any relevant ADR under `docs/decisions/` before changing the behavior they describe.

If there is a conflict:

- `AGENTS.md` wins over other repository docs
- implementation process is governed by `docs/process/development-guide.md`
- branching and integration process is governed by `docs/process/branching-strategy.md`
- task tracking process is governed by `docs/process/task-tracking.md`
- coding patterns are governed by `docs/standards/rust-coding-style.md`
- project direction and scope are governed by `docs/architecture/porting-plan.md`

## Current Project Constraints

Assume the following unless the repository docs are updated:

- target platform: Windows x64
- target Rust toolchain target: `x86_64-pc-windows-msvc`
- driver access model: dynamic loading of `vxlapi64.dll`
- architecture: `xl-driver-sys` for raw FFI and `xl-driver` for safe wrapper APIs
- active scope: MVP only
- MVP scope: common lifecycle + CAN + CAN FD

Do not silently expand beyond MVP scope.

## Must-Follow Engineering Rules

- Prefer ABI correctness over feature breadth.
- Keep `unsafe` code as small and localized as possible.
- Treat `vxlapi.h` as the source of truth for raw ABI definitions.
- Use explicit-width Rust types for ABI mappings.
- Use `extern "system"` for XL API function signatures on Windows.
- Do not expose raw handles as the default public API.
- Preserve raw XL status codes in errors.
- Do not add hidden retries, hidden fallbacks, or silent mode switches.
- Do not add new bus support without an explicit scope decision.

## Layering Rules

In `xl-driver-sys`:

- place raw types, constants, loader logic, and raw function calls
- do not place ergonomic workflow logic there

In `xl-driver`:

- place resource ownership, validation, typed errors, and ergonomic APIs
- do not duplicate raw ABI declarations there

## Verification Rules

Every meaningful code change must include verification appropriate to its risk.

Minimum expectations:

- ABI-sensitive changes require layout or type-width validation
- loader changes require smoke verification when possible
- public API changes require tests or examples when practical
- if hardware verification cannot run, state that explicitly in the final report

Do not claim verification that did not actually happen.

## Documentation Rules

Update docs when behavior or contributor expectations change.

Keep `docs/progress.md` updated continuously.

At minimum, update the dashboard whenever a meaningful slice changes:

- the current project status
- roadmap phase state
- recent completed work
- the next planned slice

At minimum, consider whether these files need updates:

- `README.md`
- `docs/README.md`
- `docs/progress.md`
- `docs/standards/engineering-principles.md`
- `docs/standards/rust-coding-style.md`
- `docs/process/development-guide.md`
- `docs/process/branching-strategy.md`
- `docs/process/task-tracking.md`
- `docs/process/release-guide.md`
- `docs/tasks/README.md`
- `docs/decisions/README.md`
- `docs/architecture/porting-plan.md`
- `AGENTS.md`

## Change Planning Rules

When implementing, prefer the smallest complete slice.

Preferred pattern:

- add only the raw ABI needed now
- add the safe wrapper that uses it
- add the verification that proves it
- update docs if the contract changed

Avoid broad speculative scaffolding for APIs that are not yet being delivered.

## Expansion Policy

After MVP, expand selectively, not automatically.

The first post-MVP expansion phase is Ethernet via the network-based API.

After that, default candidate order:

1. LIN
2. TimeSync basics
3. FlexRay
4. DAIO / A429 / MOST-family buses

Only expand when:

- there is clear demand
- a realistic verification path exists
- the architecture does not need premature generalization

## Final Checklist for Agents

Before concluding work, verify:

- the relevant task file under `docs/tasks/` was updated for the work that was performed
- the single-branch workflow in `docs/process/branching-strategy.md` was followed
- scope stayed inside the intended boundary
- `unsafe` remained localized
- ABI mappings are explicit and credible
- public APIs are clearer than raw APIs
- verification was actually run where possible
- any relevant ADRs were added or updated when a durable decision changed
- linked review context, commit message, or task file referenced the work when applicable
- `docs/progress.md` still reflects the current project state and next planned slice
- documentation is still consistent with the codebase
