#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

#[cfg(not(windows))]
compile_error!("xl-driver-sys currently supports Windows only.");

pub mod can;
pub mod canfd;
pub mod loader;
pub mod types;

pub use loader::{DEFAULT_XL_API_DLL, LibraryLocation, XlApi, XlApiLoadError};
pub use types::{
    XL_ACTIVATE_NONE, XL_BUS_TYPE_NONE, XL_CONFIG_MAX_CHANNELS, XL_INTERFACE_VERSION,
    XL_INTERFACE_VERSION_V2, XL_INTERFACE_VERSION_V3, XL_INTERFACE_VERSION_V4,
    XL_INVALID_PORTHANDLE, XL_MAX_LENGTH, XL_SUCCESS, XLaccess, XLbusParams, XLchannelConfig,
    XLdriverConfig, XLhandle, XLlong, XLportHandle, XLstatus, XLstringType, XLuint64, XLulong,
};
