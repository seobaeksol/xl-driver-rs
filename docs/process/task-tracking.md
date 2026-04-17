# Task Tracking

## Purpose

This document defines how work is tracked in this repository.

Task tracking is Markdown-based by default. The goal is to keep planning, execution notes, verification, and completion status visible in the repository without introducing separate tooling.

## Source of Truth

- task records live under `docs/tasks/`
- one task is tracked in one Markdown file
- use the filename pattern `YYYY-MM-DD-short-title` with the normal `.md` extension
- keep the current status inside the file body, not only in the filename

## Required Task Shape

Each non-trivial task file should include at least:

- a clear title
- `Status`
- `Scope`
- `Links`
- a checklist or step list
- `Verification`
- `Outcome`

The exact wording may vary, but those facts must be easy to find.

## Status Values

Use one of these values:

- `proposed`
- `in_progress`
- `blocked`
- `done`
- `dropped`

Do not invent extra status labels unless the process is updated here first.

## Workflow Rules

### Before work starts

- create the task file or update the existing one
- define a scope that matches a small complete slice
- link any related ADRs, issues, review threads, or external context

### While work is in progress

- keep the checklist and notes current enough that another contributor can resume the task
- update the status if the work becomes blocked or changes direction materially
- keep temporary observations in the task file instead of scattering them across unrelated docs

### When work completes

- record the exact verification that actually ran
- record any hardware verification gaps explicitly
- summarize the result and set the status to `done` or `dropped`

## Relationship to Other Documents

- use `docs/tasks/` for live task state
- use `docs/decisions/` for durable decisions
- use `docs/process/` for repository rules
- do not turn standards or architecture documents into running task journals

## What Not To Do

- do not track task state only in chat or review comments
- do not treat branch names as a task system
- do not claim verification that is not written down in the task or release context
- do not leave a task marked `in_progress` after the work is clearly done or dropped
