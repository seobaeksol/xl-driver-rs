use core::ffi::c_char;
use std::ffi::CStr;
use std::fmt;
use std::path::{Path, PathBuf};

use libloading::{Library, Symbol};

use crate::can::XLevent;
use crate::types::{XLaccess, XLdriverConfig, XLhandle, XLportHandle, XLstatus, XLstringType};

pub const DEFAULT_XL_API_DLL: &str = "vxlapi64.dll";

/// Repository-level representation of the target XL API DLL location.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LibraryLocation {
    path: PathBuf,
}

impl LibraryLocation {
    pub fn new(path: impl Into<PathBuf>) -> Self {
        Self { path: path.into() }
    }

    pub fn default_dll() -> Self {
        Self::new(DEFAULT_XL_API_DLL)
    }

    pub fn as_path(&self) -> &Path {
        &self.path
    }
}

impl Default for LibraryLocation {
    fn default() -> Self {
        Self::default_dll()
    }
}

impl From<PathBuf> for LibraryLocation {
    fn from(path: PathBuf) -> Self {
        Self::new(path)
    }
}

impl From<&Path> for LibraryLocation {
    fn from(path: &Path) -> Self {
        Self::new(path)
    }
}

impl From<String> for LibraryLocation {
    fn from(path: String) -> Self {
        Self::new(path)
    }
}

impl From<&str> for LibraryLocation {
    fn from(path: &str) -> Self {
        Self::new(path)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum XlApiLoadError {
    LoadLibrary {
        path: PathBuf,
        error: String,
    },
    LoadSymbol {
        path: PathBuf,
        symbol: &'static str,
        error: String,
    },
}

impl fmt::Display for XlApiLoadError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::LoadLibrary { path, error } => {
                write!(
                    f,
                    "failed to load XL API DLL from {}: {error}",
                    path.display()
                )
            }
            Self::LoadSymbol {
                path,
                symbol,
                error,
            } => write!(
                f,
                "failed to resolve {symbol} from {}: {error}",
                path.display()
            ),
        }
    }
}

impl std::error::Error for XlApiLoadError {}

pub type XLOPENDRIVER = unsafe extern "system" fn() -> XLstatus;
pub type XLCLOSEDRIVER = unsafe extern "system" fn() -> XLstatus;
pub type XLGETAPPLCONFIG =
    unsafe extern "system" fn(*mut c_char, u32, *mut u32, *mut u32, *mut u32, u32) -> XLstatus;
pub type XLSETAPPLCONFIG =
    unsafe extern "system" fn(*mut c_char, u32, u32, u32, u32, u32) -> XLstatus;
pub type XLGETDRIVERCONFIG = unsafe extern "system" fn(*mut XLdriverConfig) -> XLstatus;
pub type XLGETCHANNELINDEX = unsafe extern "system" fn(i32, i32, i32) -> i32;
pub type XLGETCHANNELMASK = unsafe extern "system" fn(i32, i32, i32) -> XLaccess;
pub type XLOPENPORT = unsafe extern "system" fn(
    *mut XLportHandle,
    *mut c_char,
    XLaccess,
    *mut XLaccess,
    u32,
    u32,
    u32,
) -> XLstatus;
pub type XLCREATEPORT =
    unsafe extern "system" fn(*mut XLportHandle, *const c_char, u32, u32, u64) -> XLstatus;
pub type XLADDCHANNELTOPORT =
    unsafe extern "system" fn(XLportHandle, XLaccess, u32, *mut u32, u64) -> XLstatus;
pub type XLFINALIZEPORT = unsafe extern "system" fn(XLportHandle) -> XLstatus;
pub type XLSETNOTIFICATION =
    unsafe extern "system" fn(XLportHandle, *mut XLhandle, i32) -> XLstatus;
