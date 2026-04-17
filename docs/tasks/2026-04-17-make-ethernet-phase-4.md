# Make Ethernet network-based the dedicated Phase 4 slice

- Status: done
- Scope: update the post-MVP roadmap so Ethernet via the network-based API is the explicit Phase 4 slice, with the remaining selective expansion guidance shifted after it
- Links: `AGENTS.md`, `docs/architecture/porting-plan.md`, `docs/process/development-guide.md`, `docs/progress.md`

## Steps

- [x] create the task record and keep it current
- [x] update the architecture roadmap to make Ethernet network-based the dedicated Phase 4
- [x] align contributor instructions and the development guide with the new post-MVP order
- [x] update the progress dashboard so the roadmap phases stay consistent

## Verification

- Reviewed the updated roadmap, process guidance, and agent instructions for consistent phase numbering and post-MVP ordering.
- Ran `rg -n "Phase 4|Phase 5|Ethernet|LIN|network-based" AGENTS.md docs/process/development-guide.md docs/architecture/porting-plan.md docs/progress.md`.

## Outcome

- Phase 4 is now the dedicated post-MVP Ethernet slice and explicitly calls for the network-based API.
- The remaining selective expansion guidance moved behind that as Phase 5.
- Contributor instructions, the development guide, the architecture plan, and the dashboard now use the same post-MVP ordering.
