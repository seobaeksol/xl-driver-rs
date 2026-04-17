# xl-driver-rs

`xl-driver-rs` is a Rust workspace for building a reliable Rust interface over the Vector XL Driver.

The project does not reimplement the driver. It targets `vxlapi64.dll` through a raw FFI layer and a safe wrapper layer.

## Status

- pre-release
- Windows x64 only
- MVP scope only
- current MVP: common lifecycle + CAN + CAN FD

## Requirements

- Windows 10 or Windows 11 x64
- Rust target: `x86_64-pc-windows-msvc`
- a local Vector XL Driver installation
- access to `vxlapi64.dll`

The repository includes local reference materials under `references/xl-driver`, but runtime use still depends on the actual driver installation in the target environment.

## Workspace Layout

```text
crates/
  xl-driver-sys/   raw ABI mappings, DLL loading, raw calls
  xl-driver/       safe wrapper APIs and resource management
docs/
  architecture/    project direction and implementation plan
  decisions/       lightweight ADRs for durable technical decisions
  process/         workflow, tracking, and release process
  standards/       engineering and Rust coding rules
  tasks/           markdown task records and execution history
examples/          runnable examples once public APIs exist
references/        vendor headers, DLLs, manuals, and samples
```

## Project Rules

Repository-level contributor rules live in [AGENTS.md](AGENTS.md).

Active and completed work is tracked in Markdown under [docs/tasks/README.md](docs/tasks/README.md).

Durable architecture or process decisions are recorded as lightweight ADRs under [docs/decisions/README.md](docs/decisions/README.md).

The main project documents are:

- [Documentation Overview](docs/README.md)
- [Porting Plan](docs/architecture/porting-plan.md)
- [Development Guide](docs/process/development-guide.md)
- [Branching Strategy](docs/process/branching-strategy.md)
- [Task Tracking](docs/process/task-tracking.md)
- [ADR Guide](docs/decisions/README.md)
- [Release Guide](docs/process/release-guide.md)
- [Engineering Principles](docs/standards/engineering-principles.md)
- [Rust Coding Style](docs/standards/rust-coding-style.md)

## Baseline Verification

Use these commands as the default local quality gate before sharing a change:

```powershell
cargo fmt --all
cargo check --workspace --all-targets
cargo clippy --workspace --all-targets -- -D warnings
cargo test --workspace --all-targets
```

Record the exact commands that actually ran in the relevant task record or release note.

## License

No project license has been selected yet.

Until a license is added explicitly, treat the repository as internal and do not assume it is ready for external redistribution.
