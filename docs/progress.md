# Project Progress

Last updated: 2026-04-17

## Snapshot

- Overall status: pre-release MVP in progress
- Target platform: Windows x64
- Runtime model: dynamic loading of `vxlapi64.dll`
- Active scope: common lifecycle + CAN + CAN FD
- Current focus: move from loader/lifecycle foundation into Phase 2 CAN workflow

## Roadmap Status

| Phase | Scope | Status | Notes |
| --- | --- | --- | --- |
| Phase 0 | workspace foundation | done | workspace skeleton builds on Windows x64 |
| Phase 1 | `xl-driver-sys` loader + common lifecycle foundation | done | raw loader, lifecycle symbols, ABI layout checks, `Driver::open()` wrapper |
| Phase 2 | CAN workflow | next | port open/activate path, bitrate setup, notification, transmit, receive |
| Phase 3 | CAN FD workflow | pending | configure CAN FD port, transmit, receive |
| Phase 4 | selective expansion after MVP | blocked by MVP completion | LIN is the default first candidate after MVP |

## MVP Checklist

| Item | Status | Notes |
| --- | --- | --- |
| Raw DLL loader works | done | dynamic loading implemented in `xl-driver-sys` |
| Core lifecycle symbols resolved | done | Phase 1 lifecycle slice is loaded and smoke-tested |
| Safe `Driver` open/close API | done | `Driver::open()` and RAII cleanup exist |
| ABI-sensitive type validation | partial | core widths plus `XLcanFdConf`, `XLchannelConfig`, and `XLdriverConfig` are checked |
| CAN open/activate/configure | not started | Phase 2 target |
| CAN transmit/receive | not started | Phase 2 target |
| CAN FD open/configure | not started | Phase 3 target |
| CAN FD transmit/receive | not started | Phase 3 target |
| Hardware-backed bus verification | not started | only loader-level smoke verification has run so far |

## Recent Completed Work

- [Implement Phase 1 loader and lifecycle foundation](tasks/2026-04-17-phase-1-loader-and-lifecycle.md)
- [Decouple loader smoke test from vendored DLL path](tasks/2026-04-17-decouple-smoke-test-from-vendored-dll.md)
- [Refresh documentation process for repository workflow](tasks/2026-04-17-document-process-refresh.md)

## Next Slice

The next planned slice is Phase 2 CAN support:

- open a CAN port through the existing raw lifecycle layer
- activate/deactivate channels explicitly
- configure CAN bitrate
- connect notification and receive-queue handling
- add the first CAN transmit/receive path with proportional verification

## Where To Look

- Roadmap: [Porting Plan](architecture/porting-plan.md)
- Live and completed work: [Tasks](tasks/README.md)
- Repository documentation index: [Documentation Overview](README.md)
