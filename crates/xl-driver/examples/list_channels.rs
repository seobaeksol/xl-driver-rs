use xl_driver::{Driver, LibraryLocation};

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let driver = match std::env::var_os("XL_DRIVER_SYS_TEST_DLL") {
        Some(path) => {
            Driver::open_with_location(LibraryLocation::from(path.to_string_lossy().into_owned()))?
        }
        None => Driver::open()?,
    };

    let channels = driver.channels()?;
    if channels.is_empty() {
        println!("No XL driver channels were reported.");
        return Ok(());
    }

    for channel in channels {
        println!(
            "index={} mask=0x{:016X} can={} can_fd={} on_bus={} virtual={} name={} transceiver={}",
            channel.channel_index,
            channel.channel_mask,
            channel.supports_can(),
            channel.supports_can_fd(),
            channel.is_on_bus,
            channel.is_virtual(),
            channel.name,
            channel.transceiver_name,
        );
    }

    Ok(())
}
