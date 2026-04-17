# Branching Strategy

## Purpose

This document defines the source-control workflow expected in this repository.

The repository uses a single-branch strategy by default. The goal is to keep history simple, reduce integration drag, and match the current MVP-scale team and change volume.

## Default Strategy

- `main` is the only long-lived branch
- normal work is integrated back into `main`
- do not create permanent `develop`, release, support, or bus-specific branches
- do not rely on branch taxonomies to represent task state

## Normal Change Flow

For a normal change:

1. create or update the relevant Markdown task file under `docs/tasks/`
2. implement the smallest complete slice
3. run the appropriate local verification
4. commit the change against `main`
5. record the task link or identifier in the review context, commit message, or both when practical

The policy is intentionally simple. Task state lives in Markdown, not in branch names.

## Optional Local Safety Branches

Short-lived local branches are allowed only as personal safety rails when they help isolate unfinished work.

Rules:

- they are temporary and disposable
- they must not become long-lived integration branches
- they must not replace the required task record
- they must not change the fact that `main` is the only long-lived shared branch

If a short-lived branch is used, integrate it back quickly and keep the final history easy to understand.

## Review and Integration

- branch-based pull requests are optional tooling, not a required process step
- if a hosted review is used, the target branch is `main`
- do not introduce merge trains, stacked branches, or branch-per-bus conventions without an explicit policy change
- prefer straightforward history over elaborate merge strategy rules

## Release Points

- release tags are cut from `main`
- do not create release branches by default
- if release isolation is ever needed later, update this document first

## What To Avoid

- long-lived feature branches
- parallel stabilization branches
- branch naming schemes used as a task tracker
- process rules that assume CI, PR gates, or merge automation already exist
