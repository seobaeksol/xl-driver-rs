/*-------------------------------------------------------------------------------------------
| File        : xlNetEthDemo.cpp
| Project     : Vector XL-API Command Line Example for Network-based Mode Ethernet functions
|
| Description : Shows how to configure a Vector Ethernet device (VN56xx) for frame transmission
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2022 by Vector Informatik GmbH.  All rights reserved.
|--------------------------------------------------------------------------------------------*/

/*===========================================================*/
/* XL-API Demo for Network-Based Mode Ethernet Configuration */
/*===========================================================*/
/*---------------------------------------------------------
                   xlNetEthDemo - HELP                       
-----------------------------------------------------------
 Keyboard commands:                                        
 '1'..'%u' Select Ethernet port                            
 '+'      Select next Ethernet port     [MP]               
 '-'      Select previous Ethernet port [MP]               
 't'      Transmit single packet        [MP]               
 '*'      Select next Ethernet port     [VP]               
 '/'      Select previous Ethernet port [VP]               
 'T'      Transmit single packet        [VP]               
 's'      Current port link status                         
 'p'      Print INFO of last received Eth message          
 'G'      Request new MAC address                          
 'R'      Release taken MAC address                        
 'm'      Set receiver MAC address                         
 'i'      Set receiver IP  address                         
 'w'      Show driver configuration                        
 'r'      Reset clocks on network                          
 'h'/'?'  Help                                             
 'q'      Quit                                             
---------------------------------------------------------*/
/*
   xlNetEthDemo - Application Demo implemented in CPP to show basic usage of Vector Hardware through XL-API
                - Demo walks through needed operation: i.e. opening communication with the driver,
                  reading available devices, available networks on those devices, sending receiving data on 
                  VPs or MPs, etc.
                - As a demo, there is a control/help menu, as shown above, that provides demo for specific
                  demo function: i.e. transmit a single packet over MP / VP, check Link status,
                  show config, etc.
*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>
#include <iomanip>
#include <conio.h>
#include "vxlapi.h"

namespace xlNetEthDemo {
  /*************/
  /* Constants */
  /*************/
  constexpr unsigned int MAX_USER_HANDLE{ 128 };
  constexpr unsigned int MAX_PAYLOAD_LENGTH{ 1500 };

  /** The EtherType of IPv4 is 0x0800. We store it in network byte-order. */
  constexpr unsigned short EthertypeIpV4Be{ 0x0080 };

  /** Size of the EtherType field in number of bytes. */
  constexpr int EtherTypeSize = 2;

  /**  Size of the IPv4 header without options in number of bytes. */
  constexpr int IpV4HeaderSize = 20;

  /** Size of a UDP header in number of bytes. */
  constexpr int UdpHeaderSize = 8;

  constexpr unsigned char defaultDestMAC[]{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }; //Broadcast MAC
  constexpr unsigned char defaultDestIP[]{ 192, 168, 255, 255 };                  //Broadcast IP

  constexpr unsigned char EscKeyCode{ 0x1B };


  //--------------------------------------------------------------------------------------------

  /** Custom XL-API exception returning the an error string if something goes wrong. */
  class XlApiException : public std::runtime_error {
  private:
    XLstatus mStatus;
  public:
    XlApiException(XLstatus s) : std::runtime_error("XL-API Exception"), mStatus(s) {}
    const char* what() const throw () {
      return xlGetErrorString(mStatus);
    }
  };

  /** This generic helper class defines common properties of MPs and VPs */
  struct Port {
    /** XLAPI assigned handle that identifies the MP or VP in function calls. */
    XLethPortHandle PortHandle{ 0 };

    /** Application assigned handle that identifies the MP or VP in XLAPI events. */
    XLrxHandle RxHandle{ 0 };

    virtual const char* Name() = 0;
    virtual const char* Type() = 0;
  };

  /** Helper class to encapsulate a measurement port. */
  struct MpPort : Port {
    /** Name of the measurement port. */
    std::string mName;

    /** Hardware MAC address provided by the network. */
    T_XL_ETH_MAC_ADDRESS mMacAddress;

    MpPort(const XLmeasurementpointDrvConfigV1* pMpDrvCfg) 
    {
      // Create a copy of the measurementPointName, as the pointer to the name becomes invalid
      // as soon as the driver config is being destroyed by xlDestroyDriverCfg.
      mName = std::string(pMpDrvCfg->measurementPointName);

      memcpy(mMacAddress.address, pMpDrvCfg->channel->busParams.data.ethernet.macAddr, XL_ETH_MACADDR_OCTETS);
    }

    const char* Name() final {
      return mName.c_str();
    }

    const char* Type() final {
      return "MP";
    }
  };

  /** Helper class to encapsulate a virtual port. */
  struct VpPort : Port {
    /** Name of the virtual port. */
    std::string mName;

    VpPort(const XLvirtualportDrvConfigV1* pVpDrvCfg) {
      // Create a copy of the virtualPortName, as the pointer to the name becomes invalid
      // as soon as the driver config is being destroyed by xlDestroyDriverCfg.
      mName = std::string(pVpDrvCfg->virtualPortName);
    }

    const char* Name() final {
      return mName.c_str();
    }

    const char* Type() final {
      return "VP";
    }
  };

  /**
   * The demo application.
   */
  class DemoApplication {
  private:
    std::string       mAppName{ "xlNetEthDemo" };

    /** Notification event signaled when the driver adds new XLAPI events to the receive queue. */
    XLhandle          mNotificationHandle{ NULL };

    XLapiIDriverConfigV1 mDriverCfg;

    /** List of devices found at start of demo */
    XLdeviceDrvConfigListV1 mDevices;

    /** List of networks found at start of demo. */
    XLnetworkDrvConfigListV1 mNetworkList;

    /** Information on the network opened by this application. */
    int mSelectedNetworkIdx{ -1 };

    /** Information on the segment that the user selected to add a VP on. */
    const XLswitchDrvConfigV1* mSegmentToAddVpOn{ nullptr };

    /** Handle for the network opened by this application. */
    XLnetworkHandle mNetworkHandle{ XL_INVALID_NETWORKHANDLE };

    /** List of all connected measurement ports. */
    std::vector<std::shared_ptr<MpPort>> mConnectedMPs;

    /** List of all open virtual ports. */
    std::vector<std::shared_ptr<VpPort>> mOpenVPs;

    /** Mapping from XlRxHandle to Ports (MP or VP) for quick lookup in XLAPI event processing. */
    std::map<XLrxHandle, std::shared_ptr<Port>> mRxHandleMap;

    /** Index of the currently selected MP in mConnectedMPs or -1 if no MP is selected. */
    int mCurrentMpIdx = -1;

    /** Index of the currently selected VP in mOpenVPs or -1 if no VP is selected. */
    int mCurrentVpIdx = -1;

    /** True if the network is active. */
    bool mIsNetworkActivated = false;

    /** Buffer to hold the last received frame instead of allocating new memory on every received frame. */
    T_XL_NET_ETH_DATAFRAME_RX mLastRxFrameReceived{ NULL };

    /** User selectable destination Mac address. */
    T_XL_ETH_MAC_ADDRESS mRemoteMac;

    /** Source Mac address requested from network. */
    T_XL_ETH_MAC_ADDRESS mSourceMac{ NULL };

    /** True if MAC address was obtained from network. */
    bool mSrcMacObtained{ false };

    /** User selectable destination IP Address. */
    unsigned char mRemoteIp[4]{ defaultDestIP[0], defaultDestIP[1], defaultDestIP[2], defaultDestIP[3] };

    // IPv4
    unsigned char mIPSrc[4] = { 196, 168, 0, 0 };

    // Example payload
    unsigned char mEthPayload[MAX_PAYLOAD_LENGTH] = { "Vector Informatik GmbH : Network-based mode Ethernet Example" };
    unsigned short mCurrentPayloadSize = sizeof("Vector Informatik GmbH : Network-based mode Ethernet Example") / sizeof(char);

    // -----------------------------------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------------------------------
    /** The first byte of the IPv4 header. */
    const unsigned char IpV4VersionIhlField = 0x40 | (IpV4HeaderSize / 4);

    /** Value of the IPv4 Protocol field for UDP. */
    const unsigned char IpProtocolUdp = 17;

    /** Name of the virtual port that this demo adds to a user-selectable segment. */
    const std::string AddVpName{ "VP_manually_added" };

    /** UDP Source port that the demo writes in transmitted frames. */
    const uint16_t UdpSourcePort = 65281;

    /** UDP destination port that the demo writes in transmitted frames. */
    const uint16_t UdpRemotePort = 65282;
    

  public:
    DemoApplication() {
      XLstatus result;
      DWORD fdwMode;

      if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &fdwMode)) {
        printf("GetConsoleMode() failed with cause %lu (0x%lx). Stop.\n", GetLastError(), GetLastError());
        throw std::runtime_error("GetConsoleMode failed.");
      }
      else if (!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), fdwMode & ~ENABLE_MOUSE_INPUT)) {
        printf("SetConsoleMode() failed with cause %lu (0x%lx). Stop.\n", GetLastError(), GetLastError());
        throw std::runtime_error("SetConsoleMode failed.");
      }

      // Set remote MAC address to default.
      memcpy(mRemoteMac.address, defaultDestMAC, XL_ETH_MACADDR_OCTETS);

      // ----------------------------------------------------------------------------------------
      //                                OPEN DRIVER [xlOpenDriver]
      // ----------------------------------------------------------------------------------------
      if (XL_SUCCESS != (result = xlOpenDriver())) {
        printf("ERROR : Could not open driver.\n");
        // We cannot use the XlApiException here, because the library which provides
        // the xlGetErrorString method has not been loaded yet.
        throw std::runtime_error("Could not open XL-API driver.");
      }
    }

    ~DemoApplication() {
      // Free resources
      XLstatus result;

      // ----------------------------------------------------------------------------------------
      //                      DEACTIVATE NETWORK [xlNetDeactivateNetwork]
      // ----------------------------------------------------------------------------------------
      if (mIsNetworkActivated) {
        if (XL_SUCCESS != (result = xlNetDeactivateNetwork(mNetworkHandle))) {
          printf("ERROR : Failed to deactivate network! ERR::%s (%d)\n", xlGetErrorString(result), result);
        }
        mIsNetworkActivated = false;
      }

      // ----------------------------------------------------------------------------------------
      //                         CLOSE NETWORK [xlNetCloseNetwork]
      // ----------------------------------------------------------------------------------------
      if (mNetworkHandle != XL_INVALID_NETWORKHANDLE) {
        if (XL_SUCCESS != (result = xlNetCloseNetwork(mNetworkHandle))) {
          printf("ERROR : Failed to close network! ERR::%s (%d)\n", xlGetErrorString(result), result);
        }
      }

      // ----------------------------------------------------------------------------------------
      //                            CLOSE DRIVER [xlCloseDriver]
      // ----------------------------------------------------------------------------------------
      if (XL_SUCCESS != (result = xlCloseDriver())) {
        printf("ERROR : Failed to close driver! ERR::%s (%d)\n", xlGetErrorString(result), result);
      }
    }

    void Run() {
      XLstatus result;
      printf("------------------------------------------------------------\n");
      printf("- xlNetEthDemo - Test Application for XL Family Driver API -\n");
      printf("-             (C) 2025 Vector Informatik GmbH              -\n");
      printf("------------------------------------------------------------\n");


      // ----------------------------------------------------------------------------------------
      //                   CREATE CONFIGURATION [xlCreateDriverConfig]
      // ----------------------------------------------------------------------------------------

      /* NOTE:
         -------------------
         > As xlCreateDriverConfig is called for the first time in the Application.
           There is no need to call xlDestroyDriverConfig first
         > See getNetEthNetworkConfig: for using xlDestroyDriverConfig prior to xlCreateDriverConfig
         -------------------
      */
      if (XL_SUCCESS != (result = xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig *)&mDriverCfg))) {
        printf("ERROR : Could not read driver config [xlCreateDriverConfig]. Reason: %s.\n", xlGetErrorString(result));
        throw XlApiException(result);
      }

      // List all available Devices found by XL-API
      if (XL_SUCCESS != (result = mDriverCfg.fctGetDeviceConfig(mDriverCfg.configHandle, &mDevices))) {
        printf("ERROR : Could not get device config [fctGetDeviceConfig]. Reason: %s.\n", xlGetErrorString(result));
        throw XlApiException(result);
      }
      printf("Found %i devices\n", mDevices.count);
      for (unsigned int devIdx = 0; devIdx < mDevices.count; devIdx++) {
        // NOTE:  .hwIndex+1 as .hwIndex numbering starts from 0
        printf("[%i] Found %s:%i(%06i) device\n", devIdx, mDevices.item[devIdx].name, mDevices.item[devIdx].hwIndex + 1, mDevices.item[devIdx].serialNumber);

        // Check network access mode for each channel
        for (unsigned int i = 0; i < mDevices.item[devIdx].channelList.count; i++) {
          auto& item = mDevices.item[devIdx].channelList.item[i];
          bool channelHasNetEthSupport = ((item.channelCapabilities & XL_CHANNEL_FLAG_EX1_NET_ETH_SUPPORT) != 0);
          printf("    Channel %u, %s, access-mode: %s\n", item.hwChannel, item.transceiver.name, (channelHasNetEthSupport ? "Network" : "Channel"));
        }
      }

      // List all available Networks from all devices
      if (XL_SUCCESS != (result = mDriverCfg.fctGetNetworkConfig(mDriverCfg.configHandle, &mNetworkList))) {
        printf("ERROR : Could not get network config [fctGetNetworkConfig]. Reason: %s.\n", xlGetErrorString(result));
        throw XlApiException(result);
      }

      if (mNetworkList.count > 0) {
        // Print set of available networks and let the user choose the network for this demo.
        ShowNetwork(mNetworkList);

        mSelectedNetworkIdx = ChooseNetwork(mNetworkList);
        if (mSelectedNetworkIdx == -1) {
          // User pressed ESC
          return;
        }
      }
      else {
        AbortDueToMissingNetwork();
        return;
      }

      ShowNetwork(mNetworkList, mSelectedNetworkIdx);

      mSegmentToAddVpOn = ChooseSegmentForAddVp();

      // ---------------------------------------------------------------------------------------------------------------------------------------
      //                                                   OPEN NETWORK [xlNetEthOpenNetwork]
      // ---------------------------------------------------------------------------------------------------------------------------------------
      const char *pNetworkName = mNetworkList.item[mSelectedNetworkIdx].networkName;
      // Open the network with an 8 MB receive queue. The network handle is used for all subsequent function calls on this network.
      if (XL_SUCCESS != (result = xlNetEthOpenNetwork(pNetworkName,
                                                      &mNetworkHandle,
                                                      mAppName.c_str(),
                                                      XL_ACCESS_TYPE_RELIABLE,
                                                      8 * 1024 * 1024) //8 MB receive buffer
                         )) {
        printf("ERROR : Failed to Open Network [xlNetEthOpenNetwork]! ERR::%s (%d)\n", xlGetErrorString(result), result);
        throw XlApiException(result);
      }

      // Call xlNetAddVirtualPort, xlNetOpenVirtualPort and/or xlNetConnectMeasurementPoint. At least one port must be opened.
      AddOpenOrConnectVPandMP();
      if (mRxHandleMap.size() == 0) {
        AbortDueToMissingPorts();
        return;
      }

      // Request a notification at queueLevel=1 byte (required to use event handling via WaitForMultipleObjects).
      if (XL_SUCCESS != (result = xlNetSetNotification(mNetworkHandle, &mNotificationHandle, 1))) { // (1) - Notify for each single event in queue
        printf("ERROR : Failed to set notification handle! [xlNetSetNotification]! ERR::%s (%d)\n", xlGetErrorString(result), result);
        throw XlApiException(result);
      }

      // ---------------------------------------------------------------------------------------------------------------------------------------
      //                                              ACTIVATE NETWORK [xlNetActivateNetwork]
      // ---------------------------------------------------------------------------------------------------------------------------------------
      // Activate network (start receiving events)
      if (XL_SUCCESS != (result = xlNetActivateNetwork(mNetworkHandle))) {
        printf("ERROR : Failed to Activate Network [xlNetActivateNetwork]! ERR::%s (%d)\n", xlGetErrorString(result), result);
        throw XlApiException(result);
      }
      mIsNetworkActivated = true;
      // ---------------------------------------------------------------------------------------------------------------------------------------

      printf("\nINFO : Initialization finished. Press any key to continue demo...\n");
      printf("> ");
      _getch();

      // Print even more information...
      ShowSettings();
      ShowDriverConfig();
      ShowMenuHelp();
      printf("> ");

      // Enter the main loop
      MainLoop();
    }

