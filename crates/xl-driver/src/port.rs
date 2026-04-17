use std::cell::Cell;
use std::ffi::CString;
use std::io;
use std::ptr;
use std::sync::Arc;
use std::time::Duration;

use windows_sys::Win32::Foundation::{WAIT_FAILED, WAIT_OBJECT_0, WAIT_TIMEOUT};
use windows_sys::Win32::System::Threading::WaitForSingleObject;
use xl_driver_sys::can::XLevent;
use xl_driver_sys::{
    XL_ACTIVATE_RESET_CLOCK, XL_BUS_TYPE_CAN, XL_ERR_QUEUE_IS_EMPTY, XL_INTERFACE_VERSION,
    XL_INVALID_PORTHANDLE, XLhandle, XLportHandle,
};

use crate::can::{
    CanEvent, CanFrame, CanOutputMode, CanQueueSize, event_from_frame, event_to_can_event,
};
use crate::driver::{DriverInner, access_mask_from_channel_indices};
use crate::error::XlError;

/// Open classic CAN port owned by the safe wrapper layer.
pub struct Port {
    driver: Arc<DriverInner>,
    handle: XLportHandle,
    access_mask: u64,
    permission_mask: u64,
    notification_handle: Cell<XLhandle>,
}

impl Port {
    pub(crate) fn open_can(
        driver: Arc<DriverInner>,
        user_name: &str,
        channel_indices: &[u32],
        bitrate: u32,
        queue_size: CanQueueSize,
    ) -> Result<Self, XlError> {
        if bitrate == 0 {
            return Err(XlError::new(None, "CAN bitrate must be greater than zero"));
        }

        let access_mask = access_mask_from_channel_indices(channel_indices)?;
        let queue_size = queue_size_value(queue_size);
        if queue_size == 0 {
            return Err(XlError::new(
                None,
                "CAN receive queue size must be greater than zero",
            ));
        }
        let mut port_handle = XL_INVALID_PORTHANDLE;
        let mut permission_mask = 0_u64;
        let user_name = CString::new(user_name)
            .map_err(|_| XlError::new(None, "CAN user name must not contain interior NUL bytes"))?;

        let status = unsafe {
            // SAFETY: All pointers are valid for the duration of the call, and
            // the signature matches `vxlapi.h`.
            (driver.api.xlOpenPort)(
                &mut port_handle,
                user_name.as_ptr().cast_mut(),
                access_mask,
                &mut permission_mask,
                queue_size,
                XL_INTERFACE_VERSION,
                XL_BUS_TYPE_CAN,
            )
        };
        driver.status_result(status)?;

        if permission_mask & access_mask != access_mask {
            close_raw_port(&driver, port_handle);
            return Err(XlError::new(
                None,
                format!(
                    "CAN port opened without init access for all requested channels (requested=0x{access_mask:016X}, permission=0x{permission_mask:016X})"
                ),
            ));
        }

        let port = Self {
            driver,
            handle: port_handle,
            access_mask,
            permission_mask,
            notification_handle: Cell::new(ptr::null_mut()),
        };

        if let Err(error) = port.set_bitrate(bitrate) {
            close_raw_port(&port.driver, port.handle);
            return Err(error);
        }

        Ok(port)
    }

    /// Returns the access mask associated with the opened port.
    pub fn access_mask(&self) -> u64 {
        self.access_mask
    }

