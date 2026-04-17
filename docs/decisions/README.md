# Decisions

This directory stores lightweight ADRs for decisions that should outlive a single task.

## When To Write An ADR

Write an ADR when a decision changes one of these:

- repository workflow
- architecture direction
- public API direction
- verification expectations
- dependency policy

Do not write an ADR for routine edits that simply follow existing policy.

## ADR Format

Keep ADRs short. A lightweight ADR should usually fit in one to two pages and include these sections:

- `Status`
- `Date`
- `Context`
- `Decision`
- `Consequences`

## Naming

- use zero-padded numeric prefixes such as `0001-short-title` with the normal `.md` extension
- never renumber accepted ADRs
- supersede an old ADR with a new ADR instead of rewriting history silently

## Relationship to Tasks

- tasks explain the work that is being executed now
- ADRs explain durable decisions that should still make sense later
- link the relevant task from the ADR when practical
