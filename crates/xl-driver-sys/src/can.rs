use crate::types::XLuint64;

pub use crate::types::XL_BUS_TYPE_CAN;

pub type XLeventTag = u8;

pub const MAX_MSG_LEN: usize = 8;

pub const XL_RECEIVE_MSG: XLeventTag = 1;
pub const XL_TRANSMIT_MSG: XLeventTag = 10;

pub const XL_USE_ALL_CHANNELS: XLuint64 = 0xFFFF_FFFF_FFFF_FFFF;

pub const XL_CAN_EXT_MSG_ID: u32 = 0x8000_0000;

pub const XL_CAN_MSG_FLAG_REMOTE_FRAME: u16 = 0x10;
pub const XL_CAN_MSG_FLAG_TX_COMPLETED: u16 = 0x40;

pub const XL_EVENT_FLAG_OVERRUN: u8 = 0x01;

pub const XL_OUTPUT_MODE_SILENT: i32 = 0;
pub const XL_OUTPUT_MODE_NORMAL: i32 = 1;
pub const XL_OUTPUT_MODE_TX_OFF: i32 = 2;
pub const XL_OUTPUT_MODE_SJA_1000_SILENT: i32 = 3;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct XLcanMsg {
    pub id: u32,
    pub flags: u16,
    pub dlc: u16,
    pub res1: u64,
    pub data: [u8; MAX_MSG_LEN],
    pub res2: u64,
}

impl Default for XLcanMsg {
    fn default() -> Self {
        Self {
            id: 0,
            flags: 0,
            dlc: 0,
            res1: 0,
            data: [0; MAX_MSG_LEN],
            res2: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union XLeventTagData {
    pub msg: XLcanMsg,
    pub raw: [u8; 32],
}

impl Default for XLeventTagData {
    fn default() -> Self {
        Self { raw: [0; 32] }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct XLevent {
    pub tag: XLeventTag,
    pub chanIndex: u8,
    pub transId: u16,
    pub portHandle: u16,
    pub flags: u8,
    pub reserved: u8,
    pub timeStamp: u64,
    pub tagData: XLeventTagData,
}

#[cfg(test)]
mod tests {
    use core::mem::{offset_of, size_of};

    use super::{MAX_MSG_LEN, XL_EVENT_FLAG_OVERRUN, XLcanMsg, XLevent, XLeventTagData};

    #[test]
    fn classic_can_layout_matches_header() {
        assert_eq!(MAX_MSG_LEN, 8);
        assert_eq!(XL_EVENT_FLAG_OVERRUN, 0x01);
        assert_eq!(size_of::<XLcanMsg>(), 32);
        assert_eq!(size_of::<XLeventTagData>(), 32);
        assert_eq!(offset_of!(XLevent, timeStamp), 8);
        assert_eq!(offset_of!(XLevent, tagData), 16);
        assert_eq!(size_of::<XLevent>(), 48);
    }
}
