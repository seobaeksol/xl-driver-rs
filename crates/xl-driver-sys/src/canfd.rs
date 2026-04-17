pub const CANFD_CONFOPT_NO_ISO: u8 = 0x08;

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