pub type XLFLUSHRECEIVEQUEUE = unsafe extern "system" fn(XLportHandle) -> XLstatus;
pub type XLGETRECEIVEQUEUELEVEL = unsafe extern "system" fn(XLportHandle, *mut i32) -> XLstatus;
pub type XLACTIVATECHANNEL =
    unsafe extern "system" fn(XLportHandle, XLaccess, u32, u32) -> XLstatus;
pub type XLDEACTIVATECHANNEL = unsafe extern "system" fn(XLportHandle, XLaccess) -> XLstatus;
pub type XLCLOSEPORT = unsafe extern "system" fn(XLportHandle) -> XLstatus;
pub type XLGETERRORSTRING = unsafe extern "system" fn(XLstatus) -> XLstringType;
pub type XLGETEVENTSTRING = unsafe extern "system" fn(*mut XLevent) -> XLstringType;
pub type XLCANSETCHANNELOUTPUT = unsafe extern "system" fn(XLportHandle, XLaccess, i32) -> XLstatus;
pub type XLCANSETCHANNELBITRATE =
    unsafe extern "system" fn(XLportHandle, XLaccess, u32) -> XLstatus;
pub type XLCANTRANSMIT =
    unsafe extern "system" fn(XLportHandle, XLaccess, *mut u32, *mut core::ffi::c_void) -> XLstatus;
pub type XLRECEIVE = unsafe extern "system" fn(XLportHandle, *mut u32, *mut XLevent) -> XLstatus;

/// Loaded XL API entrypoints for the lifecycle and classic CAN MVP slices.
pub struct XlApi {
    _library: Library,
    pub xlOpenDriver: XLOPENDRIVER,
    pub xlCloseDriver: XLCLOSEDRIVER,
    pub xlGetApplConfig: XLGETAPPLCONFIG,
    pub xlSetApplConfig: XLSETAPPLCONFIG,
    pub xlGetDriverConfig: XLGETDRIVERCONFIG,
    pub xlGetChannelIndex: XLGETCHANNELINDEX,
    pub xlGetChannelMask: XLGETCHANNELMASK,
    pub xlOpenPort: XLOPENPORT,
    pub xlCreatePort: XLCREATEPORT,
    pub xlAddChannelToPort: XLADDCHANNELTOPORT,
    pub xlFinalizePort: XLFINALIZEPORT,
    pub xlSetNotification: XLSETNOTIFICATION,
    pub xlFlushReceiveQueue: XLFLUSHRECEIVEQUEUE,
    pub xlGetReceiveQueueLevel: XLGETRECEIVEQUEUELEVEL,
    pub xlActivateChannel: XLACTIVATECHANNEL,
    pub xlDeactivateChannel: XLDEACTIVATECHANNEL,
    pub xlClosePort: XLCLOSEPORT,
    pub xlGetErrorString: XLGETERRORSTRING,
    pub xlGetEventString: XLGETEVENTSTRING,
    pub xlCanSetChannelOutput: XLCANSETCHANNELOUTPUT,
    pub xlCanSetChannelBitrate: XLCANSETCHANNELBITRATE,
    pub xlCanTransmit: XLCANTRANSMIT,
    pub xlReceive: XLRECEIVE,
}

