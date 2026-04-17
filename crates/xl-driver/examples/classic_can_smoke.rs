use std::time::{Duration, Instant};

use xl_driver::{CanEvent, CanFrame, Driver, LibraryLocation};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let driver = match std::env::var_os("XL_DRIVER_SYS_TEST_DLL") {
        Some(path) => {
            Driver::open_with_location(LibraryLocation::from(path.to_string_lossy().into_owned()))?
        }
        None => Driver::open()?,
    };

    let channels = driver.can_channels()?;
    let channel_index = std::env::var("XL_DRIVER_CAN_TEST_CHANNEL")
        .ok()
        .map(|value| value.parse::<u32>())
        .transpose()?;
    let Some(channel) = channel_index
        .and_then(|index| {
            channels
                .iter()
                .find(|candidate| candidate.channel_index == index)
        })
        .or_else(|| channels.first())
    else {
        return Err("no CAN-capable XL channels are available".into());
    };

    let bitrate = std::env::var("XL_DRIVER_CAN_TEST_BITRATE")
        .ok()
        .and_then(|value| value.parse().ok())
        .unwrap_or(500_000);

    let port = driver.open_can_port("xl-driver-rs-example", &[channel.channel_index], bitrate)?;
    port.set_notification()?;
    port.activate()?;

    let frame = CanFrame {
        id: 0x123,
        flags: 0,
        dlc: 1,
        data: [0xA5, 0, 0, 0, 0, 0, 0, 0],
    };

    port.transmit(frame)?;

    let deadline = Instant::now() + Duration::from_secs(2);
    while Instant::now() < deadline {
        if let Some(event) = port.recv_timeout(Duration::from_millis(100))? {
            match event {
                CanEvent::Receive(received) | CanEvent::Transmit(received) => {
                    println!("Observed CAN event: {received:?}");
                    port.deactivate()?;
                    return Ok(());
                }
                CanEvent::Other { description, .. } => {
                    println!("Observed other XL event: {description}");
                }
            }
        }
    }

    port.deactivate()?;
    Err("no CAN event arrived within the smoke-test timeout".into())
}
