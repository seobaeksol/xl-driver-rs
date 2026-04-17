#include <Windows.h>
#include <cstddef>
#include <vector>
#include <sstream>
#include <iomanip>

#include "menu.h"
#include "vxlapi.h"
#include "AutoCleaner.h"

#define CHECKED_XLAPI_CALL(call, failResult)                                                                                     \
  if (const XLstatus status = call; status != XL_SUCCESS) {                                                                      \
    std::cerr << colored(FOREGROUND_RED | FOREGROUND_INTENSITY) << "XL API call " #call " failed with status " << status << ": " \
              << xlGetErrorString(status) << "\n"                                                                                \
              << resetColor;                                                                                                     \
    return failResult;                                                                                                           \
  }

#define CHECKED_XLAPI_CALL_VOID(call) CHECKED_XLAPI_CALL(call, )

namespace {

std::vector<XLtsClockHandle> g_createdClockHandles{};

std::string timeScaleToString(XLtsTimeScale timeScale) {
  switch (timeScale) {
    case XL_TS_TIMESCALE_UNDEFINED:
      return "Undefined";
    case XL_TS_TIMESCALE_UTC:
      return "UTC";
    case XL_TS_TIMESCALE_TAI:
      return "TAI";
    case XL_TS_TIMESCALE_PERFORMANCE_COUNTER:
      return "Performance Counter";
    case XL_TS_TIMESCALE_ARBITRARY:
      return "Arbitrary";
    case XL_TS_TIMESCALE_RTC:
      return "RTC";
  }
  return "Unknown";
}

std::string leapSecondFlagsToString(unsigned int flags) {
  if (flags & XL_TS_LEAP_SECONDS_FLAGS_VALID) {
    return "Valid";
  }
  return "<None>";
}

std::string syncStatusToString(unsigned int status) {
  std::string result{};
  if (status & XL_TS_DOMAIN_STAT_APP_IN_SYNC) {
    result += "AppInSync ";
  }
  if (status & XL_TS_DOMAIN_STAT_DOMAIN_IN_SYNC) {
    result += "DomainInSync ";
  }
  return result.empty() ? "<None>" : result;
}

std::string clockUuidToString(const XLtsClkUuid& uuid) {
  switch (uuid.uuidFormat) {
    case XL_TS_CLK_UUID_FORMAT_UNDEFINED:
      return "Undefined";
    case XL_TS_CLK_UUID_FORMAT_VECTOR_DEV: {
      std::stringstream stream{};
      stream << "article:" << uuid.uuid.vectorDevUuid.articleNumber << " serial:" << uuid.uuid.vectorDevUuid.serialNumber
             << " clockId:" << uuid.uuid.vectorDevUuid.clkId;
      return stream.str();
    }
    case XL_TS_CLK_UUID_FORMAT_EUI64: {
      std::stringstream stream{};
      stream << std::hex << std::setfill('0');
      bool first = true;
      for (const unsigned char c : uuid.uuid.eui64Uuid.oui) {
        if (!first) {
          stream << "::";
        }
        stream << std::setw(2) << static_cast<int>(c);
        first = false;
      }
      for (const unsigned char c : uuid.uuid.eui64Uuid.extensionId) {
        stream << "::" << std::setw(2) << static_cast<int>(c);
      }
      return stream.str();
    }
    case XL_TS_CLK_UUID_FORMAT_LOCAL_PC:
      return "Local PC";
    case XL_TS_CLK_UUID_FORMAT_VECTOR_PC:
      return "Vector PC";
    case XL_TS_CLK_UUID_FORMAT_STANDARD_PC:
    case XL_TS_CLK_UUID_FORMAT_PERFORMANCE_CNT:
      return "Performance Counter";
    case XL_TS_CLK_UUID_FORMAT_EXTERNAL:
      return "External";
  }
  return "Unknown";
}

bool dumpClockHandle(XLtsClockHandle clockHandle) {
  XLtsDomainTime domainTime{};
  XLuint64       slaveTime = 0;
  CHECKED_XLAPI_CALL(xlTsGetDomainTime(clockHandle, &domainTime, &slaveTime), false);

  std::cout << "- Clock handle: " << clockHandle << "\n";
  std::cout << "  - Cluster master: " << clockUuidToString(domainTime.clusterMaster) << "\n";
  std::cout << "  - Sync status: " << syncStatusToString(domainTime.syncStatus) << "\n";
  std::cout << "  - Time scale: " << timeScaleToString(domainTime.timeScale) << "\n";
  std::cout << "  - Leap seconds: flags=" << leapSecondFlagsToString(domainTime.leapSeconds.leapSecondsFlags)
            << ", value=" << domainTime.leapSeconds.leapSecondsValue << "\n";
  std::cout << "  - Time: " << domainTime.domainTime << "\n";
  std::cout << "  - Slave time: " << slaveTime << "\n";
  std::cout << "\n";

  return true;
}

void listClocks() {
  if (g_createdClockHandles.empty()) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "No clocks opened. Create one with 'c'\n" << resetColor;
    return;
  }

