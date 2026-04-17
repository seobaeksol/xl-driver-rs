# Project Progress

Last updated: 2026-04-17

## Snapshot

- Overall status: pre-release MVP in progress
- Target platform: Windows x64
- Runtime model: dynamic loading of `vxlapi64.dll`
- Active scope: common lifecycle + CAN + CAN FD
- Current focus: start Phase 3 by building the first CAN FD workflow on top of the validated classic CAN foundation

## Roadmap Status

| Phase | Scope | Status | Notes |
| --- | --- | --- | --- |
| Phase 0 | workspace foundation | done | workspace skeleton builds on Windows x64 |
| Phase 1 | `xl-driver-sys` loader + common lifecycle foundation | done | raw loader, lifecycle symbols, ABI layout checks, `Driver::open()` wrapper |
| Phase 2 | CAN workflow | done | classic CAN path now uses `xlCreatePort`/`xlAddChannelToPort`/`xlFinalizePort` and was validated on a Vector virtual CAN channel |
| Phase 3 | CAN FD workflow | pending | configure CAN FD port, transmit, receive |
| Phase 4 | Ethernet network-based | blocked by MVP completion | dedicated first post-MVP expansion slice |
| Phase 5 | selective expansion after MVP | blocked by MVP completion | LIN is the default next candidate after Ethernet |

## MVP Checklist

| Item | Status | Notes |
| --- | --- | --- |
| Raw DLL loader works | done | dynamic loading implemented in `xl-driver-sys` |
| Core lifecycle symbols resolved | done | Phase 1 lifecycle slice is loaded and smoke-tested |
| Safe `Driver` open/close API | done | `Driver::open()` and RAII cleanup exist |
| ABI-sensitive type validation | partial | core widths plus `XLcanFdConf`, `XLchannelConfig`, and `XLdriverConfig` are checked |
| CAN open/activate/configure | done | safe API uses channel discovery plus `xlCreatePort`/`xlAddChannelToPort` and was exercised on a Vector virtual CAN channel |
| CAN transmit/receive | done | classic CAN transmit plus event observation passed on a Vector virtual CAN channel |
| CAN FD open/configure | not started | Phase 3 target |
| CAN FD transmit/receive | not started | Phase 3 target |
| Hardware-backed bus verification | partial | classic CAN was validated on a Vector virtual CAN channel; CAN FD still has no runtime validation |

## Recent Completed Work

- [Implement Phase 1 loader and lifecycle foundation](tasks/2026-04-17-phase-1-loader-and-lifecycle.md)
- [Decouple loader smoke test from vendored DLL path](tasks/2026-04-17-decouple-smoke-test-from-vendored-dll.md)
- [Implement Phase 2 CAN port workflow](tasks/2026-04-17-phase-2-can-port-workflow.md)
- [Validate Phase 2 classic CAN runtime path](tasks/2026-04-17-phase-2-can-validation.md)
- [Make Ethernet network-based the dedicated Phase 4 slice](tasks/2026-04-17-make-ethernet-phase-4.md)
- [Refresh documentation process for repository workflow](tasks/2026-04-17-document-process-refresh.md)

## Next Slice

The next planned slice is Phase 3 CAN FD:

- add the first raw CAN FD event/configuration bindings required by the MVP
- expose a safe CAN FD port open/configure path
- add transmit and receive handling for CAN FD events
- validate the CAN FD runtime path with the same standard used to close Phase 2

## Where To Look

- Roadmap: [Porting Plan](architecture/porting-plan.md)
- Live and completed work: [Tasks](tasks/README.md)
- Repository documentation index: [Documentation Overview](README.md)
