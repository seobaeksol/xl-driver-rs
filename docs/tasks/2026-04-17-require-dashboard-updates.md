# Require continuous dashboard updates

- Status: done
- Scope: update repository instructions so contributors keep `docs/progress.md` current as project status changes
- Links: `AGENTS.md`, `docs/process/development-guide.md`, `docs/progress.md`

## Steps

- [x] add the task record and keep it current
- [x] update `AGENTS.md` to require continuous dashboard maintenance
- [x] update supporting process documentation if needed
- [x] verify the new instruction is clear and points at the existing dashboard

## Verification

- Reviewed the updated wording in `AGENTS.md`.
- Reviewed the matching process wording in `docs/process/development-guide.md`.
- Ran `rg -n "docs/progress\\.md|dashboard|next planned slice|recent completed work|roadmap phase state" AGENTS.md docs/process/development-guide.md`.

## Outcome

- `AGENTS.md` now requires contributors to keep `docs/progress.md` updated continuously when a meaningful slice changes current status, roadmap phase state, recent completed work, or the next planned slice.
- `AGENTS.md` now includes `docs/progress.md` in the documentation update consideration list and final checklist.
- `docs/process/development-guide.md` now mirrors the dashboard maintenance expectation so the process docs remain consistent with the repository-level instruction.
