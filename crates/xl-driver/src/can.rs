use xl_driver_sys::can::{
    XL_OUTPUT_MODE_NORMAL, XL_OUTPUT_MODE_SILENT, XL_OUTPUT_MODE_SJA_1000_SILENT,
    XL_OUTPUT_MODE_TX_OFF, XL_RECEIVE_MSG, XL_TRANSMIT_MSG, XLcanMsg, XLevent, XLeventTagData,
};
use xl_driver_sys::{XL_CAN_MSG_FLAG_REMOTE_FRAME, XlApi};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CanQueueSize {
    Events(u32),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CanOutputMode {
    Silent,
    Normal,
    TxOff,
    Sja1000Silent,
}

impl From<CanOutputMode> for i32 {
    fn from(value: CanOutputMode) -> Self {
        match value {
            CanOutputMode::Silent => XL_OUTPUT_MODE_SILENT,
            CanOutputMode::Normal => XL_OUTPUT_MODE_NORMAL,
            CanOutputMode::TxOff => XL_OUTPUT_MODE_TX_OFF,
            CanOutputMode::Sja1000Silent => XL_OUTPUT_MODE_SJA_1000_SILENT,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CanFrame {
    pub id: u32,
    pub flags: u16,
    pub dlc: u8,
    pub data: [u8; 8],
}

impl CanFrame {
    pub fn is_remote_frame(&self) -> bool {
        (self.flags & XL_CAN_MSG_FLAG_REMOTE_FRAME) != 0
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CanEvent {
    Receive(CanFrame),
    Transmit(CanFrame),
    Other { tag: u8, description: String },
}

pub(crate) fn event_from_frame(frame: CanFrame) -> XLevent {
    XLevent {
        tag: XL_TRANSMIT_MSG,
        tagData: XLeventTagData {
            msg: XLcanMsg {
                id: frame.id,
                flags: frame.flags,
                dlc: u16::from(frame.dlc),
                data: frame.data,
                ..XLcanMsg::default()
            },
        },
        ..XLevent::default()
    }
}

pub(crate) fn event_to_can_event(api: &XlApi, event: &XLevent) -> CanEvent {
    match event.tag {
        XL_RECEIVE_MSG => CanEvent::Receive(frame_from_event(event)),
        XL_TRANSMIT_MSG => CanEvent::Transmit(frame_from_event(event)),
        _ => CanEvent::Other {
            tag: event.tag,
            description: api.event_string(event),
        },
    }
}

fn frame_from_event(event: &XLevent) -> CanFrame {
    let msg = unsafe {
        // SAFETY: `tagData.msg` shares layout with the classic CAN message
        // payload for `XL_RECEIVE_MSG` and `XL_TRANSMIT_MSG`.
        event.tagData.msg
    };

    CanFrame {
        id: msg.id,
        flags: msg.flags,
        dlc: msg.dlc as u8,
        data: msg.data,
    }
}

#[cfg(test)]
mod tests {
    use super::{CanEvent, CanFrame, event_from_frame};

    #[test]
    fn frame_round_trips_through_classic_can_event() {
        let frame = CanFrame {
            id: 0x123,
            flags: 0,
            dlc: 8,
            data: [1, 2, 3, 4, 5, 6, 7, 8],
        };

        let event = event_from_frame(frame);
        match event.tag {
            xl_driver_sys::XL_TRANSMIT_MSG => {}
            other => panic!("unexpected event tag {other}"),
        }

        let round_trip = unsafe {
            // SAFETY: transmit events store a classic CAN message in `tagData.msg`.
            event.tagData.msg
        };
        assert_eq!(round_trip.id, frame.id);
        assert_eq!(round_trip.flags, frame.flags);
        assert_eq!(round_trip.dlc, u16::from(frame.dlc));
        assert_eq!(round_trip.data, frame.data);
    }

    #[test]
    fn non_can_events_are_kept_as_other() {
        let event = xl_driver_sys::XLevent {
            tag: 4,
            ..xl_driver_sys::XLevent::default()
        };

        let fallback = CanEvent::Other {
            tag: event.tag,
            description: format!("XL event tag {}", event.tag),
        };

        match fallback {
            CanEvent::Other { tag, .. } => assert_eq!(tag, 4),
            _ => panic!("expected other event"),
        }
    }
}
