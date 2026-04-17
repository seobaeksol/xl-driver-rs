# 0001. Single branch and Markdown tracking

- Status: accepted
- Date: 2026-04-17

## Context

The repository documentation referenced branching and task-tracking documents that did not exist.

At the same time, the current repository state does not justify a heavier workflow with multiple long-lived branches, external task tooling, or CI-dependent policy.

The process needs to stay small, reviewable, and easy to follow while the project is still inside the MVP window.

## Decision

The repository will:

- use `main` as the single long-lived branch
- track work in Markdown files under `docs/tasks/`
- record durable technical or process decisions as lightweight ADRs under `docs/decisions/`
- treat locally recorded verification as the process source of truth unless and until CI is introduced explicitly

## Consequences

- task state is no longer implied by branch names or pull-request flow
- contributors must update the relevant task record before closing meaningful work
- process and architecture changes should add or update an ADR when the decision is expected to remain relevant
- documents must not claim CI behavior that the repository does not currently provide