  for (const XLtsClockHandle clockHandle : g_createdClockHandles) {
    dumpClockHandle(clockHandle);
  }
}

void createClock() {
  XLtsClockHandle clockHandle{};
  CHECKED_XLAPI_CALL_VOID(
    xlTsCreateClock(&clockHandle, XL_TS_DEFAULT_TIMEDOMAIN_NAME, XL_TS_CLK_EXTTYPE_DOMAIN, XL_TS_INTERFACE_VERSION_1));
  g_createdClockHandles.push_back(clockHandle);
  std::cout << colored(FOREGROUND_GREEN | FOREGROUND_INTENSITY) << "Created clock with handle " << clockHandle << "\n" << resetColor;
}

void destroyClock() {
  if (g_createdClockHandles.empty()) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "No clocks opened. Create one with 'c'\n" << resetColor;
    return;
  }

  std::size_t index = 0;
  if (g_createdClockHandles.size() > 1) {
    std::cout << "Enter the clock handle to destroy [0.." << g_createdClockHandles.size() - 1 << "]: ";
    std::cin >> index;
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    if (index >= g_createdClockHandles.size()) {
      std::cout << colored(FOREGROUND_RED | FOREGROUND_INTENSITY) << "Invalid index\n" << resetColor;
      return;
    }
  }

  const XLtsClockHandle clockHandle = g_createdClockHandles[index];

  CHECKED_XLAPI_CALL_VOID(xlTsDestroyClock(clockHandle));
  g_createdClockHandles.erase(g_createdClockHandles.begin() + index);
  std::cout << colored(FOREGROUND_GREEN | FOREGROUND_INTENSITY) << "Destroyed clock with handle " << clockHandle << "\n" << resetColor;
}