    /// Activates all channels on this CAN port.
    pub fn activate(&self) -> Result<(), XlError> {
        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive.
            (self.driver.api.xlActivateChannel)(
                self.handle,
                self.access_mask,
                XL_BUS_TYPE_CAN,
                XL_ACTIVATE_RESET_CLOCK,
            )
        };
        self.driver.status_result(status)
    }

    /// Deactivates all channels on this CAN port.
    pub fn deactivate(&self) -> Result<(), XlError> {
        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive.
            (self.driver.api.xlDeactivateChannel)(self.handle, self.access_mask)
        };
        self.driver.status_result(status)
    }

    /// Sets the classic CAN bitrate for all channels opened on this port.
    pub fn set_bitrate(&self, bitrate: u32) -> Result<(), XlError> {
        if bitrate == 0 {
            return Err(XlError::new(None, "CAN bitrate must be greater than zero"));
        }
        if self.permission_mask & self.access_mask != self.access_mask {
            return Err(XlError::new(
                None,
                "CAN bitrate configuration requires init access for all opened channels",
            ));
        }

        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive, and the
            // access mask refers only to channels opened on this port.
            (self.driver.api.xlCanSetChannelBitrate)(self.handle, self.access_mask, bitrate)
        };
        self.driver.status_result(status)
    }

    /// Sets the output mode for all channels opened on this port.
    pub fn set_output_mode(&self, output_mode: CanOutputMode) -> Result<(), XlError> {
        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive, and the
            // access mask refers only to channels opened on this port.
            (self.driver.api.xlCanSetChannelOutput)(
                self.handle,
                self.access_mask,
                output_mode.into(),
            )
        };
        self.driver.status_result(status)
    }

    /// Flushes the receive queue of this port.
    pub fn flush_receive_queue(&self) -> Result<(), XlError> {
        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive.
            (self.driver.api.xlFlushReceiveQueue)(self.handle)
        };
        self.driver.status_result(status)
    }

    /// Returns the number of queued classic CAN events for this port.
    pub fn receive_queue_level(&self) -> Result<i32, XlError> {
        let mut level = 0_i32;
        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive and `level`
            // points to writable stack storage.
            (self.driver.api.xlGetReceiveQueueLevel)(self.handle, &mut level)
        };
        self.driver.status_result(status)?;
        Ok(level)
    }

    /// Configures notification so receive waits can block on the driver event.
    pub fn set_notification(&self) -> Result<(), XlError> {
        self.set_notification_level(1)
    }

    /// Configures notification with an explicit queue threshold.
    pub fn set_notification_level(&self, queue_level: i32) -> Result<(), XlError> {
        if queue_level <= 0 {
            return Err(XlError::new(
                None,
                "CAN notification queue level must be greater than zero",
            ));
        }

        let mut notification_handle = ptr::null_mut();
        let status = unsafe {
            // SAFETY: The port handle is valid while `self` is alive and
            // `notification_handle` points to writable stack storage.
            (self.driver.api.xlSetNotification)(self.handle, &mut notification_handle, queue_level)
        };
        self.driver.status_result(status)?;
        self.notification_handle.set(notification_handle);
        Ok(())
    }

    /// Transmits one classic CAN frame on the channels associated with this port.
    pub fn transmit(&self, frame: CanFrame) -> Result<(), XlError> {
        if frame.dlc > 8 {
            return Err(XlError::new(
                None,
                format!(
                    "classic CAN DLC {} exceeds the 8-byte payload limit",
                    frame.dlc
                ),
            ));
        }

        let mut event_count = 1_u32;
        let mut event = event_from_frame(frame);
        let status = unsafe {
            // SAFETY: The port handle is valid, `event_count` and `event` point
            // to writable stack storage, and the call signature matches `vxlapi.h`.
            (self.driver.api.xlCanTransmit)(
                self.handle,
                self.access_mask,
                &mut event_count,
                (&mut event as *mut XLevent).cast(),
            )
        };
        self.driver.status_result(status)
    }

    /// Receives one CAN event without blocking if no notification handle is configured.
    pub fn recv(&self) -> Result<Option<CanEvent>, XlError> {
        self.recv_timeout(Duration::from_millis(0))
    }

    /// Waits up to `timeout` for a CAN event when notifications are configured.
    pub fn recv_timeout(&self, timeout: Duration) -> Result<Option<CanEvent>, XlError> {
        if !self.wait_for_notification(timeout)? {
            return Ok(None);
        }

        let mut event_count = 1_u32;
        let mut event = XLevent::default();
        let status = unsafe {
            // SAFETY: The port handle is valid, `event_count` and `event` point
            // to writable stack storage, and the call signature matches `vxlapi.h`.
            (self.driver.api.xlReceive)(self.handle, &mut event_count, &mut event)
        };

        if status == XL_ERR_QUEUE_IS_EMPTY {
            return Ok(None);
        }

        self.driver.status_result(status)?;
        Ok(Some(event_to_can_event(&self.driver.api, &event)))
    }

    fn wait_for_notification(&self, timeout: Duration) -> Result<bool, XlError> {
        let notification_handle = self.notification_handle.get();
        if notification_handle.is_null() {
            return Ok(true);
        }

        let timeout_ms = duration_to_wait_millis(timeout);
        let wait_status = unsafe {
            // SAFETY: `notification_handle` comes from `xlSetNotification`, and
            // `WaitForSingleObject` only borrows it for the duration of the call.
            WaitForSingleObject(notification_handle, timeout_ms)
        };

        match wait_status {
            WAIT_OBJECT_0 => Ok(true),
            WAIT_TIMEOUT => Ok(false),
            WAIT_FAILED => Err(XlError::new(
                None,
                format!(
                    "waiting for CAN notification failed: {}",
                    io::Error::last_os_error()
                ),
            )),
            other => Err(XlError::new(
                None,
                format!("unexpected CAN notification wait result {other}"),
            )),
        }
    }
}

impl Drop for Port {
    fn drop(&mut self) {
        close_raw_port(&self.driver, self.handle);
    }
}

fn duration_to_wait_millis(timeout: Duration) -> u32 {
    timeout.as_millis().try_into().unwrap_or(u32::MAX)
}

fn queue_size_value(queue_size: CanQueueSize) -> u32 {
    match queue_size {
        CanQueueSize::Events(count) => count,
    }
}

fn close_raw_port(driver: &DriverInner, handle: XLportHandle) {
    if handle == XL_INVALID_PORTHANDLE {
        return;
    }

    let _ = unsafe {
        // SAFETY: The handle was previously returned by `xlOpenPort`, and this
        // function is only used to pair a best-effort close with that open call.
        (driver.api.xlClosePort)(handle)
    };
}

#[cfg(test)]
mod tests {
    use crate::can::{CanFrame, CanQueueSize};

    use super::{duration_to_wait_millis, queue_size_value};

    #[test]
    fn queue_size_uses_event_count() {
        assert_eq!(queue_size_value(CanQueueSize::Events(4096)), 4096);
    }

    #[test]
    fn wait_timeout_clamps_large_durations() {
        assert_eq!(
            duration_to_wait_millis(std::time::Duration::from_millis(25)),
            25
        );
        assert_eq!(duration_to_wait_millis(std::time::Duration::MAX), u32::MAX);
    }

    #[test]
    fn can_frame_flags_remote_frames() {
        let frame = CanFrame {
            id: 0x123,
            flags: xl_driver_sys::XL_CAN_MSG_FLAG_REMOTE_FRAME,
            dlc: 0,
            data: [0; 8],
        };

        assert!(frame.is_remote_frame());
    }
}
