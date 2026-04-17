# Examples

Repository-level notes for runnable examples.

## Current Rule

Do not add placeholder example code that does not exercise a real public API.

The current runnable examples live under `crates/xl-driver/examples/` so they can
be built directly with `cargo run -p xl-driver --example <name>`.

Examples should be added only when they can demonstrate an actual supported slice such as:

- driver open and close
- CAN port open and activation
- CAN transmit and receive
- CAN FD configuration and receive flow

## Expectations

Each example should be:

- small
- directly runnable in a supported Windows environment
- aligned with the current MVP scope
- updated when the public API changes
