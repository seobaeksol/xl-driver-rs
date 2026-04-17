#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CanQueueSize {
    Events(u32),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CanFrame {
    pub id: u32,
    pub dlc: u8,
    pub data: [u8; 8],
}
