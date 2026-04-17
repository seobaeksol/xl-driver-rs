pub use xl_driver_sys::canfd::XLcanFdConf;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CanFdQueueSize {
    Bytes(u32),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CanFdConfig {
    pub raw: XLcanFdConf,
}

impl From<XLcanFdConf> for CanFdConfig {
    fn from(raw: XLcanFdConf) -> Self {
        Self { raw }
    }
}
