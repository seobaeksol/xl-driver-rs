#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

#[cfg(not(windows))]
compile_error!("xl-driver-sys currently supports Windows only.");

pub mod can;
pub mod canfd;
pub mod loader;
pub mod types;

pub use can::{
    MAX_MSG_LEN, XL_BUS_TYPE_CAN, XL_CAN_EXT_MSG_ID, XL_CAN_MSG_FLAG_REMOTE_FRAME,
    XL_CAN_MSG_FLAG_TX_COMPLETED, XL_EVENT_FLAG_OVERRUN, XL_OUTPUT_MODE_NORMAL,
    XL_OUTPUT_MODE_SILENT, XL_OUTPUT_MODE_SJA_1000_SILENT, XL_OUTPUT_MODE_TX_OFF, XL_RECEIVE_MSG,
    XL_TRANSMIT_MSG, XL_USE_ALL_CHANNELS, XLcanMsg, XLevent, XLeventTag, XLeventTagData,
};
pub use loader::{DEFAULT_XL_API_DLL, LibraryLocation, XlApi, XlApiLoadError};
pub use types::{
    XL_ACTIVATE_NONE, XL_ACTIVATE_RESET_CLOCK, XL_BUS_TYPE_NONE, XL_CONFIG_MAX_CHANNELS,
    XL_ERR_QUEUE_IS_EMPTY, XL_INTERFACE_VERSION, XL_INTERFACE_VERSION_V2, XL_INTERFACE_VERSION_V3,
    XL_INTERFACE_VERSION_V4, XL_INVALID_PORTHANDLE, XL_MAX_APPNAME, XL_MAX_LENGTH, XL_SUCCESS,
    XLaccess, XLbusParams, XLchannelConfig, XLdriverConfig, XLhandle, XLlong, XLportHandle,
    XLstatus, XLstringType, XLuint64, XLulong,
};
