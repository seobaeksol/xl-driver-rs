use xl_driver_sys::{LibraryLocation, XL_SUCCESS, XlApi};

use crate::error::XlError;

/// Process-wide XL Driver session.
///
/// A `Driver` owns the loaded XL API DLL handle and keeps the driver opened for
/// the lifetime of the value.
pub struct Driver {
    api: XlApi,
}

impl Driver {
    /// Loads the default `vxlapi64.dll` and opens the driver.
    pub fn open() -> Result<Self, XlError> {
        Self::open_with_location(LibraryLocation::default())
    }

    /// Loads the XL API DLL from the provided location and opens the driver.
    pub fn open_with_location(location: impl Into<LibraryLocation>) -> Result<Self, XlError> {
        let api = XlApi::load(&location.into())?;
        let status = unsafe {
            // SAFETY: The function pointer was resolved from the loaded DLL with
            // the signature declared in `vxlapi.h`.
            (api.xlOpenDriver)()
        };

        if status != XL_SUCCESS {
            return Err(XlError::from_status(status, api.error_string(status)));
        }

        Ok(Self { api })
    }
}

impl Drop for Driver {
    fn drop(&mut self) {
        let _ = unsafe {
            // SAFETY: The driver was opened successfully before `Driver` was
            // constructed, and the matching close entrypoint comes from the same DLL.
            (self.api.xlCloseDriver)()
        };
    }
}

#[cfg(test)]
mod tests {
    use crate::Driver;

    #[test]
    fn missing_dll_returns_loader_error() {
        let error = match Driver::open_with_location("this-file-does-not-exist-vxlapi64.dll") {
            Ok(_) => panic!("loading a missing DLL should fail"),
            Err(error) => error,
        };

        assert_eq!(error.code, None);
        assert!(error.message.contains("failed to load XL API DLL"));
    }
}
