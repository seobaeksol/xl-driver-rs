#[cfg(not(windows))]
compile_error!("xl-driver currently supports Windows only.");

pub mod can;
pub mod canfd;
pub mod driver;
pub mod error;
pub mod port;

pub use can::{CanEvent, CanFrame, CanOutputMode, CanQueueSize};
pub use canfd::{CanFdConfig, CanFdQueueSize};
pub use driver::Driver;
pub use error::XlError;
pub use port::Port;
pub use xl_driver_sys::LibraryLocation;