void resetClocks() {
  // Create a driver configuration to get device list
  XLapiIDriverConfigV1 driverConfig;
  CHECKED_XLAPI_CALL_VOID(xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig*)&driverConfig));
  AutoCleaner driverConfigCleaner{driverConfig.configHandle, xlDestroyDriverConfig};

  // Get list of devices
  XLdeviceDrvConfigListV1 deviceList;
  if (XL_SUCCESS != driverConfig.fctGetDeviceConfig(driverConfig.configHandle, &deviceList)) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "Failure getting network configuration\n" << resetColor;
    return;
  }

  // Go thorugh all devices and their channels to find a CAN channel
  char appName[XL_MAX_APPNAME] = "xlVTssDemo";
  XLaccess channelMaskCAN          = 0;
  std::stringstream channelIdxs;
  for (unsigned int deviceIterator = 0; deviceIterator < deviceList.count; deviceIterator++) {
    const XLdeviceDrvConfigV1& device = deviceList.item[deviceIterator];
    for (unsigned int channelIterator = 0; channelIterator < device.channelList.count; channelIterator++) {
      const XLchannelDrvConfigV1& channel = device.channelList.item[channelIterator];
      if ((channel.channelBusCapabilities & XL_BUS_TYPE_CAN) && (device.hwType != XL_HWTYPE_VIRTUAL)) {
        channelMaskCAN |= xlGetChannelMask(device.hwType, device.hwIndex, channelIterator);
        channelIdxs << channel.channelIndex << ",";
      }
    }
  }
  
  if (channelMaskCAN == 0) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "No channels available.\n" << resetColor;
    return;
  }

  XLportHandle portHandle;
  CHECKED_XLAPI_CALL_VOID(xlOpenPort(&portHandle, appName, channelMaskCAN, &channelMaskCAN, 256, XL_INTERFACE_VERSION, XL_BUS_TYPE_CAN));
  AutoCleaner portCleaner{portHandle, xlClosePort};

  // Reset the clock for CAN channels
  XLtsTimeScale   timescale;
  XLtsLeapSeconds leapSeconds;
  XLtsClkUuid     gmUuid;
  XLuint64        timeOffset = 0;
  CHECKED_XLAPI_CALL_VOID(xlTsResetClocks(portHandle, &timescale, &leapSeconds, &gmUuid, &timeOffset));
  std::cout << "- Reset clocks on channel [" << channelIdxs.str().erase(channelIdxs.str().size() - 1) << "]:\n";
  std::cout << "  - Cluster master: " << clockUuidToString(gmUuid) << "\n";
  std::cout << "  - Time scale: " << timeScaleToString(timescale) << "\n";
  std::cout << "  - Leap seconds: flags=" << leapSecondFlagsToString(leapSeconds.leapSecondsFlags)
            << ", value=" << leapSeconds.leapSecondsValue << "\n";
  std::cout << "  - TimeOffset: " << timeOffset << "\n";
  std::cout << "\n";
  return;
}

void resetClocksNet() {
  // Create a driver configuration to get the network list
  XLapiIDriverConfigV1 driverConfig;
  CHECKED_XLAPI_CALL_VOID(xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig*)&driverConfig));
  AutoCleaner driverConfigCleaner{driverConfig.configHandle, xlDestroyDriverConfig};

  // Get Network List
  XLnetworkDrvConfigListV1 networkList;
  if (XL_SUCCESS != driverConfig.fctGetNetworkConfig(driverConfig.configHandle, &networkList)) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "Failure getting network configuration.\n" << resetColor;
    return;
  }

  if (networkList.count == 0) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN)
              << "No networks available. Networks can be created in Vector Hardware Manager->Ethernet Network Configuration.\n"
              << resetColor;
    return;
  }

  for (unsigned int network = 0; network < networkList.count; network++) {
    // Open network
    XLnetworkHandle networkhandle;
    char            appName[XL_MAX_APPNAME] = "xlVTssDemo";
    CHECKED_XLAPI_CALL_VOID(xlNetEthOpenNetwork(networkList.item[network].networkName, &networkhandle, appName, XL_ACCESS_TYPE_RELIABLE,
                                                8 * 1024 * 1024));  //8 MB receive buffer
    AutoCleaner networkCleaner{networkhandle, xlNetCloseNetwork};

    // Reset clocks on network
    XLtsTimeScale   timescale;
    XLtsLeapSeconds leapSeconds;
    XLtsClkUuid     gmUuid;
    XLuint64        timeOffset = 0;
    CHECKED_XLAPI_CALL_VOID(xlNetTsResetClocks(networkhandle, &timescale, &leapSeconds, &gmUuid, &timeOffset));
    std::cout << "- Reset clocks on network " << networkList.item[network].networkName << ":\n";
    std::cout << "  - Cluster master: " << clockUuidToString(gmUuid) << "\n";
    std::cout << "  - Time scale: " << timeScaleToString(timescale) << "\n";
    std::cout << "  - Leap seconds: flags=" << leapSecondFlagsToString(leapSeconds.leapSecondsFlags)
              << ", value=" << leapSeconds.leapSecondsValue << "\n";
    std::cout << "  - TimeOffset: " << timeOffset << "\n";
    std::cout << "\n";
  }
}

