use std::sync::Arc;

use xl_driver_sys::{LibraryLocation, XL_SUCCESS, XLaccess, XLstatus, XlApi};

use crate::can::CanQueueSize;
use crate::error::XlError;
use crate::port::Port;

pub(crate) struct DriverInner {
    pub api: XlApi,
}

impl DriverInner {
    pub fn error_from_status(&self, status: XLstatus) -> XlError {
        XlError::from_status(status, self.api.error_string(status))
    }

    pub fn status_result(&self, status: XLstatus) -> Result<(), XlError> {
        if status == XL_SUCCESS {
            Ok(())
        } else {
            Err(self.error_from_status(status))
        }
    }
}

impl Drop for DriverInner {
    fn drop(&mut self) {
        let _ = unsafe {
            // SAFETY: The driver was opened successfully before `DriverInner`
            // was constructed, and the close entrypoint comes from the same DLL.
            (self.api.xlCloseDriver)()
        };
    }
}

/// Process-wide XL Driver session.
///
/// `Driver` keeps the XL API loaded and the driver opened for as long as any
/// derived port object still exists.
#[derive(Clone)]
pub struct Driver {
    inner: Arc<DriverInner>,
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

        Ok(Self {
            inner: Arc::new(DriverInner { api }),
        })
    }

    /// Opens a classic CAN port with the default receive queue size.
    pub fn open_can_port(
        &self,
        user_name: &str,
        channel_indices: &[u32],
        bitrate: u32,
    ) -> Result<Port, XlError> {
        self.open_can_port_with_queue(
            user_name,
            channel_indices,
            bitrate,
            CanQueueSize::Events(4096),
        )
    }

    /// Opens a classic CAN port with an explicit receive queue size.
    pub fn open_can_port_with_queue(
        &self,
        user_name: &str,
        channel_indices: &[u32],
        bitrate: u32,
        queue_size: CanQueueSize,
    ) -> Result<Port, XlError> {
        Port::open_can(
            Arc::clone(&self.inner),
            user_name,
            channel_indices,
            bitrate,
            queue_size,
        )
    }
}

pub(crate) fn access_mask_from_channel_indices(
    channel_indices: &[u32],
) -> Result<XLaccess, XlError> {
    if channel_indices.is_empty() {
        return Err(XlError::new(
            None,
            "at least one CAN channel index is required",
        ));
    }

    let mut access_mask = 0_u64;
    for &channel_index in channel_indices {
        if channel_index >= 64 {
            return Err(XlError::new(
                None,
                format!("CAN channel index {channel_index} exceeds the 64-channel mask limit"),
            ));
        }
        access_mask |= 1_u64 << channel_index;
    }

    Ok(access_mask)
}

#[cfg(test)]
mod tests {
    use super::{Driver, access_mask_from_channel_indices};

    #[test]
    fn access_mask_rejects_empty_channel_list() {
        let error =
            access_mask_from_channel_indices(&[]).expect_err("empty channel list should fail");
        assert_eq!(error.code, None);
    }

    #[test]
    fn access_mask_builds_bitmask_from_channel_indices() {
        let access_mask = access_mask_from_channel_indices(&[0, 3, 5]).expect("mask should build");
        assert_eq!(access_mask, 0b10_1001);
    }

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
