use core::ffi::c_char;

use windows_sys::Win32::Foundation::HANDLE;

/// XL API status code as defined by `vxlapi.h`.
pub type XLstatus = i16;

/// XL API unsigned 64-bit integer as defined by `vxlapi.h`.
pub type XLuint64 = u64;

/// XL API channel access mask as defined by `vxlapi.h`.
pub type XLaccess = XLuint64;

/// XL API channel index mask type as defined by `vxlapi.h`.
pub type XLindex = XLuint64;

/// XL API signed long as defined by `vxlapi.h`.
///
/// Note: on Windows x64, C `long` remains 32-bit.
pub type XLlong = i32;

/// XL API unsigned long as defined by `vxlapi.h`.
///
/// Note: on Windows x64, C `unsigned long` remains 32-bit.
pub type XLulong = u32;

/// XL API port handle as defined by `vxlapi.h`.
pub type XLportHandle = XLlong;

/// XL API notification handle.
pub type XLhandle = HANDLE;

/// Borrowed C string returned by the XL API.
pub type XLstringType = *mut c_char;

pub const XL_SUCCESS: XLstatus = 0;
pub const XL_ERR_QUEUE_IS_EMPTY: XLstatus = 10;

pub const XL_BUS_TYPE_NONE: u32 = 0x0000_0000;
pub const XL_BUS_TYPE_CAN: u32 = 0x0000_0001;

pub const XL_INTERFACE_VERSION_V2: u32 = 2;
pub const XL_INTERFACE_VERSION_V3: u32 = 3;
pub const XL_INTERFACE_VERSION_V4: u32 = 4;
pub const XL_INTERFACE_VERSION: u32 = XL_INTERFACE_VERSION_V3;

pub const XL_ACTIVATE_NONE: u32 = 0;
pub const XL_ACTIVATE_RESET_CLOCK: u32 = 8;

pub const XL_MAX_APPNAME: usize = 32;
pub const XL_MAX_LENGTH: usize = 31;
pub const XL_CONFIG_MAX_CHANNELS: usize = 64;

pub const XL_INVALID_PORTHANDLE: XLportHandle = -1;