impl XlApi {
    pub fn load(location: &LibraryLocation) -> Result<Self, XlApiLoadError> {
        let path = location.as_path().to_path_buf();

        let library = unsafe {
            // SAFETY: The caller chooses the DLL path, and we keep the `Library`
            // alive for at least as long as all copied function pointers.
            Library::new(&path)
        }
        .map_err(|error| XlApiLoadError::LoadLibrary {
            path: path.clone(),
            error: error.to_string(),
        })?;

        let xlOpenDriver =
            unsafe { load_symbol(&library, &path, b"xlOpenDriver\0", "xlOpenDriver")? };
        let xlCloseDriver =
            unsafe { load_symbol(&library, &path, b"xlCloseDriver\0", "xlCloseDriver")? };
        let xlGetApplConfig =
            unsafe { load_symbol(&library, &path, b"xlGetApplConfig\0", "xlGetApplConfig")? };
        let xlSetApplConfig =
            unsafe { load_symbol(&library, &path, b"xlSetApplConfig\0", "xlSetApplConfig")? };
        let xlGetDriverConfig =
            unsafe { load_symbol(&library, &path, b"xlGetDriverConfig\0", "xlGetDriverConfig")? };
        let xlGetChannelIndex =
            unsafe { load_symbol(&library, &path, b"xlGetChannelIndex\0", "xlGetChannelIndex")? };
        let xlGetChannelMask =
            unsafe { load_symbol(&library, &path, b"xlGetChannelMask\0", "xlGetChannelMask")? };
        let xlOpenPort = unsafe { load_symbol(&library, &path, b"xlOpenPort\0", "xlOpenPort")? };
        let xlCreatePort =
            unsafe { load_symbol(&library, &path, b"xlCreatePort\0", "xlCreatePort")? };
        let xlAddChannelToPort = unsafe {
            load_symbol(
                &library,
                &path,
                b"xlAddChannelToPort\0",
                "xlAddChannelToPort",
            )?
        };
        let xlFinalizePort =
            unsafe { load_symbol(&library, &path, b"xlFinalizePort\0", "xlFinalizePort")? };
        let xlSetNotification =
            unsafe { load_symbol(&library, &path, b"xlSetNotification\0", "xlSetNotification")? };
        let xlFlushReceiveQueue = unsafe {
            load_symbol(
                &library,
                &path,
                b"xlFlushReceiveQueue\0",
                "xlFlushReceiveQueue",
            )?
        };
        let xlGetReceiveQueueLevel = unsafe {
            load_symbol(
                &library,
                &path,
                b"xlGetReceiveQueueLevel\0",
                "xlGetReceiveQueueLevel",
            )?
        };
        let xlActivateChannel =
            unsafe { load_symbol(&library, &path, b"xlActivateChannel\0", "xlActivateChannel")? };
        let xlDeactivateChannel = unsafe {
            load_symbol(
                &library,
                &path,
                b"xlDeactivateChannel\0",
                "xlDeactivateChannel",
            )?
        };
        let xlClosePort = unsafe { load_symbol(&library, &path, b"xlClosePort\0", "xlClosePort")? };
        let xlGetErrorString =
            unsafe { load_symbol(&library, &path, b"xlGetErrorString\0", "xlGetErrorString")? };
        let xlGetEventString =
            unsafe { load_symbol(&library, &path, b"xlGetEventString\0", "xlGetEventString")? };
        let xlCanSetChannelOutput = unsafe {
            load_symbol(
                &library,
                &path,
                b"xlCanSetChannelOutput\0",
                "xlCanSetChannelOutput",
            )?
        };
        let xlCanSetChannelBitrate = unsafe {
            load_symbol(
                &library,
                &path,
                b"xlCanSetChannelBitrate\0",
                "xlCanSetChannelBitrate",
            )?
        };
        let xlCanTransmit =
            unsafe { load_symbol(&library, &path, b"xlCanTransmit\0", "xlCanTransmit")? };
        let xlReceive = unsafe { load_symbol(&library, &path, b"xlReceive\0", "xlReceive")? };

        Ok(Self {
            _library: library,
            xlOpenDriver,
            xlCloseDriver,
            xlGetApplConfig,
            xlSetApplConfig,
            xlGetDriverConfig,
            xlGetChannelIndex,
            xlGetChannelMask,
            xlOpenPort,
            xlCreatePort,
            xlAddChannelToPort,
            xlFinalizePort,
            xlSetNotification,
            xlFlushReceiveQueue,
            xlGetReceiveQueueLevel,
            xlActivateChannel,
            xlDeactivateChannel,
            xlClosePort,
            xlGetErrorString,
            xlGetEventString,
            xlCanSetChannelOutput,
            xlCanSetChannelBitrate,
            xlCanTransmit,
            xlReceive,
        })
    }