void getHardwareChannelStatus() {
  XLapiIDriverConfigV1 driverConfig;
  CHECKED_XLAPI_CALL_VOID(xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig*)&driverConfig));
  AutoCleaner driverConfigCleaner{driverConfig.configHandle, xlDestroyDriverConfig};

  // List CAN channels
  std::cout << "Available CAN channels: \n";
  XLdeviceDrvConfigListV1 deviceList;
  if (XL_SUCCESS != driverConfig.fctGetDeviceConfig(driverConfig.configHandle, &deviceList)) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "  - Failure getting Device configuration.\n" << resetColor;
  }
  else {
    bool foundChannel = false;
    for (unsigned int deviceIterator = 0; deviceIterator < deviceList.count; deviceIterator++) {
      const XLdeviceDrvConfigV1& device = deviceList.item[deviceIterator];
      for (unsigned int channelIterator = 0; channelIterator < device.channelList.count; channelIterator++) {
        const XLchannelDrvConfigV1& channel = device.channelList.item[channelIterator];
        if ((channel.channelBusCapabilities & XL_BUS_TYPE_CAN) && (device.hwType != XL_HWTYPE_VIRTUAL)) {
          foundChannel         = true;
          XLaccess channelMask = xlGetChannelMask(device.hwType, device.hwIndex, channelIterator);
          std::cout << std::setfill('0') << "  - Ch: " << std::dec << std::setw(2) << +channel.channelIndex << " CM : 0x" << std::hex
                    << std::setw(std::numeric_limits<XLaccess>::digits / 4) << channelMask << "; On Device: " << device.name;
          if (channel.transceiver.type != XL_TRANSCEIVER_TYPE_NONE) {
            std::cout << "; " << channel.transceiver.name;
          }
          std::cout << "\n";
        }
      }
      std::cout.copyfmt(std::ios(nullptr)); // clear output formatting
    }
    if (!foundChannel) {
      std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "  - No CAN channel available.\n" << resetColor;
    }
  }

  // List ethernet networks
  std::cout << "\n" << "Available Ethernet networks: \n";
  XLnetworkDrvConfigListV1 networkList;
  if (XL_SUCCESS != driverConfig.fctGetNetworkConfig(driverConfig.configHandle, &networkList)) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "  - Failure getting network configuration.\n\n" << resetColor;
    return;
  }

  if (networkList.count == 0) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN)
              << "  - No networks available. Networks can be created in Vector Hardware Manager->Ethernet Network Configuration.\n\n"
              << resetColor;
    return;
  }

  for (unsigned int network = 0; network < networkList.count; network++) {
    std::cout << "  - " << networkList.item[network].networkName << "\n";
  }
  std::cout << "\n";
}