private:
  //--------------------------------------------------------------------------------------------
  // Command Line Input/Controls
  void ShowMenuHelp() {
    printf("\n");
    printf("-------------------------------------------------------------\n");
    printf("-                   xlNetEthDemo - HELP                     -\n");
    printf("-------------------------------------------------------------\n");
    printf("- Keyboard commands:                                        -\n");
    if (mConnectedMPs.size() > 1) {
      printf("- '1'..'%u' Select Ethernet port          [MP]              -\n", min(mConnectedMPs.size(), 9));
      printf("- '+'      Select next Ethernet port     [MP]               -\n");
      printf("- '-'      Select previous Ethernet port [MP]               -\n");
    }
    printf("- 't'      Transmit single packet        [MP]               -\n");
    printf("- '*'      Select next Ethernet port     [VP]               -\n");
    printf("- '/'      Select previous Ethernet port [VP]               -\n");
    printf("- 'T'      Transmit single packet        [VP]               -\n");
    printf("- 's'      Current port link status                         -\n");
    printf("- 'p'      Print INFO of last received Eth message          -\n");
    printf("- 'G'      Request new MAC address                          -\n");
    printf("- 'R'      Release taken MAC address                        -\n");
    printf("- 'm'      Set receiver MAC address                         -\n");
    printf("- 'i'      Set receiver IP  address                         -\n");
    printf("- 'w'      Show driver configuration                        -\n");
    printf("- 'r'      Reset clocks on network                          -\n");
    printf("- 'h'/'?'  Help                                             -\n");
    printf("- 'q'      Quit                                             -\n");
    printf("-------------------------------------------------------------\n");
    printf("\n");

  }

  void ShowSettings() {
    printf("Default settings:\n");
    printf("- Port index: %u\n", mCurrentMpIdx);
    printf("- Remote MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mRemoteMac.address[0], mRemoteMac.address[1], mRemoteMac.address[2],
           mRemoteMac.address[3], mRemoteMac.address[4], mRemoteMac.address[5]);
    printf("- Remote IP address:  %3i.%3i.%3i.%3i\n",
           mRemoteIp[0], mRemoteIp[1],
           mRemoteIp[2], mRemoteIp[3]);
  }

  void ShowDriverConfig() {
    XLstatus        result = XL_SUCCESS;

    XLapiIDriverConfigV1     localDriverConfig;
    XLnetworkDrvConfigListV1 localNetworkConfigList;
    XLdeviceDrvConfigListV1  localDeviceConfigList;

    if (XL_SUCCESS == (result = GetNetEthNetworkConfig(&localDriverConfig, &localNetworkConfigList))) {
      GetNetEthDeviceConfig(&localDriverConfig, &localDeviceConfigList);
      ShowNetwork(localNetworkConfigList);

      // NOTE: Call [xlDestroyDriverConfig] to destroy current snapshot - to prevent memory leakage
      if (XL_SUCCESS != (result = xlDestroyDriverConfig(localDriverConfig.configHandle))) {
        printf("ERROR : Could not destroy last DriverConfig snapshot [xlDestroyDriverConfig]. Reason: %s.\n", xlGetErrorString(result));
        throw XlApiException(result);
      }
    }
    else {
      printf("ERROR : Could not get network config [GetNetEthNetworkConfig]. Reason: %s.\n", xlGetErrorString(result));
      throw XlApiException(result);
    }
  }

  //--------------------------------------------------------------------------------------------

  static void ShowNetwork(const XLnetworkDrvConfigListV1& pNetworkConfigList, int networkId = -1) {
    if (networkId == -1) {
      printf("\n+-------------------------------------------------------------------------------------------------------+\n");
      printf("|                                     Network Configuration (ALL [%2u])                                  |\n", pNetworkConfigList.count);
    }
    else {
      printf("\n+-------------------------------------------------------------------------------------------------------+\n");
      printf("|                                    Network Configuration (ID:%2u)                                      |\n", networkId);
    }

    printf("+----+---------------------------------+-------+---------------------------------+--------+------+------+\n");
    printf("| ID |              Name               | SegId |             SegName             |  Type  | #MPs | #VPs |\n");
    printf("+----+---------------------------------+-------+---------------------------------+--------+------+------+\n");

    for (unsigned int netIdx = 0; netIdx < pNetworkConfigList.count; netIdx++) {
      if (networkId <= -1) { /* show all networks on the device */ }
      else {
        if (netIdx != (unsigned int)networkId) {
          /* Show only network with ID listed in console printout */
          continue;
        }
      }

      auto& network = pNetworkConfigList.item[netIdx];
      const char *netName = network.networkName;

      for (unsigned short segmentIdx = 0; segmentIdx < network.switchList.count; segmentIdx++) {
        auto& segment = network.switchList.item[segmentIdx];
        const char *segName = segment.switchName;

        printf("| %3i|", netIdx);

        const char HEADER_NETNAME[] = "              Name               ";
        const char HEADER_SEGNAME[] = "             SegName             ";

        int space = (sizeof(HEADER_NETNAME) - strlen(netName)) / (2 * sizeof(char));
        if (space < 0) {
          space = 0;
        }
        for (int i = 0; i < space; i++) printf(" ");
        printf("%s", netName);

        space = ((sizeof(HEADER_NETNAME) - strlen(netName)) / sizeof(char)) - space - 1;
        if (space < 0) {
          space = 0;
        }
        for (int i = 0; i < space; i++) printf(" ");

        printf("| %6i|", segment.switchId);

        space = (sizeof(HEADER_SEGNAME) - strlen(segName)) / (2 * sizeof(char));
        for (int i = 0; i < space; i++) printf(" ");
        printf("%s", segName);
        space = ((sizeof(HEADER_SEGNAME) - strlen(segName)) / sizeof(char)) - space - 1;
        for (int i = 0; i < space; i++) printf(" ");

        switch (segment.switchCapability) {
          case XL_NET_ETH_SWITCH_CAP_DIRECTCONN:
            printf("| DIRECT |");
            break;
          case XL_NET_ETH_SWITCH_CAP_REALSWITCH:
            printf("| SWITCH |");
            break;
          case XL_NET_ETH_SWITCH_CAP_TAP_LINK:
            printf("|   TAP  |");
            break;
          case XL_NET_ETH_SWITCH_CAP_MULTIDROP:
            printf("|MUL_DROP|");
            break;
        }
        printf(" %5i|", segment.mpList.count);
        printf(" %5i|\n", segment.vpList.count);
      }
    }

    printf("+----+---------------------------------+-------+---------------------------------+--------+------+------+\n");
  }

  int ChooseNetwork(const XLnetworkDrvConfigListV1& networks) {
    DWORD          numberOfEventsRead = 0;
    INPUT_RECORD   input;

    printf("Choose Network to connect to: ");

    while (true) {
      if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &numberOfEventsRead)) {
        printf("read failed \n");
        throw std::runtime_error("Could not read from console.");
      }
      else if (numberOfEventsRead > 0 && input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
        printf("\n");
        auto c = input.Event.KeyEvent.uChar.AsciiChar;
        if (c == EscKeyCode) {
          return -1;
        }
        else if (c >= '0' && c <= '9') {
          // Remove ASCII offset
          unsigned int networkIdx = c - '0';
          if (networkIdx < networks.count) {
            // Check if demo can open this network.
            auto& network = networks.item[networkIdx];
            if (network.networkType == XL_ETH_NETWORK &&
                network.statusCode == XL_NET_CFG_STAT_OK) {
              printf("Selected network %u %s\n", networkIdx, network.networkName);
              return networkIdx;
            }
            else if (network.networkType != XL_ETH_NETWORK) {
              printf("Cannot select %s because it is not an Ethernet network.\n", network.networkName);
            }
            else {
              printf("Cannot select %s because of configuration error: %s.\n", network.networkName, network.statusErrorString);
            }
          }
        }

        printf("Enter a network ID in range 0..%u or ESC to abort.\n", networks.count);
        printf("Choose Network to connect to: ");
      }
    }
  }

  const XLswitchDrvConfigV1* ChooseSegmentForAddVp() {
    printf("Choose Segment to manually attach VP: ");
    DWORD          numberOfEventsRead = 0;
    INPUT_RECORD   input;

    while (true) {
      if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &numberOfEventsRead)) {
        printf("read failed \n");
        throw std::runtime_error("Could not read from console.");
      }
      else if (numberOfEventsRead > 0 && input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
        printf("\n");
        auto c = input.Event.KeyEvent.uChar.AsciiChar;
        if (c == EscKeyCode) {
          return nullptr;
        }
        auto& networkConfig = mNetworkList.item[mSelectedNetworkIdx];
        switch (c) {
          case '0': // Select Segment ID
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            if (c - (unsigned)'0' < networkConfig.switchList.count) {
              auto selectedSegment = c - '0';
             
              // Lookup segment by switchId
              auto& segment = networkConfig.switchList.item[selectedSegment];

              // Check if this segment can accommodate another VP
              if (segment.switchCapability == XL_NET_ETH_SWITCH_CAP_TAP_LINK) {
                printf("Virtual ports cannot be added to TAPs.\n");
              }
              else if (segment.switchCapability == XL_NET_ETH_SWITCH_CAP_DIRECTCONN && segment.vpList.count > 0) {
                printf("The selected TAP already has a VP attached.\n");
              }
              else {
                printf("Selected segment %u.\n", segment.switchId);
                return &segment;
              }
            }
            else {
              printf("Max available Segments (%i)\n", networkConfig.switchList.count);
            }
            break;
          default:
            printf("Enter one of the segIDs above or ESC to skip this step.\n");
            break;
        }
      }
    }
    printf("\n");
  } // ChooseSegmentForAddVp

  /** Abort demo as no network is configured. */
  static void AbortDueToMissingNetwork() {
    printf("Found 0 Ethernet networks. Please ensure that a device operating in network-based mode is connected\n");
    printf("and use Vector Hardware Configuration to configure a network on that device.\n");
    printf("Press any key to quit.\n");
    _getch();
  }

  /** Abort demo as no MPs or VPs are available. */
  static void AbortDueToMissingPorts() {
    printf("No virtual ports could be added or opened and no measurement point was connected.\n");
    printf("Please check the network configuration.\n");
    printf("Press any key to quit.\n");
    _getch();
  }

  /** Add ports to the network. */
  void AddOpenOrConnectVPandMP() {
    XLstatus result;

    // For simplicity, assign XlRxHandles sequentially, starting with 1.
    XLrxHandle nextRxHandle = (XLrxHandle)1;

    // If the user selected a segment for adding a VP, add it now.
    if (mSegmentToAddVpOn != nullptr) {
      // ---------------------------------------------------------------------------------------------------------------------------------------
      //                                               ADD VIRTUAL PORT [xlNetAddVirtualPort]
      // ---------------------------------------------------------------------------------------------------------------------------------------
      XLethPortHandle vpHandle;
      if (XL_SUCCESS != (result = xlNetAddVirtualPort(mNetworkHandle,
                                                      mSegmentToAddVpOn->switchName,
                                                      AddVpName.c_str(),
                                                      &vpHandle,
                                                      nextRxHandle))) {
        printf("ERROR : Failed to Add Virtual Port [xlNetAddVirtualPort]! on VP : %s)! ERR::%s (%d)\n", mSegmentToAddVpOn->switchName, xlGetErrorString(result), result);
        throw XlApiException(result);
      }

      // After adding a VP, it becomes part of the driver config. To show this, the demo creates a new driver config instance
      // and looks up the new VP. The demo also checks the switchId and network name, just in case that the config contains
      // a second VP with the same name in another network.
      XLapiIDriverConfigV1 tmpDriverCfg;
      XLvirtualportDrvConfigListV1 tmpVpCfgList;

      if (XL_SUCCESS != (result = xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig *)&tmpDriverCfg))) {
        printf("ERROR : Could not read driver config [xlCreateDriverConfig]. Reason: %s.\n", xlGetErrorString(result));
        throw XlApiException(result);
      }
      tmpDriverCfg.fctGetVirtualPortConfig(tmpDriverCfg.configHandle, &tmpVpCfgList);
      for (unsigned int vpIdx = 0; vpIdx < tmpVpCfgList.count; vpIdx++) {
        auto& vpConfig = tmpVpCfgList.item[vpIdx];

        if ((strcmp(vpConfig.virtualPortName, AddVpName.c_str()) == 0) &&
          (vpConfig.switchId == mSegmentToAddVpOn->switchId) &&
            (strcmp(mNetworkList.item[vpConfig.networkIdx].networkName, mNetworkList.item[mSelectedNetworkIdx].networkName) == 0)) {
          InsertOpenVp(&vpConfig, vpHandle, nextRxHandle);
          nextRxHandle++;
          printf("INFO : VP \"%s\" successfully manually added\n", vpConfig.virtualPortName);
        }
      }
      // NOTE: Call [xlDestroyDriverConfig] to destroy current snapshot - to prevent memory leakage
      if (XL_SUCCESS != (result = xlDestroyDriverConfig(tmpDriverCfg.configHandle))) {
        printf("ERROR : Could not destroy last DriverConfig snapshot [xlDestroyDriverConfig]. Reason: %s.\n", xlGetErrorString(result));
      }
    }

    //// ---------------------------------------------------------------------------------------------------------------------------------------
    ////                                              OPEN VIRTUAL PORT [xlNetOpenVirtualPort]
    //// ---------------------------------------------------------------------------------------------------------------------------------------
    // For demo purposes, try to open every pre-configured VP in the network. Note that this reads the mNetworks.switchList
    // from the first call of CreateDriverConfig(). Opening the VP that was just added would fail, because it is already opened.
    auto switchList = mNetworkList.item[mSelectedNetworkIdx].switchList;
    for (unsigned int segIdx = 0; segIdx < switchList.count; segIdx++) {
      auto& segmentConfig = switchList.item[segIdx];
      for (unsigned int vpIdx = 0; vpIdx < segmentConfig.vpList.count; vpIdx++) {
        auto& vpConfig = segmentConfig.vpList.item[vpIdx];
        
        XLethPortHandle vpHandle;
        if (XL_SUCCESS != (result = xlNetOpenVirtualPort(mNetworkHandle,
                                                         vpConfig.virtualPortName,
                                                         &vpHandle,
                                                         nextRxHandle))) {
          printf("ERROR : Failed to Open Virtual Port [xlNetOpenVirtualPort]! on VP : %s)! ERR::%s (%d)\n", vpConfig.virtualPortName, xlGetErrorString(result), result);
          throw XlApiException(result);
        }

        InsertOpenVp(&vpConfig, vpHandle, nextRxHandle);
        nextRxHandle++;
      }
    }

    //// ---------------------------------------------------------------------------------------------------------------------------------------
    ////                                       CONNECT MEASURING POINT [xlNetConnectMeasurementPoint]
    //// ---------------------------------------------------------------------------------------------------------------------------------------
    // For demo purposes, try to connect every MP in the network.
    for (unsigned int segIdx = 0; segIdx < switchList.count; segIdx++) {
      auto& segmentConfig = switchList.item[segIdx];
      for (unsigned int mpIdx = 0; mpIdx < segmentConfig.mpList.count; mpIdx++) {
        auto& mpConfig = segmentConfig.mpList.item[mpIdx];

        XLethPortHandle mpHandle;
        if (XL_SUCCESS != (result = xlNetConnectMeasurementPoint(mNetworkHandle,
                                                         mpConfig.measurementPointName,
                                                         &mpHandle,
                                                         nextRxHandle))) {
          printf("ERROR : Failed to Connect MP [xlNetConnectMeasurementPoint] on MP : %s! ERR::%s (%d)\n", mpConfig.measurementPointName, xlGetErrorString(result), result);
          throw XlApiException(result);
        }

        InsertConnectedMp(&mpConfig, mpHandle, nextRxHandle);
        nextRxHandle++;
      }
    }
  }

  /** Insert an open VP in the data structures of this demo. */
  void InsertOpenVp(const XLvirtualportDrvConfigV1* vpConfig, XLethPortHandle vpHandle, XLrxHandle rxHandle) {

    auto vp = std::make_shared<VpPort>(vpConfig);
    vp->PortHandle = vpHandle;
    vp->RxHandle = rxHandle;
    
    mOpenVPs.push_back(vp);
    mRxHandleMap[rxHandle] = vp;

    if (mCurrentVpIdx == -1)
    {
      mCurrentVpIdx = mOpenVPs.size() - 1;
    }
    printf("INFO: Inserted VP %s with rxHandle (%u)\n", vp->Name(), rxHandle);
  }

  /** Insert a connected MP in the data structures of this demo. */
  void InsertConnectedMp(const XLmeasurementpointDrvConfigV1* mpConfig, XLethPortHandle mpHandle, XLrxHandle rxHandle) {

    auto mp = std::make_shared<MpPort>(mpConfig);
    mp->PortHandle = mpHandle;
    mp->RxHandle = rxHandle;
    mConnectedMPs.push_back(mp);

    mRxHandleMap[rxHandle] = mp;

    if (mCurrentMpIdx == -1) {
      mCurrentMpIdx = mConnectedMPs.size() - 1;
    }
    printf("INFO: Inserted MP %s with rxHandle (%u)\n", mp->Name(), rxHandle);
  }

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

  std::string clockUuidToString(const XLtsClkUuid* uuid) {
    if (uuid == NULL) {
      return "Unknown";
    }

    switch (uuid->uuidFormat) {
      case XL_TS_CLK_UUID_FORMAT_UNDEFINED:
        return "Undefined";
      case XL_TS_CLK_UUID_FORMAT_VECTOR_DEV: {
        std::stringstream stream{};
        stream << "article:" << uuid->uuid.vectorDevUuid.articleNumber << " serial:" << uuid->uuid.vectorDevUuid.serialNumber
               << " clockId:" << uuid->uuid.vectorDevUuid.clkId;
        return stream.str();
      }
      case XL_TS_CLK_UUID_FORMAT_EUI64: {
        std::stringstream stream{};
        stream << std::hex << std::setfill('0');
        bool first = true;
        for (const unsigned char c : uuid->uuid.eui64Uuid.oui) {
          if (!first) {
            stream << "::";
          }
          stream << std::setw(2) << static_cast<int>(c);
          first = false;
        }
        for (const unsigned char c : uuid->uuid.eui64Uuid.extensionId) {
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

  void ResetClock() {
    // Reset clocks on network
    XLtsTimeScale   timescale;
    XLtsLeapSeconds leapSeconds;
    XLtsClkUuid     gmUuid;
    XLuint64        timeOffset = 0;

    XLstatus status = xlNetTsResetClocks(mNetworkHandle, &timescale, &leapSeconds, &gmUuid, &timeOffset);
    if (status != XL_SUCCESS) {
      printf("Reset clocks failed with status: %s\n", xlGetErrorString(status));
      return;
    }
    else {
      printf("Reset clocks on network: \n");
      printf("  Cluster master: %s\n", clockUuidToString(&gmUuid).c_str());
      printf("  Time scale: %s\n", timeScaleToString(timescale).c_str());
      printf("  Leap seconds: flags= 0x%X, value= %u\n", leapSeconds.leapSecondsFlags, leapSeconds.leapSecondsValue);
      printf("  TimeOffset: %llu \n ", timeOffset);
      printf("\n");
    }
  }

  // -----------------------------------------------------------------------------------------------
  // Handlers for input events and menu entries
  // -----------------------------------------------------------------------------------------------
  void HandleInputEvent(bool& shallQuit) {
    DWORD          numberOfEventsRead = 0;
    INPUT_RECORD   input;

    if (!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &numberOfEventsRead)) {
      printf("read failed \n");
      throw std::runtime_error("Could not read from console.");
    }
    else if (numberOfEventsRead > 0 && input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown) {
      CHAR inputChar = input.Event.KeyEvent.uChar.AsciiChar;
      switch (inputChar) {
        case 0:   // Special keys (Ctrl, Shift etc.)
          return;
        case '1': // Select Ethernet port
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          HandleSelectMp(inputChar - '1');
          break;
        case '+':
          HandlePrevNextMp(1);
          break;
        case '-':
          HandlePrevNextMp(-1);
          break;
        case '*':
          HandlePrevNextVp(1);
          break;
        case '/':
          HandlePrevNextVp(-1);
          break;
        case 't':
          HandleTransmitOnMp();
          break;
        case 'T':
          HandleTransmitOnVp();
          break;
        case 's':
          HandleRequestChannelStatus();
          break;
        case 'p':
          HandlePrintLastRxFrameReceived();
          break;
        case 'G':
          HandleRequestSourceMacAddress();
          break;
        case 'R':
          HandleReleaseSourceMacAddress();
          break;
        case 'm':
          HandleEnterDestinationMacAddress();
          break;
        case 'i':
          HandleEnterDestinationIpAddress();
          break;
        case 'w':
          ShowDriverConfig();
          break;
        case 'r':
          ResetClock();
          break;
        case 'h':
        case '?':
          ShowMenuHelp();
          break;
        case 'q':
          shallQuit = true;
          break;
        default:
          printf("Unknown command.\n");
          break;
      }
      printf("> ");
    }
  }

  void HandleSelectMp(unsigned int mpIdx) {
    if (mpIdx < mConnectedMPs.size()) {
      mCurrentMpIdx = mpIdx;
      printf("Selected MP (%u) \"%s\".\n", mpIdx + 1, mConnectedMPs[mpIdx]->Name());
    }
    else {
      printf("Unknown MP in the network. Max MP #%i\n", mConnectedMPs.size());
    }
  }

  /**
   * Select the next or previous measurement point.
   * 
   * \param offset -1 for the previous or +1 for the next MP.
   */
  void HandlePrevNextMp(int offset) {
    if (mConnectedMPs.size() > 0) {
      mCurrentMpIdx = (mConnectedMPs.size() + mCurrentMpIdx + offset) % mConnectedMPs.size();
      printf("Selected MP (%u) \"%s\".\n", mCurrentMpIdx + 1, mConnectedMPs[mCurrentMpIdx]->Name());
    }
    else {
      printf("No connected MPs available.\n");
    }
  }

  /**
   * Select the next or previous virtual port.
   * 
   * \param offset -1 for the previous or +1 for the next VP.
   */
  void HandlePrevNextVp(int offset) {
    if (mOpenVPs.size() > 0) {
      mCurrentVpIdx = (mOpenVPs.size() + mCurrentVpIdx + offset) % mOpenVPs.size();
      printf("Selected VP (%u) \"%s\".\n", mCurrentVpIdx + 1, mOpenVPs[mCurrentVpIdx]->Name());
    }
    else {
      printf("No open VPs available.\n");
    }
  }

  /** Transmit a single frame on the currently selected VP. */
  void HandleTransmitOnVp() {
    printf("Transmitting on VP\n");
    if (mCurrentVpIdx >= 0) {
      SendSingleFrame(mOpenVPs[mCurrentVpIdx]);
    }
    else {
      printf("No open VP available.\n");
    }
  }

  /** Transmit a single frame on the currently selected MP. */
  void HandleTransmitOnMp() {
    printf("Transmitting on MP\n");
    if (mCurrentMpIdx >= 0) {
      SendSingleFrame(mConnectedMPs[mCurrentMpIdx]);
    }
    else {
      printf("No connected MP available.\n");
    }
  }

  /** Request a XlEthChannelStatus event from every open measurement point in the network. */
  void HandleRequestChannelStatus() {
    xlNetEthRequestChannelStatus(mNetworkHandle);
  }

  /** Prints information on the frame that is stored in mLastRxFrameReceived. */
  void HandlePrintLastRxFrameReceived() {
    if (mLastRxFrameReceived.dataLen < XL_ETH_PAYLOAD_SIZE_MIN + EtherTypeSize) {
      printf("ERROR: No or incomplete frame received.\n");
      return;
    }

    auto frameRx = &mLastRxFrameReceived;

    printf("----------------------\n");
    printf("MAC Src: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
           frameRx->sourceMAC[0],
           frameRx->sourceMAC[1],
           frameRx->sourceMAC[2],
           frameRx->sourceMAC[3],
           frameRx->sourceMAC[4],
           frameRx->sourceMAC[5]);
    printf("MAC Dst: 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
           frameRx->destMAC[0],
           frameRx->destMAC[1],
           frameRx->destMAC[2],
           frameRx->destMAC[3],
           frameRx->destMAC[4],
           frameRx->destMAC[5]);
    printf("----------------------\n");
    printf("----------------------\n");
    printf("IP Src: %u.%u.%u.%u\n",
           frameRx->frameData.ethFrame.payload[12],
           frameRx->frameData.ethFrame.payload[13],
           frameRx->frameData.ethFrame.payload[14],
           frameRx->frameData.ethFrame.payload[15]);
    printf("IP Dst: %u.%u.%u.%u\n",
           frameRx->frameData.ethFrame.payload[16],
           frameRx->frameData.ethFrame.payload[17],
           frameRx->frameData.ethFrame.payload[18],
           frameRx->frameData.ethFrame.payload[19]);
    printf("----------------------\n");
    printf("Raw payload\n");
    printf("----------------------\n");
    for (int byte = 0; byte < frameRx->dataLen; byte++) {
      printf("0x%02X ", frameRx->frameData.ethFrame.payload[byte]);
      if (byte % 8 == 7)
        printf("\n");
    }
    printf("\n");
    printf("----------------------\n");
    printf("Payload:\n");
    printf("----------------------\n");
    for (int byte = IpV4HeaderSize + UdpHeaderSize; byte < IpV4HeaderSize + UdpHeaderSize + mCurrentPayloadSize; byte++) {
      printf("%c", frameRx->frameData.ethFrame.payload[byte]);
    }
    printf("\n");
    printf("----------------------\n");

  }

  /** Reserve a globally unique MAC address from the network. */
  void HandleRequestSourceMacAddress() {
    XLstatus result;
    if (!mSrcMacObtained) {
      if (XL_SUCCESS != (result = xlNetRequestMACAddress(mNetworkHandle, &mSourceMac))) {
        printf("ERROR : Failed to obtain source MAC Address : %s (%i)\n", xlGetErrorString(result), result);
        throw XlApiException(result);
      }
      else {
        printf("New Src MAC Address : 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n",
               mSourceMac.address[0],
               mSourceMac.address[1],
               mSourceMac.address[2],
               mSourceMac.address[3],
               mSourceMac.address[4],
               mSourceMac.address[5]);
        mSrcMacObtained = true;
      }
    }
    else {
      printf("INFO : One MAC Address was already taken\n");
    }
  }

  /** Releases the source MAC address that was previously reserved. */
  void HandleReleaseSourceMacAddress() {
    XLstatus result;
    if (mSrcMacObtained) {
      if (XL_SUCCESS != (result = xlNetReleaseMACAddress(mNetworkHandle, &mSourceMac))) {
        printf("ERROR : Failed to release source MAC address to the network MAC pool: %s (%i)\n", xlGetErrorString(result), result);
        throw XlApiException(result);
      }
      else {
        printf("INFO : MAC Address has been successfully released\n");
        mSrcMacObtained = false;
      }
    }
    else {
      printf("INFO : No MAC Address to release.\n");
    }
  }

  /** Lets the user set a different destination MAC address. */
  void HandleEnterDestinationMacAddress() {
    int  remoteMAC[XL_ETH_MACADDR_OCTETS];
    int  scanResult = 0;
    char inputMAC[64];

    printf("Enter destination MAC Address:\n");
    int i = 0;
    do {
      scanf_s("%c", &inputMAC[i], (unsigned)_countof(inputMAC));
      i++;
    } while (inputMAC[i - 1] != '\n' && i < 64);

    if (i < 64)
      scanResult = sscanf_s(inputMAC, "%2x%*1[:-]%2x%*1[:-]%2x%*1[:-]%2x%*1[:-]%2x%*1[:-]%2x", &remoteMAC[0], &remoteMAC[1], &remoteMAC[2], &remoteMAC[3], &remoteMAC[4], &remoteMAC[5]);

    switch (scanResult) {
      // Expect six octets
      case 6:

        for (unsigned int octet = 0; octet < XL_ETH_MACADDR_OCTETS; ++octet) {
          if (remoteMAC[octet] <= 255) {
            mRemoteMac.address[octet] = (unsigned char)remoteMAC[octet];
          }
          else {
            printf("ERROR : Given destination MAC is invalid!\n");
            break;
          }
        }
        printf("INFO : MAC address ok.\n");
        break;
      default:
        printf("ERROR : Destination MAC has wrong format!\n");
        break;
    }
  }

  /** Lets the user set a different destination IP. */
  void HandleEnterDestinationIpAddress() {
    int  remoteIP[4];
    int  scanResult = 0;
    char inputIP[32];

    printf("Enter destination IP Address:\n");
    int i = 0;
    do {
      scanf_s("%c", &inputIP[i], (unsigned)_countof(inputIP));
      i++;
    } while (inputIP[i - 1] != '\n' && i < 32);

    if (i < 64)
      scanResult = sscanf_s(inputIP, "%3d%*1[.-:]%3d%*1[.-:]%3d%*1[.-:]%3d", &remoteIP[0], &remoteIP[1], &remoteIP[2], &remoteIP[3]);

    switch (scanResult) {
      case 4:
        for (unsigned int quartet = 0; quartet < 4; ++quartet) {
          if (remoteIP[quartet] <= 255) {
            mRemoteIp[quartet] = (unsigned char)remoteIP[quartet];
          }
          else {
            printf("ERROR : Given destination IP is invalid!\n");
            break;
          }
        }
        printf("INFO : IP address ok.\n");
        break;
      default:
        printf("ERROR : Destination IP has wrong format!\n");
        break;
    }
  }

  // -----------------------------------------------------------------------------------------------
  // XL-API Event handling
  // -----------------------------------------------------------------------------------------------
  /** Writes a line of output for every event received. */
  XLstatus HandleXLApiEvent() {
    XLstatus result = XL_SUCCESS;

    do {
      T_XL_NET_ETH_EVENT rxEvent;
      XLrxHandle         rxHandles[MAX_USER_HANDLE];
      unsigned int       rxCount = MAX_USER_HANDLE;

      // ---------------------------------------------------------------------------------------------------------------------------------------
      //                                              DATA & EVENT RECEIVE [xlNetEthReceive]
      // ---------------------------------------------------------------------------------------------------------------------------------------
      result = xlNetEthReceive(mNetworkHandle, &rxEvent, &rxCount, rxHandles);
      if (XL_ERR_QUEUE_IS_EMPTY == result) {
        //  XL_ERR_QUEUE_IS_EMPTY is normal, but any other return code is a problem.
        return result;
      }
      else if (XL_ERR_INSUFFICIENT_BUFFER == result) {
        printf("Event receive: Insufficient buffer\n");
        return result;
      }
      else if (XL_SUCCESS != result) {
        printf("Event receive: Error 0x%x\n", result);
        return result;
      }

      if (rxEvent.flagsChip & XL_ETH_QUEUE_OVERFLOW) {
        printf("Event receive: Overflow\n");
        return result;
      }

      switch (rxEvent.tag) {
        case XL_ETH_EVENT_TAG_FRAMERX_ERROR_SIMULATION:
        case XL_ETH_EVENT_TAG_FRAMETX_ERROR_SIMULATION:
        case XL_ETH_EVENT_TAG_FRAMETX_ACK_SIMULATION:
        case XL_ETH_EVENT_TAG_FRAMERX_SIMULATION:
        case XL_ETH_EVENT_TAG_FRAMERX_ERROR_MEASUREMENT:
        case XL_ETH_EVENT_TAG_FRAMETX_ERROR_MEASUREMENT:
        case XL_ETH_EVENT_TAG_FRAMETX_MEASUREMENT:
        case XL_ETH_EVENT_TAG_FRAMERX_MEASUREMENT:
          HandleEthernetEvent(&rxEvent, rxCount, rxHandles);
          break;
        case XL_ETH_EVENT_TAG_CHANNEL_STATUS: {
          T_XL_ETH_CHANNEL_STATUS* pChStatus = &rxEvent.tagData.channelStatus;
          for (unsigned int i = 0; i < rxCount; i++) {
            printf("Link on %s : LINK % s\n", mRxHandleMap[rxHandles[i]]->Name(), (unsigned int)pChStatus->link == XL_ETH_STATUS_LINK_UP ? "UP" : "DOWN");
          }
          break;
        }
        default:
          printf("%u Unknown Event 0x%x\n", (unsigned long)rxEvent.timeStampSync, rxEvent.tag);
          break;
      } // End switch

    } while (result == XL_SUCCESS);

    return result;
  }

  /** Handles Ethernet-related XL-API events. */
  void HandleEthernetEvent(const T_XL_NET_ETH_EVENT *pRxEvent, unsigned int rxHandleCount, XLrxHandle* pRxHandle) {

    printf("Received ");
    switch (pRxEvent->tag) {
      case XL_ETH_EVENT_TAG_FRAMERX_ERROR_SIMULATION:
        printf("FrameRxErrorSimulation (Error Flags: 0x%08x) ", pRxEvent->tagData.frameSimRxError.errorFlags);
        break;
      case XL_ETH_EVENT_TAG_FRAMETX_ERROR_SIMULATION:
        printf("FrameTxErrorSimulation (Error Flags: 0x%08x) ", pRxEvent->tagData.frameSimTxError.errorFlags);
        break;
      case XL_ETH_EVENT_TAG_FRAMETX_ACK_SIMULATION:
        printf("FrameTxAckSimulation ");
        break;
      case XL_ETH_EVENT_TAG_FRAMERX_SIMULATION:
        printf("FrameRxSimulation ");
        memcpy(&mLastRxFrameReceived, &pRxEvent->tagData.frameSimRx, sizeof(T_XL_NET_ETH_DATAFRAME_RX));
        break;
      case XL_ETH_EVENT_TAG_FRAMERX_ERROR_MEASUREMENT:
        printf("FrameRxErrorMeasurement (Error Flags: 0x%08x) ", pRxEvent->tagData.frameMeasureRxError.errorFlags);
        break;
      case XL_ETH_EVENT_TAG_FRAMETX_ERROR_MEASUREMENT:
        printf("FrameTxErrorMeasurement (Error Flags: 0x%08x) ", pRxEvent->tagData.frameMeasureTxError.errorFlags);
        break;
      case XL_ETH_EVENT_TAG_FRAMETX_MEASUREMENT:
        printf("FrameTxMeasurement ");
        break;
      case XL_ETH_EVENT_TAG_FRAMERX_MEASUREMENT:
        printf("FrameRxMeasurement ");
        memcpy(&mLastRxFrameReceived, &pRxEvent->tagData.frameMeasureRx, sizeof(T_XL_NET_ETH_DATAFRAME_RX));
        break;
      default:
        printf("Unknown Event\n");
        break;
    }

    if (rxHandleCount > 0) {
      printf("on ");
      for (unsigned int i = 0; i < rxHandleCount; i++) {
        printf("%s", mRxHandleMap[pRxHandle[i]]->Name());
        if (i + 1 < rxHandleCount) {
          printf(", ");
        }
      }
      printf("\n");
    }
  } // HandleEthernetEvent

    /** Return a human-readable representation of the link speed. */
  static const char *GetChannelStatusSpeedName(unsigned int speed) {
    const char *name = NULL;

    // values from T_XL_ETH_CHANNEL_STATUS event
    switch (speed) {
      case XL_ETH_STATUS_SPEED_UNKNOWN:     name = "unknown";         break;
      case XL_ETH_STATUS_SPEED_10:          name = "10 MBit";         break;
      case XL_ETH_STATUS_SPEED_100:         name = "100 MBit";        break;
      case XL_ETH_STATUS_SPEED_1000:        name = "1 GBit";          break;
      default:                              name = "invalid";         break;
    }

    return name;
  } //GetChannelStatusSpeedName

  /** Return a human-readable representation of connector type. */
  static const char *GetChannelStatusConnectorName(unsigned int connector) {
    const char *name = NULL;

    switch (connector) {
      case XL_ETH_STATUS_CONNECTOR_DSUB:    name = "DSub";            break;
      case XL_ETH_STATUS_CONNECTOR_RJ45:    name = "RJ45";            break;
      default:                              name = "n/a";             break;
    }

    return name;
  } //GetChannelStatusConnectorName

  /** Return a human-readable representation of clock synchronization mode. */
  static const char *GetChannelStatusClockModeName(unsigned int mode) {
    const char *name = NULL;

    switch (mode) {
      case XL_ETH_STATUS_CLOCK_DONT_CARE:   name = "DC";              break;
      case XL_ETH_STATUS_CLOCK_MASTER:      name = "Ma";              break;
      case XL_ETH_STATUS_CLOCK_SLAVE:       name = "Sl";              break;
      default:                              name = "n/a";             break;
    }

    return name;
  } //GetChannelStatusClockModeName

  // -----------------------------------------------------------------------------------------------

  /** Calculate the IPv4 checksum as the 16-bit one's complement of the one's complement sum of all 16 bit words in the header. */
  unsigned short CalculateIPv4HeaderChecksum(const unsigned char *pHeader, unsigned int headerOctets) {
    unsigned int   wordSum = 0;
    unsigned short onesComplement = 0;
    unsigned int   octet = 0;

    // Sanity check
    if (nullptr == pHeader)
      return 0;

    // Calculate sum of all 16-bit word values
    for (octet = 0; octet < headerOctets; octet += 2) {
      wordSum += pHeader[octet] << 8;
      wordSum += pHeader[octet + 1];
    }
    onesComplement = ~((wordSum >> 16) + (wordSum & 0xffff));
    return onesComplement;
  }

  /** Write an IPv4 header and a UDP header. */
  unsigned char FrameAddIpV4UdpHeader(T_XL_ETH_DATAFRAME_TX *pFrame, unsigned char* srcIP, unsigned char* dstIP) {
    unsigned char *pPayload;

    // Sanity check
    if (nullptr == pFrame)
      return FALSE;
    if (pFrame->dataLen > (XL_ETH_RAW_FRAME_SIZE_MAX - 2))
      return FALSE;
    if (pFrame->dataLen < (46 + 2)) // Check for Ethernet minimum size
      return FALSE;

    pPayload = pFrame->frameData.ethFrame.payload;

    // IPv4 Header
    unsigned int idx = 0;
    unsigned int startIdx = 0;
    pPayload[idx++] = IpV4VersionIhlField;                 // Version = 0x40, Header length = 5x32bits = 20 Bytes
    pPayload[idx++] = (unsigned char)0;                              // Type of service
    pPayload[idx++] = (unsigned char)((pFrame->dataLen - 2) >> 8);   // MSB of IP packet length, including IP header  
    pPayload[idx++] = (unsigned char)((pFrame->dataLen - 2) & 0xff); // LSB of IP packet length, including IP header
    pPayload[idx++] = 0x00;                        // MSB Identification
    pPayload[idx++] = 0x00;                        // LSB Identification
    pPayload[idx++] = 0x00;                        // Flags, Fragment offset
    pPayload[idx++] = 0x00;                        // Fragment offset
    pPayload[idx++] = (unsigned char)0;            // TTL
    pPayload[idx++] = IpProtocolUdp;                        // Protocol (TCP=6, UDP=17)
    pPayload[idx++] = 0x00;                        // MSB Header checksum
    pPayload[idx++] = 0x00;                        // LSB Header checksum
    pPayload[idx++] = (unsigned char)(srcIP[0]);   // IP Source address [__:xx:xx:xx]
    pPayload[idx++] = (unsigned char)(srcIP[1]);   // IP Source address [xx:__:xx:xx]
    pPayload[idx++] = (unsigned char)(srcIP[2]);   // IP Source address [xx:xx:__:xx]
    pPayload[idx++] = (unsigned char)(srcIP[3]);   // IP Source address [xx:xx:xx:__]
    pPayload[idx++] = (unsigned char)(dstIP[0]);   // IP Destination address [__:xx:xx:xx]
    pPayload[idx++] = (unsigned char)(dstIP[1]);   // IP Destination address [xx:__:xx:xx]
    pPayload[idx++] = (unsigned char)(dstIP[2]);   // IP Destination address [xx:xx:__:xx]
    pPayload[idx++] = (unsigned char)(dstIP[3]);   // IP Destination address [xx:xx:xx:__]
    // Calculate IPv4 header checksum
    unsigned short IPv4checksum = CalculateIPv4HeaderChecksum(&pPayload[startIdx], idx);
    pPayload[startIdx + 10] = (unsigned char)(IPv4checksum / 256);
    pPayload[startIdx + 11] = IPv4checksum % 256;

    // Add UDP header
    pPayload[idx++] = (unsigned char)(UdpSourcePort >> 8);  // >> 8); // MSB of UDP source port
    pPayload[idx++] = (unsigned char)(UdpSourcePort);  //; // LSB of UDP source port
    pPayload[idx++] = (unsigned char)(UdpRemotePort >> 8); // >> 8); // MSB of UDP destination port
    pPayload[idx++] = (unsigned char)(UdpRemotePort); //; // LSB of UDP destination port
    pPayload[idx++] = ((pFrame->dataLen - (IpV4HeaderSize + 2)) >> 8) & 0xff; //MSB of UDP packet length (including UDP header)
    pPayload[idx++] = ((pFrame->dataLen - (IpV4HeaderSize + 2)) & 0xff);      //LSB of UDP packet length (including UDP header)
    pPayload[idx++] = 0x00;                         // MSB of UDP packet checksum (0x0000 = no checksum calculated)
    pPayload[idx++] = 0x00;                         // LSB of UDP packet checksum (0x0000 = no checksum calculated)

    return TRUE;
  }

  /** Send a single ethernet frame with a IPv4 Header, a UDP header and a string as payload. */
  XLstatus SendSingleFrame(std::shared_ptr<Port> portToSendOn) {

    // Allocate a new, zero-filled DataFrameTx on stack
    T_XL_ETH_DATAFRAME_TX frame;
    memset(&frame, 0, sizeof(T_XL_ETH_DATAFRAME_TX));

    // -----  FRAME LENGTH ------
    // Ensure the frame has at least the minimum length. +1 reserves a byte for the null-termination of the string.
    int dataLen = max((EtherTypeSize + IpV4HeaderSize + UdpHeaderSize + mCurrentPayloadSize + 1), (int)(XL_ETH_PAYLOAD_SIZE_MIN + EtherTypeSize));
    if (dataLen > XL_ETH_PAYLOAD_SIZE_MAX) {
      throw std::runtime_error("Invalid payload length");
    }

    frame.dataLen = (unsigned short)dataLen;
    printf("Data %u (total %u) sending on %s %s\n", mCurrentPayloadSize, frame.dataLen, portToSendOn->Type(), portToSendOn->Name());

    
    // -----  MAC ADDRESS CONFIG ------
    // The UseSourceMac flag must be set to transmit in network-based mode.
    frame.flags |= XL_ETH_DATAFRAME_FLAGS_USE_SOURCE_MAC;
    if (mSrcMacObtained == false) {
      // If the user did not acquire a unique MAC for sending, the demo tries to emulate the behavior without UseSourceMac, by using the MAC
      // address that is assigned to the channel. This on only possible on MPs as VPs are not associated to a channel. Note that on the recent
      // devices, like the VN5620, which only support the network based mode, the MAC address of automotive Ethernet channels is zero.

      // Use dynamic_cast to check if the portToSendOn is a MpPort
      auto mp = std::dynamic_pointer_cast<MpPort>(portToSendOn);
      if (mp != nullptr) {
        memcpy(frame.sourceMAC, mp->mMacAddress.address, XL_ETH_MACADDR_OCTETS);
      }
    }
    else {
      memcpy(frame.sourceMAC, mSourceMac.address, XL_ETH_MACADDR_OCTETS);
    }
    memcpy(frame.destMAC, mRemoteMac.address, XL_ETH_MACADDR_OCTETS);

    frame.frameData.ethFrame.etherType = EthertypeIpV4Be;

    // -----  IPv4 / UDP HEADER ------
    // Write a simple IPv4 and UDP header at the start of the frame. The LSB of the SourceIP is set to the RxHandle of the sending port.
    mIPSrc[3] = (unsigned char)portToSendOn->RxHandle;
    FrameAddIpV4UdpHeader(&frame, mIPSrc, mRemoteIp);

    // -----  ADD DATA PAYLOAD ------
    unsigned int headerLength = (IpV4HeaderSize + UdpHeaderSize + EtherTypeSize);
    memcpy(&frame.frameData.rawData[mCurrentPayloadSize - headerLength], &headerLength, headerLength);

    for (unsigned int i = headerLength; i < (mCurrentPayloadSize + headerLength); ++i) {
      frame.frameData.rawData[i] = mEthPayload[i - headerLength];
    }

    // -----  SEND ON MP/VP PORT ------
    return xlNetEthSend(mNetworkHandle, portToSendOn->PortHandle, 0, &frame);
  }

  /** Requests Ethernet device config from XL-API. */
  XLstatus GetNetEthDeviceConfig(XLapiIDriverConfigV1 *pDriverConfig, XLdeviceDrvConfigListV1 *pDeviceConfigList) {
    XLstatus result = XL_SUCCESS;

    // List all available Devices connected to the PC
    if (XL_SUCCESS != (result = pDriverConfig->fctGetDeviceConfig(pDriverConfig->configHandle, pDeviceConfigList))) {
      printf("ERROR : Could not get device config [fctGetDeviceConfig]. Reason: %s.\n", xlGetErrorString(result));
    }
    else {
      printf("Found %i devices\n", pDeviceConfigList->count);
      for (unsigned int device = 0; device < pDeviceConfigList->count; device++) {
        // NOTE:  .hwIndex+1 as .hwIndex numbering starts from 0
        printf("[%i] Found %s:%i(%06i) device\n", device, pDeviceConfigList->item[device].name, pDeviceConfigList->item[device].hwIndex + 1, pDeviceConfigList->item[device].serialNumber);
      }
      for (unsigned int deviceID = 0; deviceID < pDeviceConfigList->count; deviceID++) {
        /* Show only devices with Ethernet Ports */
        bool containesEthPort = false;
        for (unsigned int devicePort = 0; devicePort < pDeviceConfigList->item[deviceID].channelList.count; devicePort++) {
          if (pDeviceConfigList->item[deviceID].channelList.item[devicePort].busParams.busType == XL_BUS_TYPE_ETHERNET) {
            containesEthPort = true;
            break;
          }
        }

        if (!containesEthPort)
          continue;

        printf("Device : %s\n", pDeviceConfigList->item[deviceID].name);
        printf("---------------------------------------------------------------------------\n");
        // NOTE:  .hwIndex+1 as .hwIndex numbering starts from 0
        printf("%s : %2i (%06i)              Hardware Configuration\n", pDeviceConfigList->item[deviceID].name, pDeviceConfigList->item[deviceID].hwIndex + 1, pDeviceConfigList->item[deviceID].serialNumber);
        printf("---------------------------------------------------------------------------\n");
        printf("HwNr|HwCh|Transceiver             |MAC              |Link State            \n");
        printf("----+----+------------------------+-----------------+----------------------\n");
        for (unsigned int portIndex = 0; portIndex < pDeviceConfigList->item[deviceID].channelList.count; portIndex++) {
          const XLchannelDrvConfigV1 *channelConfig = &pDeviceConfigList->item[deviceID].channelList.item[portIndex];
          char                   linkState[100];

          switch (channelConfig->busParams.busType) {
            case XL_BUS_TYPE_ETHERNET:
              switch (channelConfig->busParams.data.ethernet.link) {
                case XL_ETH_STATUS_LINK_UNKNOWN:
                  strcpy_s(linkState, sizeof(linkState), "Unknown state");
                  break;
                case XL_ETH_STATUS_LINK_DOWN:
                  strcpy_s(linkState, sizeof(linkState), "Link down");
                  break;
                case XL_ETH_STATUS_LINK_UP:
                  switch (channelConfig->busParams.data.ethernet.phy) {
                    case XL_ETH_STATUS_PHY_UNKNOWN:
                      strcpy_s(linkState, sizeof(linkState), "Unknown PHY");
                      break;
                    case XL_ETH_STATUS_PHY_IEEE_802_3:
                      sprintf_s(linkState, sizeof(linkState), "IEEE %s, %s",
                                GetChannelStatusSpeedName(channelConfig->busParams.data.ethernet.speed),
                                GetChannelStatusConnectorName(channelConfig->busParams.data.ethernet.connector));
                      break;
                    case XL_ETH_STATUS_PHY_100BASE_T1:
                      sprintf_s(linkState, sizeof(linkState), "100BASE-T1 %s (%s), %s",
                                GetChannelStatusSpeedName(channelConfig->busParams.data.ethernet.speed),
                                GetChannelStatusClockModeName(channelConfig->busParams.data.ethernet.clockMode),
                                GetChannelStatusConnectorName(channelConfig->busParams.data.ethernet.connector));
                      break;
                    case XL_ETH_STATUS_PHY_1000BASE_T1:
                      sprintf_s(linkState, sizeof(linkState), "1000BASE-T1 %s (%s), %s",
                                GetChannelStatusSpeedName(channelConfig->busParams.data.ethernet.speed),
                                GetChannelStatusClockModeName(channelConfig->busParams.data.ethernet.clockMode),
                                GetChannelStatusConnectorName(channelConfig->busParams.data.ethernet.connector));
                      break;
                    default:
                      strcpy_s(linkState, sizeof(linkState), "Internal error");
                      break;
                  }
                  break;
                case XL_ETH_STATUS_LINK_ERROR:
                  strcpy_s(linkState, sizeof(linkState), "Link error");
                  break;
                default:
                  strcpy_s(linkState, sizeof(linkState), "Internal error");
                  break;
              }

              printf("%4u|%4u|%-24s|%02x:%02x:%02x:%02x:%02x:%02x|%s\n",
                     channelConfig->deviceIndex,//hwIndex,
                     channelConfig->hwChannel,
                     channelConfig->transceiver.name,
                     channelConfig->busParams.data.ethernet.macAddr[0], channelConfig->busParams.data.ethernet.macAddr[1],
                     channelConfig->busParams.data.ethernet.macAddr[2], channelConfig->busParams.data.ethernet.macAddr[3],
                     channelConfig->busParams.data.ethernet.macAddr[4], channelConfig->busParams.data.ethernet.macAddr[5],
                     linkState
              );
              break;
            default:
              // Do nothing.
              break;
          }
        }
        printf("----+----+------------------------+-----------------+----------------------\n");
        printf("\n");
      }
    }

    return result;
  }

  /** Requests Ethernet Network config from XL-API. */
  XLstatus GetNetEthNetworkConfig(XLapiIDriverConfigV1 *pDriverConfig, XLnetworkDrvConfigListV1 *pNetworkConfigList) {
    XLstatus result = XL_SUCCESS;
    // --------------------------------------------------------------------------------------------------------
    // xlCreateDriverConfig - make new snapshot of current running device configuration
    // .fctGetNetworkConfig - extract information about Networks from the read configuration
    // --------------------------------------------------------------------------------------------------------
    if (XL_SUCCESS != (result = xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig *) pDriverConfig))) {
      printf("ERROR : Could not read driver config [xlCreateDriverConfig]. Reason: %s.\n", xlGetErrorString(result));
    }
    else {
      // Networks
      result = pDriverConfig->fctGetNetworkConfig(pDriverConfig->configHandle, pNetworkConfigList);
      if (result != XL_SUCCESS) {
        printf("ERROR : Could not get network config [fctGetNetworkConfig]. Reason: %s.\n", xlGetErrorString(result));
      }
    }
    return result;
  }

  /** The main loop handles both incoming XL-API events and keyboard input. */
  void MainLoop() {
    DWORD    numWaitHandles = 0;
    HANDLE   waitHandles[2] = { NULL, NULL };
    XLstatus result;

    bool shallQuit = false;

    waitHandles[numWaitHandles++] = GetStdHandle(STD_INPUT_HANDLE);
    waitHandles[numWaitHandles++] = mNotificationHandle;
    while (!shallQuit) {
      DWORD waitResult = 0;
      DWORD waitTime = 10; // [ms]

      waitResult = WaitForMultipleObjects(numWaitHandles, waitHandles, FALSE, waitTime);
      switch (waitResult) {
        case WAIT_OBJECT_0:     // Input event (from console) received
          HandleInputEvent(shallQuit);
          break;
        case WAIT_OBJECT_0 + 1: // XL-API event received
          if (XL_SUCCESS != (result = HandleXLApiEvent())) {
          }
          break;
        default:
          break;
      }
    }
  }
  
};
} // namespace xlNetEthDemo

/** Entry point of the application */
int main() {
  try {
    auto demoApp = xlNetEthDemo::DemoApplication();
    demoApp.Run();
  }
  catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << std::endl;
    std::cerr << "Press any key to quit." << std::endl;
    _getch();
    return -1;
  }
  return 0;
}