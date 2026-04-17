# Tasks

This directory stores Markdown task records for planned, active, blocked, and completed work.

## Naming

- use `YYYY-MM-DD-short-title` plus the normal `.md` extension
- keep the title short but specific enough to search later
- create a new file for a new slice of work instead of rewriting old task history

## Minimal Template

```md
# Short task title

- Status: proposed
- Scope: one small complete slice
- Links: related docs, ADRs, issues, or review context

## Steps

- [ ] first step
- [ ] second step

## Verification

- Not run yet.

## Outcome

- Not completed yet.
```

## Rules

- update the task before and after non-trivial work
- keep the verification section factual
- link the relevant ADR when the task changes a durable decision
- keep completed tasks as history unless there is an explicit cleanup rule later
