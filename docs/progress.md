# Project Progress

Last updated: 2026-04-17

## Snapshot

- Overall status: pre-release MVP in progress
- Target platform: Windows x64
- Runtime model: dynamic loading of `vxlapi64.dll`
- Active scope: common lifecycle + CAN + CAN FD
- Current focus: finish Phase 2 by validating the new classic CAN workflow on hardware or loopback

## Roadmap Status

| Phase | Scope | Status | Notes |
| --- | --- | --- | --- |
| Phase 0 | workspace foundation | done | workspace skeleton builds on Windows x64 |
| Phase 1 | `xl-driver-sys` loader + common lifecycle foundation | done | raw loader, lifecycle symbols, ABI layout checks, `Driver::open()` wrapper |
| Phase 2 | CAN workflow | in_progress | classic CAN port open/configure/activate/notification/transmit/receive API exists; hardware-backed validation is still pending |
| Phase 3 | CAN FD workflow | pending | configure CAN FD port, transmit, receive |
| Phase 4 | selective expansion after MVP | blocked by MVP completion | LIN is the default first candidate after MVP |

## MVP Checklist

| Item | Status | Notes |
| --- | --- | --- |
| Raw DLL loader works | done | dynamic loading implemented in `xl-driver-sys` |
| Core lifecycle symbols resolved | done | Phase 1 lifecycle slice is loaded and smoke-tested |
| Safe `Driver` open/close API | done | `Driver::open()` and RAII cleanup exist |
| ABI-sensitive type validation | partial | core widths plus `XLcanFdConf`, `XLchannelConfig`, and `XLdriverConfig` are checked |
| CAN open/activate/configure | partial | safe API implemented; hardware-backed verification still pending |
| CAN transmit/receive | partial | safe API implemented; hardware-backed verification still pending |
| CAN FD open/configure | not started | Phase 3 target |
| CAN FD transmit/receive | not started | Phase 3 target |
| Hardware-backed bus verification | not started | only loader-level smoke verification has run so far |

## Recent Completed Work

- [Implement Phase 1 loader and lifecycle foundation](tasks/2026-04-17-phase-1-loader-and-lifecycle.md)
- [Decouple loader smoke test from vendored DLL path](tasks/2026-04-17-decouple-smoke-test-from-vendored-dll.md)
- [Implement Phase 2 CAN port workflow](tasks/2026-04-17-phase-2-can-port-workflow.md)
- [Refresh documentation process for repository workflow](tasks/2026-04-17-document-process-refresh.md)

## Next Slice

The next planned slice is Phase 2 CAN validation:

- run the new classic CAN workflow against real hardware or a credible loopback setup
- verify port open, bitrate configuration, activation, transmit, notification, and receive behavior end to end
- tighten the API based on runtime findings instead of adding more speculative surface area
- begin Phase 3 CAN FD work only after the Phase 2 runtime path is credible

## Where To Look

- Roadmap: [Porting Plan](architecture/porting-plan.md)
- Live and completed work: [Tasks](tasks/README.md)
- Repository documentation index: [Documentation Overview](README.md)
