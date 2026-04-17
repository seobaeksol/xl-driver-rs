# Refresh documentation process for repository workflow

- Status: done
- Scope: repair broken documentation references and align the repository docs with a single-branch workflow, Markdown task tracking, lightweight ADRs, and no-CI assumptions
- Links: `AGENTS.md`, `docs/process/branching-strategy.md`, `docs/process/task-tracking.md`, `docs/decisions/0001-single-branch-and-markdown-tracking.md`

## Steps

- [x] identify broken document references and structure mismatches
- [x] add missing process documents for branching strategy and task tracking
- [x] introduce `docs/tasks/` and `docs/decisions/` with repository guidance
- [x] verify that document references resolve and update this task with the result

## Verification

- Ran `rg -n "CI|pull request|branch-per|develop|task register|task-tracking\.md|branching-strategy\.md|docs/tasks/|docs/decisions/" README.md AGENTS.md docs examples` to spot stale workflow wording.
- Ran a repository-wide Markdown reference scan for local `.md` and `.pdf` paths; result was `OK` after removing placeholder filename false positives from the docs.

## Outcome

- Added the missing branching strategy and task tracking process docs.
- Added `docs/tasks/` and `docs/decisions/` guidance plus one completed task record and one accepted ADR.
- Updated `README.md`, `docs/README.md`, `docs/process/development-guide.md`, `docs/process/release-guide.md`, and `AGENTS.md` to match the new repository policy.