    pub fn load_default() -> Result<Self, XlApiLoadError> {
        Self::load(&LibraryLocation::default())
    }

    pub fn error_string(&self, status: XLstatus) -> String {
        let error_ptr = unsafe {
            // SAFETY: The function pointer was resolved from the loaded XL API
            // DLL with the exact signature declared in `vxlapi.h`.
            (self.xlGetErrorString)(status)
        };

        if error_ptr.is_null() {
            return format!("XL status {status}");
        }

        let error = unsafe {
            // SAFETY: `xlGetErrorString` returns a null-terminated string owned
            // by the XL API for the lifetime of the loaded library. We copy it
            // into an owned `String` before returning.
            CStr::from_ptr(error_ptr)
        };

        error.to_string_lossy().into_owned()
    }

    pub fn event_string(&self, event: &XLevent) -> String {
        let event_ptr = event as *const XLevent as *mut XLevent;
        let error_ptr = unsafe {
            // SAFETY: The function pointer was resolved from the loaded XL API
            // DLL with the exact signature declared in `vxlapi.h`. The driver
            // only reads the event to format a message string.
            (self.xlGetEventString)(event_ptr)
        };

        if error_ptr.is_null() {
            return format!("XL event tag {}", event.tag);
        }

        let error = unsafe {
            // SAFETY: `xlGetEventString` returns a null-terminated string owned
            // by the XL API for the lifetime of the loaded library. We copy it
            // into an owned `String` before returning.
            CStr::from_ptr(error_ptr)
        };

        error.to_string_lossy().into_owned()
    }
}

unsafe fn load_symbol<T: Copy>(
    library: &Library,
    path: &Path,
    symbol: &'static [u8],
    symbol_name: &'static str,
) -> Result<T, XlApiLoadError> {
    let loaded: Symbol<'_, T> = unsafe {
        // SAFETY: The symbol names are null-terminated and the type `T`
        // matches the corresponding `vxlapi.h` declaration.
        library.get(symbol)
    }
    .map_err(|error| XlApiLoadError::LoadSymbol {
        path: path.to_path_buf(),
        symbol: symbol_name,
        error: error.to_string(),
    })?;

    Ok(*loaded)
}

#[cfg(test)]
mod tests {
    use std::path::PathBuf;

    use crate::types::XL_SUCCESS;

    use super::{DEFAULT_XL_API_DLL, LibraryLocation, XlApi};

    const XL_DRIVER_SYS_TEST_DLL_ENV: &str = "XL_DRIVER_SYS_TEST_DLL";

    #[test]
    fn library_location_defaults_to_vendor_dll_name() {
        assert_eq!(
            LibraryLocation::default().as_path(),
            PathBuf::from(DEFAULT_XL_API_DLL).as_path()
        );
    }

    #[test]
    #[ignore = "requires XL_DRIVER_SYS_TEST_DLL or an installed vxlapi64.dll with runtime support"]
    fn smoke_loads_configured_or_default_dll_and_opens_driver() {
        let location = std::env::var_os(XL_DRIVER_SYS_TEST_DLL_ENV)
            .map(|path| LibraryLocation::new(PathBuf::from(path)))
            .unwrap_or_default();

        let api = XlApi::load(&location).unwrap_or_else(|error| {
            panic!(
                "smoke test could not load {}: {error}",
                location.as_path().display()
            )
        });

        let open_status = unsafe {
            // SAFETY: The symbol is resolved from the loaded DLL and takes no parameters.
            (api.xlOpenDriver)()
        };
        assert_eq!(open_status, XL_SUCCESS, "{}", api.error_string(open_status));

        let close_status = unsafe {
            // SAFETY: The symbol is resolved from the loaded DLL and takes no parameters.
            (api.xlCloseDriver)()
        };
        assert_eq!(
            close_status,
            XL_SUCCESS,
            "{}",
            api.error_string(close_status)
        );
    }
}