void getStatus() {
  // Create a driver configuration to get the device list
  XLapiIDriverConfigV1 driverConfig;
  CHECKED_XLAPI_CALL_VOID(xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig*)&driverConfig));
  AutoCleaner driverConfigCleaner{driverConfig.configHandle, xlDestroyDriverConfig};

  // Get a list of devices
  XLdeviceDrvConfigListV1 deviceList;
  if (XL_SUCCESS != driverConfig.fctGetDeviceConfig(driverConfig.configHandle, &deviceList)) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "Failure getting Device configuration.\n\n" << resetColor;
    return;
  }

  // Go thorugh all devices and their channels to find all CAN channels
  XLaccess channelMask             = 0;
  char     appName[XL_MAX_APPNAME] = "xlVTssDemo";
  for (unsigned int deviceIterator = 0; deviceIterator < deviceList.count; deviceIterator++) {
    const XLdeviceDrvConfigV1& device = deviceList.item[deviceIterator];
    for (unsigned int channelIterator = 0; channelIterator < device.channelList.count; channelIterator++) {
      const XLchannelDrvConfigV1& channel = device.channelList.item[channelIterator];
      if ((channel.channelBusCapabilities & XL_BUS_TYPE_CAN) && (device.hwType != XL_HWTYPE_VIRTUAL)) {
        channelMask += xlGetChannelMask(device.hwType, device.hwIndex, channelIterator);
      }
    }
  }

  if (channelMask == 0) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "No channels available.\n\n" << resetColor;
    return;
  }

  // Open port with all CAN channels
  XLportHandle portHandle;
  CHECKED_XLAPI_CALL_VOID(xlOpenPort(&portHandle, appName, channelMask, &channelMask, 256, XL_INTERFACE_VERSION, XL_BUS_TYPE_CAN));
  AutoCleaner portCleaner{portHandle, xlClosePort};

  // Call get status
  XLtsTimeScale   timescale;
  XLtsLeapSeconds leapSeconds;
  XLtsClkUuid     gmUuid;
  CHECKED_XLAPI_CALL_VOID(xlTsGetStatus(portHandle, XL_USE_ALL_CHANNELS, &timescale, &leapSeconds, &gmUuid, nullptr));
  std::cout << "- Get status of CAN channels:\n";
  std::cout << "  - Cluster master: " << clockUuidToString(gmUuid) << "\n";
  std::cout << "  - Time scale: " << timeScaleToString(timescale) << "\n";
  std::cout << "  - Leap seconds: flags=" << leapSecondFlagsToString(leapSeconds.leapSecondsFlags)
            << ", value=" << leapSeconds.leapSecondsValue << "\n";
  std::cout << "\n";
}

void getStatusNet() {
  // Create a driver configuration to get the network list
  XLapiIDriverConfigV1 driverConfig;
  CHECKED_XLAPI_CALL_VOID(xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig*)&driverConfig));
  AutoCleaner driverConfigCleaner{driverConfig.configHandle, xlDestroyDriverConfig};

  // Get Network List
  XLnetworkDrvConfigListV1 networkList;
  if (XL_SUCCESS != driverConfig.fctGetNetworkConfig(driverConfig.configHandle, &networkList)) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN) << "Failure getting network configuration.\n\n" << resetColor;
    return;
  }

  if (networkList.count == 0) {
    std::cout << colored(FOREGROUND_RED | FOREGROUND_GREEN)
              << "No networks available. Networks can be created in Vector Hardware Manager->Ethernet Network Configuration.\n\n"
              << resetColor;
    return;
  }

  for (unsigned int network = 0; network < networkList.count; network++) {
    // Open network
    XLnetworkHandle networkhandle;
    char            appName[XL_MAX_APPNAME] = "xlVTssDemo";
    CHECKED_XLAPI_CALL_VOID(xlNetEthOpenNetwork(networkList.item[network].networkName, &networkhandle, appName, XL_ACCESS_TYPE_RELIABLE,
                                                8 * 1024 * 1024));  //8 MB receive buffer
    AutoCleaner networkCleaner{networkhandle, xlNetCloseNetwork};

    // Reset clocks on network
    XLtsTimeScale   timescale;
    XLtsLeapSeconds leapSeconds;
    XLtsClkUuid     gmUuid;
    CHECKED_XLAPI_CALL_VOID(xlNetTsGetStatus(networkhandle, XL_INVALID_ETHPORTHANDLE, &timescale, &leapSeconds, &gmUuid, nullptr));
    std::cout << "- Get status on network " << networkList.item[network].networkName << ":\n";
    std::cout << "  - Cluster master: " << clockUuidToString(gmUuid) << "\n";
    std::cout << "  - Time scale: " << timeScaleToString(timescale) << "\n";
    std::cout << "  - Leap seconds: flags=" << leapSecondFlagsToString(leapSeconds.leapSecondsFlags)
              << ", value=" << leapSeconds.leapSecondsValue << "\n";
    std::cout << "\n";
  }
}

