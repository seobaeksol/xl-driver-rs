use std::mem::MaybeUninit;
use std::ptr;
use std::sync::Arc;

use xl_driver_sys::{
    LibraryLocation, XL_CONFIG_MAX_CHANNELS, XL_SUCCESS, XLaccess, XLdriverConfig, XLstatus, XlApi,
};

use crate::can::CanQueueSize;
use crate::channel::ChannelInfo;
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

    pub fn driver_config(&self) -> Result<XLdriverConfig, XlError> {
        let mut config = MaybeUninit::<XLdriverConfig>::zeroed();
        let status = unsafe {
            // SAFETY: `xlGetDriverConfig` writes a complete `XLdriverConfig`
            // into the provided pointer on success.
            (self.api.xlGetDriverConfig)(config.as_mut_ptr())
        };
        self.status_result(status)?;

        Ok(unsafe {
            // SAFETY: The XL API call above reported success, so the config was
            // fully initialized by the driver.
            config.assume_init()
        })
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

    /// Returns the current XL driver channel list snapshot.
    pub fn channels(&self) -> Result<Vec<ChannelInfo>, XlError> {
        let config = self.inner.driver_config()?;
        Ok(channel_infos_from_driver_config(&config))
    }

    /// Returns the subset of channels that can participate in CAN workflows.
    pub fn can_channels(&self) -> Result<Vec<ChannelInfo>, XlError> {
        Ok(self
            .channels()?
            .into_iter()
            .filter(ChannelInfo::supports_can)
            .collect())
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
    driver: &DriverInner,
    channel_indices: &[u32],
) -> Result<XLaccess, XlError> {
    if channel_indices.is_empty() {
        return Err(XlError::new(
            None,
            "at least one CAN channel index is required",
        ));
    }

    let channels = channel_infos_from_driver_config(&driver.driver_config()?);
    let mut access_mask = 0_u64;
    for &channel_index in channel_indices {
        let channel = channels
            .iter()
            .find(|candidate| candidate.channel_index == channel_index)
            .ok_or_else(|| {
                XlError::new(None, format!("unknown CAN channel index {channel_index}"))
            })?;

        if !channel.supports_can() {
            return Err(XlError::new(
                None,
                format!("channel index {channel_index} is not CAN-capable"),
            ));
        }

        access_mask |= channel.channel_mask;
    }

    Ok(access_mask)
}

fn channel_infos_from_driver_config(config: &XLdriverConfig) -> Vec<ChannelInfo> {
    let channel_count = unsafe { ptr::addr_of!(config.channelCount).read_unaligned() as usize }
        .min(XL_CONFIG_MAX_CHANNELS);

    let mut channels = Vec::with_capacity(channel_count);
    for index in 0..channel_count {
        let raw = unsafe {
            // SAFETY: `config.channel` contains `channelCount` packed entries.
            ptr::addr_of!(config.channel[index]).read_unaligned()
        };
        channels.push(ChannelInfo::from_raw(raw));
    }
    channels
}

#[cfg(test)]
mod tests {
    use super::Driver;
    use crate::channel::ChannelInfo;

    #[test]
    fn access_mask_rejects_empty_channel_list() {
        let error =
            access_mask_from_channels(&[], &[]).expect_err("empty channel list should fail");
        assert_eq!(error.code, None);
    }

    #[test]
    fn access_mask_builds_mask_from_resolved_channels() {
        let channels = vec![
            ChannelInfo {
                name: "CAN 3".into(),
                transceiver_name: "VN".into(),
                channel_index: 3,
                channel_mask: 0x100,
                hw_type: 0,
                hw_index: 0,
                hw_channel: 0,
                channel_capabilities: 0,
                bus_capabilities: xl_driver_sys::XL_BUS_ACTIVE_CAP_CAN,
                bus_active_capabilities: xl_driver_sys::XL_BUS_TYPE_CAN as u16,
                connected_bus_type: xl_driver_sys::XL_BUS_TYPE_CAN,
                is_on_bus: true,
                serial_number: 1,
                article_number: 1,
            },
            ChannelInfo {
                name: "CAN 5".into(),
                transceiver_name: "VN".into(),
                channel_index: 5,
                channel_mask: 0x400,
                hw_type: 0,
                hw_index: 0,
                hw_channel: 1,
                channel_capabilities: 0,
                bus_capabilities: xl_driver_sys::XL_BUS_ACTIVE_CAP_CAN,
                bus_active_capabilities: xl_driver_sys::XL_BUS_TYPE_CAN as u16,
                connected_bus_type: xl_driver_sys::XL_BUS_TYPE_CAN,
                is_on_bus: true,
                serial_number: 2,
                article_number: 2,
            },
        ];

        let access_mask = access_mask_from_channels(&channels, &[3, 5]).expect("mask should build");
        assert_eq!(access_mask, 0x500);
    }

    #[test]
    fn access_mask_rejects_unknown_channel_index() {
        let channels = vec![ChannelInfo {
            name: "CAN 3".into(),
            transceiver_name: "VN".into(),
            channel_index: 3,
            channel_mask: 0x100,
            hw_type: 0,
            hw_index: 0,
            hw_channel: 0,
            channel_capabilities: 0,
            bus_capabilities: xl_driver_sys::XL_BUS_ACTIVE_CAP_CAN,
            bus_active_capabilities: xl_driver_sys::XL_BUS_TYPE_CAN as u16,
            connected_bus_type: xl_driver_sys::XL_BUS_TYPE_CAN,
            is_on_bus: true,
            serial_number: 1,
            article_number: 1,
        }];

        let error = access_mask_from_channels(&channels, &[9])
            .expect_err("unknown channel index should fail");
        assert!(error.message.contains("unknown CAN channel index 9"));
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

    fn access_mask_from_channels(
        channels: &[ChannelInfo],
        channel_indices: &[u32],
    ) -> Result<xl_driver_sys::XLaccess, crate::XlError> {
        if channel_indices.is_empty() {
            return Err(crate::XlError::new(
                None,
                "at least one CAN channel index is required",
            ));
        }

        let mut access_mask = 0_u64;
        for &channel_index in channel_indices {
            let channel = channels
                .iter()
                .find(|candidate| candidate.channel_index == channel_index)
                .ok_or_else(|| {
                    crate::XlError::new(None, format!("unknown CAN channel index {channel_index}"))
                })?;

            if !channel.supports_can() {
                return Err(crate::XlError::new(
                    None,
                    format!("channel index {channel_index} is not CAN-capable"),
                ));
            }

            access_mask |= channel.channel_mask;
        }

        Ok(access_mask)
    }
}
