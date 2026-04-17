# Release Guide

## Purpose

This document defines how release readiness should be evaluated in this repository.

The project is still pre-release. The goal of this guide is to keep versioning and publication decisions explicit while the MVP is being built.

## Current Release State

- no public release has been made
- both crates are marked `publish = false`
- the repository has no selected outbound license yet
- no required CI workflow is assumed today
- crates.io publication is out of scope until the MVP and repository policy are ready

## Versioning Policy

Until the project is ready for external release:

- use normal Git history for iteration
- treat `main` as the single long-lived branch and the source for release tags
- do not create release branches unless the repository policy is changed explicitly
- keep crate versions aligned unless there is a strong reason not to
- treat version bumps as explicit release-preparation work, not incidental edits

Once external releases are approved, use semantic versioning with these expectations:

- `0.x` while the public API is still expected to change
- patch releases for compatible fixes
- minor releases for compatible API additions
- major releases only when compatibility commitments are established and intentionally broken

## Release Preconditions

Do not prepare a release until all of the following are true:

- MVP scope is complete or the release scope is explicitly documented
- required verification has been run for the included surface
- public API docs and examples match the actual code
- license selection has been made explicitly
- publication targets and ownership are decided explicitly

## Publication Policy

Before any external publication:

- remove or justify `publish = false`
- add a repository license file
- confirm that vendor-related distribution assumptions are acceptable
- review crate metadata such as description, authorship, repository URL, and documentation links
- add or update release notes

Do not publish partial or speculative APIs just because the crate compiles.

## Release Checklist

When release preparation begins, verify:

- `cargo fmt --all --check`
- `cargo check --workspace --all-targets`
- `cargo clippy --workspace --all-targets -- -D warnings`
- `cargo test --workspace --all-targets`
- the release task records the exact verification that ran
- docs are current
- examples are current
- ADRs still match repository behavior

If hardware verification is required for the included API surface, record what was tested and what was not tested.