void monitorStatus() {
  // Create status port which will receive the events.
  XLportHandle statusPort{};
  char         appName[XL_MAX_APPNAME] = "xlVTssDemo";
  CHECKED_XLAPI_CALL_VOID(xlCreatePort(&statusPort, appName, 0, XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_STATUS));
  CHECKED_XLAPI_CALL_VOID(xlFinalizePort(statusPort));
  AutoCleaner statusPortCleaner{statusPort, xlClosePort};

  // Activate status event for this new network.
  XLhandle eventHandle{};
  CHECKED_XLAPI_CALL_VOID(xlTsSetNotification(statusPort, XL_INVALID_PORTHANDLE, &eventHandle));

  std::cout << "Waiting 10s for an event on all ports...\n";
  if (WaitForSingleObject(static_cast<HANDLE>(eventHandle), 10'000) == STATUS_WAIT_0) {
    std::cout << colored(FOREGROUND_GREEN | FOREGROUND_INTENSITY) << "Event received\n" << resetColor;
    getStatus();
  }
  else {
    std::cout << "Timeout\n";
  }
}

void monitorStatusNet() {
  // Create status port which will receive the events.
  XLportHandle statusPort{};
  char         appName[XL_MAX_APPNAME] = "xlVTssDemo";
  CHECKED_XLAPI_CALL_VOID(xlCreatePort(&statusPort, appName, 0, XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_STATUS));
  CHECKED_XLAPI_CALL_VOID(xlFinalizePort(statusPort));
  AutoCleaner statusPortCleaner{statusPort, xlClosePort};

  // Activate status event for all networks.
  XLhandle eventHandle{};
  CHECKED_XLAPI_CALL_VOID(xlNetTsSetNotification(statusPort, XL_INVALID_NETWORKHANDLE, &eventHandle));

  std::cout << "Waiting 10s for an event on all networks...\n";
  if (WaitForSingleObject(static_cast<HANDLE>(eventHandle), 10'000) == STATUS_WAIT_0) {
    std::cout << colored(FOREGROUND_GREEN | FOREGROUND_INTENSITY) << "Event received\n" << resetColor;
    getStatusNet();
  }
  else {
    std::cout << "Timeout\n";
  }
}

void mainMenuPrinter() {
  std::cout << "- 'l'\tGet opened clock domain times\n";
  std::cout << "- 'c'\tCreate a new clock\n";
  std::cout << "- 'x'\tDestroy a new clock\n";
  std::cout << "- 'r'\tReset clocks for channels\n";
  std::cout << "- 'R'\tReset clocks for networks\n";
  std::cout << "- 'H'\tGet hardware channels\n";
  std::cout << "- 's'\tGet status for channels\n";
  std::cout << "- 'S'\tGet status for networks\n";
  std::cout << "- 'm'\tMonitor status for channels\n";
  std::cout << "- 'M'\tMonitor status for networks\n";
}

MenuAction mainMenuHandler(char choice) {
  switch (choice) {
    case 'l':
      listClocks();
      break;
    case 'c':
      createClock();
      break;
    case 'r':
      resetClocks();
      break;
    case 'R':
      resetClocksNet();
      break;
    case 's':
      getStatus();
      break;
    case 'S':
      getStatusNet();
      break;
    case 'H':
      getHardwareChannelStatus();
      break;
    case 'x':
      destroyClock();
      break;
    case 'm':
      monitorStatus();
      break;
    case 'M':
      monitorStatusNet();
      break;
  }
  return MenuAction::CONTINUE;
}

}  // namespace

int main() {
  CHECKED_XLAPI_CALL(xlOpenDriver(), EXIT_FAILURE);

  menuMain("Main Menu", &mainMenuPrinter, &mainMenuHandler);

  CHECKED_XLAPI_CALL(xlCloseDriver(), EXIT_FAILURE);

  return EXIT_SUCCESS;
}
