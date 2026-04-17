# Development Guide

## Purpose

This document defines how changes should be planned, implemented, verified, and documented in this repository.

The goal is repeatable execution, not individual coding style preferences.

## Required Workflow

### 1. Confirm scope before coding

Before implementation, confirm:

- whether the work is inside the current MVP scope
- which layer is affected: `xl-driver-sys`, `xl-driver`, or both
- whether the change alters ABI mappings, public API contracts, or verification requirements

If the change adds support for a new bus outside MVP, it should be treated as an explicit scope extension.

### 2. Record or update the task

Before non-trivial implementation work:

- create or update the relevant Markdown task record under `docs/tasks/`
- make the task scope narrow enough to match one meaningful delivery slice
- note any linked ADRs, review context, or blocking dependencies

Task status must be updated again when the work is blocked, completed, or dropped.

### 3. Choose the smallest valid layer

Use this rule:

- if the change is about ABI, layout, symbols, or DLL loading, change `xl-driver-sys`
- if the change is about Rust ergonomics, ownership, validation, or high-level behavior, change `xl-driver`
- do not place wrapper policy into the raw layer
- do not duplicate raw ABI definitions in the safe layer

### 4. Implement the smallest useful slice

Prefer narrow vertical slices over wide partial scaffolding.

Good example:

- add the raw binding for a CAN function
- add the safe wrapper that uses it
- add the verification that proves it works

Bad example:

- add raw declarations for dozens of future APIs with no immediate consumer or validation path

### 5. Verify proportionally

Every change must include verification proportional to risk.

Minimum expectation:

- build verification for all changes
- layout verification for ABI-sensitive changes
- behavior verification for wrapper changes

Record the exact verification that ran in the relevant task file, release note, or review context.

### 6. Update docs when contracts change

Update docs when any of these change:

- supported scope
- platform assumptions
- crate boundaries
- public API usage pattern
- extension policy
- verification expectations
- repository workflow assumptions
- dashboard-level project status or the next planned slice

## Quality Gates by Change Type

## Raw ABI change

Examples:

- new C type alias
- new struct or union mapping
- new DLL symbol
- changed calling convention

Required:

- header-based verification of the mapping
- size and layout assertions where applicable
- smoke verification if the loader surface changed
- notes in docs if the change affects contributor behavior

## Safe wrapper change

Examples:

- new public method
- new resource ownership path
- new conversion or validation logic

Required:

- unit tests for the new behavior where practical
- explicit error behavior
- docs or examples for public API changes

## Scope extension

Examples:

- new bus support
- new runtime mode
- new crate-level dependency that changes architecture

Required:

- explicit decision that the extension is desired now
- identified verification path
- update to relevant docs and `AGENTS.md` if expectations change

## Verification Checklist

Before considering work complete, check:

- does the ABI mapping still match the header
- is the `unsafe` surface still localized
- does the safe wrapper avoid leaking raw implementation details
- are errors explicit and debuggable
- are unsupported cases rejected clearly
- is scope still consistent with the current repository plan

## MVP Completion Criteria

The MVP should be considered complete only when all of the following are true:

- raw loader and core lifecycle APIs work reliably
- CAN open, activate, configure, transmit, and receive paths are implemented
- CAN FD open, configure, transmit, and receive paths are implemented
- ABI-sensitive types used by MVP are layout-validated
- documentation matches actual repository behavior

MVP completion does not imply that all remaining XL buses will be implemented.

## Selective Expansion Policy

After MVP, expansion should be bus-by-bus and justified explicitly.

A new bus should be added only when most of the following are true:

- there is real user or project demand
- the bus can be tested on actual hardware or a trustworthy validation setup
- the new work can reuse the current loader and port abstractions without major distortion
- the bus has a clear recommended API path in Vector documentation
- the implementation can be delivered as a complete slice instead of a speculative scaffold

Expansion should usually be rejected or deferred when:

- hardware verification is unavailable
- the bus adds large complexity with unclear payoff
- only legacy paths exist and there is no clear reason to support them now
- the work would force premature generalization of the current architecture

## Suggested Expansion Order

If expansion is approved, the first post-MVP phase is Ethernet using the network-based API.

After that, the default order is:

1. LIN
2. TimeSync basics
3. FlexRay
4. DAIO, A429, MOST-family buses

This order is a default priority, not a promise.

## Documentation Maintenance Rules

Keep the document set coherent.

When changing implementation expectations:

- update `docs/standards/engineering-principles.md` if the design philosophy or decision rules change
- update `docs/standards/rust-coding-style.md` if coding patterns or API rules change
- update `docs/progress.md` when the current project status, roadmap phase state, recent completed work, or next planned slice changed
- update `docs/process/development-guide.md` if workflow or quality gates change
- update `docs/process/branching-strategy.md` if branch or merge rules change
- update `docs/process/task-tracking.md` if task tracking rules change
- update `docs/process/release-guide.md` if release or publication rules change
- update `docs/tasks/README.md` if task file structure or naming rules change
- update `docs/decisions/README.md` if ADR rules or format change
- update `docs/architecture/porting-plan.md` if scope or architecture direction changes
- update `README.md` if the project status, requirements, or workspace layout changed
- update `AGENTS.md` if the instructions that agents must follow change

## Review Expectations

A reviewer should be able to answer yes to these questions:

- is the scope appropriate
- is the ABI handling explicit and credible
- is `unsafe` justified and localized
- is the wrapper API clearer than the raw API it hides
- is verification matched to the risk
- will the next contributor understand why this code looks the way it does

If the answer to one of these is no, the change is not ready.
