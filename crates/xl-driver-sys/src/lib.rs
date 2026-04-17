#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

#[cfg(not(windows))]
compile_error!("xl-driver-sys currently supports Windows only.");

pub mod can;
pub mod canfd;
pub mod loader;
pub mod types;
