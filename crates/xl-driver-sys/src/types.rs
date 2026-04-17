use core::ffi::{c_char, c_void};

/// XL API status code as defined by `vxlapi.h`.
pub type XLstatus = i16;

/// XL API channel access mask as defined by `vxlapi.h`.
pub type XLaccess = u64;

/// XL API port handle as defined by `vxlapi.h`.
///
/// Note: this must remain 32-bit on Windows x64 because the C header uses `long`.
pub type XLportHandle = i32;

/// XL API notification handle.
pub type XLhandle = *mut c_void;

/// Borrowed C string returned by the XL API.
pub type XLstringType = *mut c_char;

pub const XL_INTERFACE_VERSION_V2: u32 = 2;
pub const XL_INTERFACE_VERSION_V3: u32 = 3;
pub const XL_INTERFACE_VERSION_V4: u32 = 4;
pub const XL_INTERFACE_VERSION: u32 = XL_INTERFACE_VERSION_V3;

pub const XL_INVALID_PORTHANDLE: XLportHandle = -1;
