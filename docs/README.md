# Documentation Overview

This directory uses a shallow, type-based structure.

## Structure

```text
docs/
  README.md
  architecture/
  decisions/
  process/
  standards/
  tasks/
```

## Directory Rules

### `standards/`

Use this directory for relatively stable rules and conventions that contributors are expected to follow.

Current files:

- `standards/engineering-principles.md`
- `standards/rust-coding-style.md`

### `process/`

Use this directory for documents that define how work is planned, tracked, executed, and verified.

Current files:

- `process/development-guide.md`
- `process/branching-strategy.md`
- `process/task-tracking.md`
- `process/release-guide.md`

### `architecture/`

Use this directory for project direction, design plans, and implementation strategy documents.

Current files:

- `architecture/porting-plan.md`

### `decisions/`

Use this directory for lightweight ADRs that capture durable technical or process decisions.

Current files:

- `decisions/README.md`
- `decisions/0001-single-branch-and-markdown-tracking.md`

### `tasks/`

Use this directory for Markdown task records that track planned, active, blocked, and completed work.

Current files:

- `tasks/README.md`
- dated task records such as `tasks/2026-04-17-document-process-refresh.md`

## General Rules

- keep the structure shallow
- do not nest beyond one level unless there is a strong reason
- do not duplicate vendor materials from `references/` into `docs/`
- keep the root `README.md` aligned with the active repository state
- keep live task tracking inside `docs/tasks/`, not mixed into standards or architecture docs
- keep durable decisions inside `docs/decisions/`, not buried in task notes
- update `AGENTS.md` when required-read paths change
- update document references when files are moved or renamed
