use std::time::{Duration, Instant};

use xl_driver::{CanEvent, CanFrame, Driver, LibraryLocation};

const XL_DRIVER_CAN_TEST_CHANNEL_ENV: &str = "XL_DRIVER_CAN_TEST_CHANNEL";
const XL_DRIVER_CAN_TEST_BITRATE_ENV: &str = "XL_DRIVER_CAN_TEST_BITRATE";
const XL_DRIVER_SYS_TEST_DLL_ENV: &str = "XL_DRIVER_SYS_TEST_DLL";

#[test]
#[ignore = "requires a configured CAN-capable XL channel or loopback environment"]
fn classic_can_port_can_activate_transmit_and_observe_events() {
    let driver = open_driver_from_env();
    let channels = driver
        .can_channels()
        .expect("CAN channel discovery should succeed");
    assert!(
        !channels.is_empty(),
        "no CAN-capable XL channels are available for the hardware test"
    );

    let channel_index = std::env::var(XL_DRIVER_CAN_TEST_CHANNEL_ENV)
        .ok()
        .map(|value| {
            value
                .parse::<u32>()
                .expect("XL_DRIVER_CAN_TEST_CHANNEL must be a u32")
        })
        .unwrap_or(channels[0].channel_index);

    let bitrate = std::env::var(XL_DRIVER_CAN_TEST_BITRATE_ENV)
        .ok()
        .map(|value| {
            value
                .parse::<u32>()
                .expect("XL_DRIVER_CAN_TEST_BITRATE must be a u32")
        })
        .unwrap_or(500_000);

    let port = driver
        .open_can_port("xl-driver-rs-hardware-test", &[channel_index], bitrate)
        .expect("CAN port open should succeed");
    port.set_notification()
        .expect("notification setup should succeed");
    port.activate().expect("CAN activation should succeed");

    let frame = CanFrame {
        id: 0x123,
        flags: 0,
        dlc: 2,
        data: [0xCA, 0xFE, 0, 0, 0, 0, 0, 0],
    };
    port.transmit(frame).expect("CAN transmit should succeed");

    let deadline = Instant::now() + Duration::from_secs(2);
    let mut saw_can_event = false;
    while Instant::now() < deadline {
        match port
            .recv_timeout(Duration::from_millis(100))
            .expect("receive wait should succeed")
        {
            Some(CanEvent::Receive(received)) | Some(CanEvent::Transmit(received)) => {
                saw_can_event = true;
                assert_eq!(received.id & !xl_driver_sys::XL_CAN_EXT_MSG_ID, frame.id);
                break;
            }
            Some(CanEvent::Other { .. }) => {}
            None => {}
        }
    }

    port.deactivate().expect("CAN deactivation should succeed");
    assert!(
        saw_can_event,
        "no transmit or receive CAN event arrived within the smoke-test timeout"
    );
}

fn open_driver_from_env() -> Driver {
    match std::env::var_os(XL_DRIVER_SYS_TEST_DLL_ENV) {
        Some(path) => {
            Driver::open_with_location(LibraryLocation::from(path.to_string_lossy().into_owned()))
                .expect("driver open with explicit DLL should succeed")
        }
        None => Driver::open().expect("driver open should succeed"),
    }
}