/// Raw CAN FD configuration passed to `xlCanFdSetConfiguration`.
///
/// `vxlapi.h` places this type inside a `#pragma pack(1)` region, but the field
/// order and fixed-width members produce the same field offsets and total size
/// under `#[repr(C)]`, which keeps the type ergonomic to use in safe wrappers.
#[repr(C)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct XLcanFdConf {
    pub arbitrationBitRate: u32,
    pub sjwAbr: u32,
    pub tseg1Abr: u32,
    pub tseg2Abr: u32,
    pub dataBitRate: u32,
    pub sjwDbr: u32,
    pub tseg1Dbr: u32,
    pub tseg2Dbr: u32,
    pub reserved: u8,
    pub options: u8,
    pub reserved1: [u8; 2],
    pub reserved2: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLbusParamsCan {
    pub bitRate: u32,
    pub sjw: u8,
    pub tseg1: u8,
    pub tseg2: u8,
    pub sam: u8,
    pub outputMode: u8,
    pub reserved1: [u8; 7],
    pub canOpMode: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLbusParamsCanFd {
    pub arbitrationBitRate: u32,
    pub sjwAbr: u8,
    pub tseg1Abr: u8,
    pub tseg2Abr: u8,
    pub samAbr: u8,
    pub outputMode: u8,
    pub sjwDbr: u8,
    pub tseg1Dbr: u8,
    pub tseg2Dbr: u8,
    pub dataBitRate: u32,
    pub canOpMode: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLbusParamsMost {
    pub activeSpeedGrade: u32,
    pub compatibleSpeedGrade: u32,
    pub inicFwVersion: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLbusParamsFlexRay {
    pub status: u32,
    pub cfgMode: u32,
    pub baudrate: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLbusParamsEthernet {
    pub macAddr: [u8; 6],
    pub connector: u8,
    pub phy: u8,
    pub link: u8,
    pub speed: u8,
    pub clockMode: u8,
    pub bypass: u8,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLa429TxParameters {
    pub bitrate: u32,
    pub parity: u32,
    pub minGap: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLa429RxParameters {
    pub bitrate: u32,
    pub minBitrate: u32,
    pub maxBitrate: u32,
    pub parity: u32,
    pub minGap: u32,
    pub autoBaudrate: u32,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union XLa429Direction {
    pub tx: XLa429TxParameters,
    pub rx: XLa429RxParameters,
    pub raw: [u8; 24],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLa429BusParams {
    pub channelDirection: u16,
    pub res1: u16,
    pub dir: XLa429Direction,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union XLbusParamsData {
    pub can: XLbusParamsCan,
    pub canFD: XLbusParamsCanFd,
    pub most: XLbusParamsMost,
    pub flexray: XLbusParamsFlexRay,
    pub ethernet: XLbusParamsEthernet,
    pub a429: XLa429BusParams,
    pub raw: [u8; 28],
}

/// Packed bus parameter block returned by `xlGetDriverConfig`.
#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct XLbusParams {
    pub busType: u32,
    pub data: XLbusParamsData,
}

/// Packed channel description returned by `xlGetDriverConfig`.
#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct XLchannelConfig {
    pub name: [c_char; XL_MAX_LENGTH + 1],
    pub hwType: u8,
    pub hwIndex: u8,
    pub hwChannel: u8,
    pub transceiverType: u16,
    pub transceiverState: u16,
    pub configError: u16,
    pub channelIndex: u8,
    pub channelMask: XLuint64,
    pub channelCapabilities: u32,
    pub channelBusCapabilities: u32,
    pub isOnBus: u8,
    pub connectedBusType: u32,
    pub busParams: XLbusParams,
    pub _doNotUse: u32,
    pub driverVersion: u32,
    pub interfaceVersion: u32,
    pub raw_data: [u32; 10],
    pub serialNumber: u32,
    pub articleNumber: u32,
    pub transceiverName: [c_char; XL_MAX_LENGTH + 1],
    pub specialCabFlags: u32,
    pub dominantTimeout: u32,
    pub dominantRecessiveDelay: u8,
    pub recessiveDominantDelay: u8,
    pub connectionInfo: u8,
    pub currentlyAvailableTimestamps: u8,
    pub minimalSupplyVoltage: u16,
    pub maximalSupplyVoltage: u16,
    pub maximalBaudrate: u32,
    pub fpgaCoreCapabilities: u8,
    pub specialDeviceStatus: u8,
    pub channelBusActiveCapabilities: u16,
    pub breakOffset: u16,
    pub delimiterOffset: u16,
    pub reserved: [u32; 3],
}

/// Packed driver configuration returned by `xlGetDriverConfig`.
#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct XLdriverConfig {
    pub dllVersion: u32,
    pub channelCount: u32,
    pub reserved: [u32; 10],
    pub channel: [XLchannelConfig; XL_CONFIG_MAX_CHANNELS],
}

#[cfg(test)]
mod tests {
    use core::mem::{align_of, offset_of, size_of};

    use super::{
        XL_CONFIG_MAX_CHANNELS, XL_SUCCESS, XLaccess, XLbusParams, XLcanFdConf, XLchannelConfig,
        XLdriverConfig, XLhandle, XLportHandle, XLstatus,
    };

    #[test]
    fn core_widths_match_vxlapi_header() {
        assert_eq!(XL_SUCCESS, 0);
        assert_eq!(size_of::<XLstatus>(), 2);
        assert_eq!(size_of::<XLaccess>(), 8);
        assert_eq!(size_of::<XLportHandle>(), 4);
        assert_eq!(size_of::<XLhandle>(), 8);
    }

    #[test]
    fn canfd_config_layout_matches_phase1_expectations() {
        assert_eq!(size_of::<XLcanFdConf>(), 40);
        assert_eq!(offset_of!(XLcanFdConf, reserved2), 36);
    }

    #[test]
    fn driver_config_layout_matches_vxlapi_header() {
        assert_eq!(XL_CONFIG_MAX_CHANNELS, 64);
        assert_eq!(size_of::<XLbusParams>(), 32);
        assert_eq!(align_of::<XLchannelConfig>(), 1);
        assert_eq!(offset_of!(XLchannelConfig, channelMask), 42);
        assert_eq!(size_of::<XLchannelConfig>(), 227);
        assert_eq!(offset_of!(XLdriverConfig, channel), 48);
        assert_eq!(size_of::<XLdriverConfig>(), 14_576);
    }
}
