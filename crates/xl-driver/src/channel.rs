use core::ffi::c_char;
use std::ffi::CStr;
use std::ptr;

use xl_driver_sys::{
    XL_BUS_ACTIVE_CAP_CAN, XL_BUS_COMPATIBLE_CAN, XL_BUS_TYPE_CAN,
    XL_CHANNEL_FLAG_EX1_CANFD_ISO_SUPPORT, XL_HWTYPE_VIRTUAL, XLchannelConfig,
};

/// Snapshot of one XL driver channel entry.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ChannelInfo {
    pub name: String,
    pub transceiver_name: String,
    pub channel_index: u32,
    pub channel_mask: u64,
    pub hw_type: u32,
    pub hw_index: u32,
    pub hw_channel: u32,
    pub channel_capabilities: u32,
    pub bus_capabilities: u32,
    pub bus_active_capabilities: u16,
    pub connected_bus_type: u32,
    pub is_on_bus: bool,
    pub serial_number: u32,
    pub article_number: u32,
}

impl ChannelInfo {
    /// Returns whether the channel can participate in classic CAN workflows.
    pub fn supports_can(&self) -> bool {
        self.connected_bus_type == XL_BUS_TYPE_CAN
            || (self.bus_capabilities & XL_BUS_COMPATIBLE_CAN) != 0
            || (self.bus_capabilities & XL_BUS_ACTIVE_CAP_CAN) != 0
            || (u32::from(self.bus_active_capabilities) & XL_BUS_COMPATIBLE_CAN) != 0
    }

    /// Returns whether the channel advertises CAN FD ISO capability.
    pub fn supports_can_fd(&self) -> bool {
        self.supports_can()
            && (self.channel_capabilities & XL_CHANNEL_FLAG_EX1_CANFD_ISO_SUPPORT) != 0
    }

    /// Returns whether the hardware is the Vector virtual device.
    pub fn is_virtual(&self) -> bool {
        self.hw_type == u32::from(XL_HWTYPE_VIRTUAL)
    }

    pub(crate) fn from_raw(raw: XLchannelConfig) -> Self {
        Self {
            name: read_c_string(ptr::addr_of!(raw.name)),
            transceiver_name: read_c_string(ptr::addr_of!(raw.transceiverName)),
            channel_index: u32::from(read_packed(ptr::addr_of!(raw.channelIndex))),
            channel_mask: read_packed(ptr::addr_of!(raw.channelMask)),
            hw_type: u32::from(read_packed(ptr::addr_of!(raw.hwType))),
            hw_index: u32::from(read_packed(ptr::addr_of!(raw.hwIndex))),
            hw_channel: u32::from(read_packed(ptr::addr_of!(raw.hwChannel))),
            channel_capabilities: read_packed(ptr::addr_of!(raw.channelCapabilities)),
            bus_capabilities: read_packed(ptr::addr_of!(raw.channelBusCapabilities)),
            bus_active_capabilities: read_packed(ptr::addr_of!(raw.channelBusActiveCapabilities)),
            connected_bus_type: read_packed(ptr::addr_of!(raw.connectedBusType)),
            is_on_bus: read_packed(ptr::addr_of!(raw.isOnBus)) != 0,
            serial_number: read_packed(ptr::addr_of!(raw.serialNumber)),
            article_number: read_packed(ptr::addr_of!(raw.articleNumber)),
        }
    }
}

fn read_packed<T: Copy>(value: *const T) -> T {
    unsafe {
        // SAFETY: Callers pass field pointers into packed XL structs, so reads
        // must be unaligned and copied by value.
        ptr::read_unaligned(value)
    }
}

fn read_c_string<const N: usize>(value: *const [c_char; N]) -> String {
    let bytes = read_packed(value);
    let ptr = bytes.as_ptr();
    unsafe {
        // SAFETY: XL driver channel strings are fixed-size null-terminated C
        // strings. The copied array lives for this call, and we convert it into
        // an owned Rust `String` immediately.
        CStr::from_ptr(ptr)
    }
    .to_string_lossy()
    .into_owned()
}

#[cfg(test)]
mod tests {
    use xl_driver_sys::XLchannelConfig;

    use super::ChannelInfo;

    #[test]
    fn raw_channel_config_is_converted_to_owned_metadata() {
        let mut raw = XLchannelConfig {
            channelIndex: 7,
            channelMask: 1_u64 << 7,
            hwType: 2,
            hwIndex: 3,
            hwChannel: 1,
            channelCapabilities: xl_driver_sys::XL_CHANNEL_FLAG_EX1_CANFD_ISO_SUPPORT,
            channelBusCapabilities: xl_driver_sys::XL_BUS_ACTIVE_CAP_CAN,
            connectedBusType: xl_driver_sys::XL_BUS_TYPE_CAN,
            isOnBus: 1,
            serialNumber: 42,
            articleNumber: 99,
            channelBusActiveCapabilities: xl_driver_sys::XL_BUS_TYPE_CAN as u16,
            ..unsafe {
                // SAFETY: Zero initialization is valid for the remaining raw
                // metadata fields used only as inert placeholders in this test.
                std::mem::zeroed()
            }
        };

        raw.name[..4].copy_from_slice(&[b'C' as i8, b'A' as i8, b'N' as i8, 0]);
        raw.transceiverName[..4].copy_from_slice(&[b'V' as i8, b'N' as i8, b'1' as i8, 0]);

        let info = ChannelInfo::from_raw(raw);
        assert_eq!(info.name, "CAN");
        assert_eq!(info.transceiver_name, "VN1");
        assert_eq!(info.channel_index, 7);
        assert_eq!(info.channel_mask, 1_u64 << 7);
        assert!(info.supports_can());
        assert!(info.supports_can_fd());
        assert!(info.is_on_bus);
    }
}
