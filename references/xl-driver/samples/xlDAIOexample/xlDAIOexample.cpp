/*------------------------------------------------------------------------------
| File:
|   xlDAIOexample.c
| Project:
|   IOcab/IOpiggy Test Application ()
|   Example application using 'vxlapi.dll'
|-------------------------------------------------------------------------------
|-------------------------------------------------------------------------------
| Copyright (c) 2014 by Vector Informatik GmbH.  All rights reserved.
 -----------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "vxlapi.h"

#include <string>

// analog lines
const unsigned int AIO0    = XL_DAIO_PORT_MASK_ANALOG_A0;
const unsigned int AIO1    = XL_DAIO_PORT_MASK_ANALOG_A1;
const unsigned int AIO2    = XL_DAIO_PORT_MASK_ANALOG_A2;
const unsigned int AIO3    = XL_DAIO_PORT_MASK_ANALOG_A3;
const unsigned int AIO_ALL = AIO0 | AIO1 | AIO2 | AIO3;

// digital lines
const unsigned int DIO0    = XL_DAIO_PORT_MASK_DIGITAL_D0;
const unsigned int DIO1    = XL_DAIO_PORT_MASK_DIGITAL_D1;
const unsigned int DIO2    = XL_DAIO_PORT_MASK_DIGITAL_D2;
const unsigned int DIO3    = XL_DAIO_PORT_MASK_DIGITAL_D3;
const unsigned int DIO4    = XL_DAIO_PORT_MASK_DIGITAL_D4;
const unsigned int DIO5    = XL_DAIO_PORT_MASK_DIGITAL_D5;
const unsigned int DIO6    = XL_DAIO_PORT_MASK_DIGITAL_D6;
const unsigned int DIO7    = XL_DAIO_PORT_MASK_DIGITAL_D7;
const unsigned int DIO_ALL = DIO0 | DIO1 | DIO2 | DIO3 | DIO4 | DIO5 | DIO6 | DIO7;

// masks for switches
const unsigned int SWITCH_DIO0_DIO01 = 0x01;

// Driver Variables
char           g_AppName[XL_MAX_LENGTH + 1]{"xlDAIOexample"};  // Application name which is displayed in VHWconf
XLportHandle   g_xlPortHandle{XL_INVALID_PORTHANDLE};          // Global porthandle (we use only one!)
XLdriverConfig g_xlDrvConfig;                                  // Contains the actual hardware configuration
XLaccess       g_xlChannelMask{0};                             // Global channelmask (includes all founded channels)
XLaccess       g_xlPermissionMask;                             // Global permissionmask (includes all founded channels)
unsigned short g_trcvType;                                     // Transceiver type on DAIO channel

// Thread and Event Variables
HANDLE   g_Thread;                       // Flag to start/stop the RX thread
XLhandle g_hMsgEvent;                    // Notification handle for the receive queue
int      g_ThreadRun{1};                 // controls RX thread
int      g_ioPiggyDigitalTriggerCyclic;  // Cyclic or on-edge digital trigger

// Measuring Variables
unsigned int g_switchState{0u};     // On IO-Cab: 0 = relay open, 1 = relay closed
double       g_sampleScaling{1.0};  // [mV] Voltage

const unsigned int g_frequency{1000u};  // Considered to be [ms]
const unsigned int g_outputMilliVolt{4096u};

constexpr unsigned int      receiveEventCount = 1;
constexpr const char* const drawLine          = "-------------------------------------------------------------------\n";

// This function deals really just with the data structure use by the depricated legacy cabs!
void displayCabEvent(const XLevent& xlEvent) {
  printf(" - Analog Port :       AIO0 |       AIO1 |       AIO2 |       AIO3 |\n");
  printf("                 -----------+------------+------------+------------+\n");
  printf("                 %7.1f mV ", xlEvent.tagData.daioData.value_analog[0] * g_sampleScaling);
  printf("| %7.1f mV ", xlEvent.tagData.daioData.value_analog[1] * g_sampleScaling);
  printf("| %7.1f mV ", xlEvent.tagData.daioData.value_analog[2] * g_sampleScaling);
  printf("| %7.1f mV |\n\n", xlEvent.tagData.daioData.value_analog[3] * g_sampleScaling);

  if (g_switchState) {
    printf(" - Switch      : CLOSED\n");
  }
  else {
    printf(" - Switch      : OPEN\n");
  }

  printf(" - Digital Port: DIO7|DIO6|DIO5|DIO4|DIO3|DIO2|DIO1|DIO0|(value)\n");
  printf("                 ----+----+----+----+----+----+----+----+\n");
  printf("                   %x |  %x |  %x |  %x |  %x |  %x |  %x |  %x | (%x)\n\n", (xlEvent.tagData.daioData.value_digital & 128) >> 7,
         (xlEvent.tagData.daioData.value_digital & 64) >> 6, (xlEvent.tagData.daioData.value_digital & 32) >> 5,
         (xlEvent.tagData.daioData.value_digital & 16) >> 4, (xlEvent.tagData.daioData.value_digital & 8) >> 3,
         (xlEvent.tagData.daioData.value_digital & 4) >> 2, (xlEvent.tagData.daioData.value_digital & 2) >> 1,
         (xlEvent.tagData.daioData.value_digital & 1), (xlEvent.tagData.daioData.value_digital));
}

// parsing of event data in case of the piggys and indeed the built-in DAIO transceivers, event though we do not call them piggys
void displayPiggyEvent(const XLevent& xlEvent) {
  if (xlEvent.tagData.daioPiggyData.daioEvtTag == XL_DAIO_EVT_ID_ANALOG) {
    printf(" - Analog Port :       AIO0 |       AIO1 |       AIO2 |       AIO3 |\n");
    printf("                 -----------+------------+------------+------------+\n");
    printf("                 %7.1f mV ", xlEvent.tagData.daioPiggyData.data.analog.measuredAnalogData0 * g_sampleScaling);
    printf("| %7.1f mV ", xlEvent.tagData.daioPiggyData.data.analog.measuredAnalogData1 * g_sampleScaling);
    printf("| %7.1f mV ", xlEvent.tagData.daioPiggyData.data.analog.measuredAnalogData2 * g_sampleScaling);
    printf("| %7.1f mV |\n\n", xlEvent.tagData.daioPiggyData.data.analog.measuredAnalogData3 * g_sampleScaling);
  }
  if (xlEvent.tagData.daioPiggyData.daioEvtTag == XL_DAIO_EVT_ID_DIGITAL) {
    printf(" - Digital out : %s\n", (g_switchState > 0) ? "HIGH" : "LOW");

    printf(" - Digital Port: DIO7|DIO6|DIO5|DIO4|DIO3|DIO2|DIO1|DIO0|(value)\n");
    printf("                 ----+----+----+----+----+----+----+----+\n");
    printf("                   %x |  %x |  %x |  %x |  %x |  %x |  %x |  %x | (%x)\n\n",
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 128) >> 7,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 64) >> 6,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 32) >> 5,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 16) >> 4,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 8) >> 3,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 4) >> 2,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 2) >> 1,
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData & 1),
           (xlEvent.tagData.daioPiggyData.data.digital.digitalInputData));
  }
}

DWORD WINAPI RxThread(LPVOID) {
  while (g_ThreadRun) {
    // wait for RX message event
    DWORD eventResult = WaitForSingleObject(g_hMsgEvent, 10);

    if (eventResult != WAIT_OBJECT_0) {
      continue;
    }

    // read hardware queue while not empty
    XLstatus xlStatus = XL_SUCCESS;
    while (!xlStatus) {
      unsigned int msgsrx = receiveEventCount;
      XLevent      xlEvent;
      xlStatus = xlReceive(g_xlPortHandle, &msgsrx, &xlEvent);

      if (xlStatus != XL_SUCCESS) {
        break;
      }

      //deal with depricated legacy DAIO cab data
      if (xlEvent.tag == XL_RECEIVE_DAIO_DATA) {
        displayCabEvent(xlEvent);
      }
      //deal with data coming from piggys AND built-in transceivers, that use the same data structure
      if (xlEvent.tag == XL_RECEIVE_DAIO_PIGGY) {
        displayPiggyEvent(xlEvent);
      }
    }
  }
  return NO_ERROR;
}

XLstatus daioInit() {
  printf("\nDRIVER INITIALIZATION\n%s", drawLine);

  // Open Driver
  XLstatus xlStatus = xlOpenDriver();
  if (xlStatus) {
    printf("\nERROR: xlOpenDriver failed!\n");
    return xlStatus;
  }
  printf(" >> XL Driver Opened.\n");

  // Read driver config
  xlStatus = xlGetDriverConfig(&g_xlDrvConfig);
  if (xlStatus) {
    printf("\nERROR: xlGetDriverConfig failed!\n");
    return xlStatus;
  }

  // Searching for a DAIO channel...
  unsigned int hwType    = 0;
  unsigned int hwIndex   = 0;  // Index of the hardware for the same hardware type
  unsigned int hwChannel = 0;  // Index of the channel on the same hardware.

  xlStatus = xlGetApplConfig(g_AppName,   // name of application's channel
                             0,           // zero based application channel
                             &hwType,     // received type of hardware
                             &hwIndex,    // received hardware index
                             &hwChannel,  // received hardware channel
                             XL_BUS_TYPE_DAIO);

  if (xlStatus == XL_SUCCESS) {
    printf(" >> HW found in registry\n");
  }
  else {
    // search a channel which supports DAIO
    auto channelIt = std::find_if(std::begin(g_xlDrvConfig.channel), std::end(g_xlDrvConfig.channel),
                                  [](const auto& channel) { return channel.channelBusCapabilities & XL_BUS_ACTIVE_CAP_DAIO; });

    if (channelIt == std::end(g_xlDrvConfig.channel)) {
      printf("Error: no channel assignment found!\n");
      return XL_ERROR;
    }

    hwType    = channelIt->hwType;
    hwChannel = channelIt->hwChannel;
    hwIndex   = channelIt->hwIndex;
    printf(" >> Found DAIO channel on hWType: %d;\n", hwType);

    xlStatus = xlSetApplConfig(  // Registration of Application with default settings
      g_AppName,                 // Application Name
      0,                         // Application channel 0
      hwType,                    // hwType  (e.g. VN1670...)
      hwIndex,                   // Index of hardware (slot) (0,1,...)
      hwChannel,                 // Index of channel (connector) (0,1,...)
      XL_BUS_TYPE_DAIO);         // the application is for DAIO.

    if (xlStatus) {
      printf("\nERROR: xlSetApplConfig failed!\n");
      return xlStatus;
    }
  }

  // assign transceiver
  unsigned int ci = xlGetChannelIndex(hwType, hwIndex, hwChannel);
  if (ci > XL_CONFIG_MAX_CHANNELS) {
    printf("Error: ?! // E.g. no Channel assigned in the Vector Hardware Manager or device has no electricity\n");
    return XL_ERROR;
  }
  g_trcvType = g_xlDrvConfig.channel[ci].transceiverType;
  printf(" >> DAIO transveiver type: 0x%x\n", g_trcvType);
  printf(" >> Check in the vxlapi.h what transceiver this corresponds to.\n");

  // Get Channel Mask - Access to one phys. Channel
  g_xlChannelMask = xlGetChannelMask(hwType, hwIndex, hwChannel);
  if (g_xlChannelMask == 0) {
    printf("\nERROR: Vector Hardware Configuration not properly set. Check your settings!\n");
    char callSign;
    xlPopupHwConfig(&callSign, 0);  // open VHwConf
    return XL_ERROR;
  }

  g_xlPermissionMask = g_xlChannelMask;
  printf(" >> Channel Mask Set CM:0x%I64x\n", g_xlChannelMask);

  // Open Port
  if (xlOpenPort(&g_xlPortHandle, g_AppName, g_xlChannelMask, &g_xlPermissionMask, 1024, XL_INTERFACE_VERSION, XL_BUS_TYPE_DAIO)
      != XL_SUCCESS) {
    printf("\nERROR: xlOpenPort failed!\n");
    return XL_ERROR;
  }
  printf(" >> Port Opened. ");

  return xlStatus;
}

// create RxThread
XLstatus daioCreateRxThread() {
  if (g_xlPortHandle != XL_INVALID_PORTHANDLE) {
    // Send a event for each Msg!!!
    XLstatus xlStatus = xlSetNotification(g_xlPortHandle, &g_hMsgEvent, 1);
    if (xlStatus != XL_SUCCESS) {
      return xlStatus;
    }

    DWORD ThreadId = 0;
    g_Thread       = CreateThread(0, 0x1000, RxThread, (LPVOID)0, 0, &ThreadId);
    if (g_Thread == NULL) {
      printf("\nERROR: CreateThread failed\n");
      printf("%i\n", GetLastError());
      return XL_ERROR;
    }
  }

  return xlResetClock(g_xlPortHandle);
}

// set up daio Cab - Cabs are a legacy product and depricated
XLstatus daioSetupCab() {
  unsigned int digitalOutputMask = DIO0 | DIO1;
  unsigned int digitalInputMask  = DIO_ALL & (~digitalOutputMask);  // all DIOs, that are not output are input
  if (xlDAIOSetDigitalParameters(g_xlPortHandle, g_xlChannelMask, digitalInputMask, digitalOutputMask) != XL_SUCCESS) {
    printf("\nERROR: xlDAIOSetDigitalParameters failed\n");
    return 0;
  }

  unsigned int analogOutputMask = AIO0;
  unsigned int analogInputMask  = AIO_ALL & (~analogOutputMask);  // all AIOs, that are not output are input
  if (g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_8444_OPTO) {
    // IO-Cab
    if (xlDAIOSetAnalogParameters(g_xlPortHandle, g_xlChannelMask, analogInputMask, analogOutputMask, 0x00) != XL_SUCCESS) {
      printf("\nERROR: xlDAIOSetAnalogParameters failed\n");
      return 0;
    }
  }

  // Activate Channel
  XLstatus xlStatus = xlActivateChannel(g_xlPortHandle, g_xlChannelMask, XL_BUS_TYPE_DAIO, 0);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlActivateChannel failed!\n");
    return xlStatus;
  }
  printf(" >> Channel Activated.\n");

  xlStatus = xlDAIOSetAnalogOutput(g_xlPortHandle, g_xlChannelMask, g_outputMilliVolt, 0, 0, 0);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlDAIOSetAnalogOutput failed\n");
    return xlStatus;
  }

  // Measurement configuration
  if (xlDAIOSetMeasurementFrequency(g_xlPortHandle, g_xlChannelMask, g_frequency) != XL_SUCCESS) {
    printf("\nERROR: xlDAIOSetMeasurementFrequency failed\n");
    return 0;
  }

  // The unit of the sample value is 1 mV.
  g_sampleScaling = 1.0;

  return xlStatus;
}

// set up built-in/fixed transceivers
XLstatus daioSetupBuiltIn(void) {
  // Set Trigger mode
  XLdaioTriggerMode trigMode;
  memset(&trigMode, 0x00, sizeof(trigMode));
  trigMode.portTypeMask = XL_DAIO_PORT_TYPE_MASK_ANALOG;
  if (g_ioPiggyDigitalTriggerCyclic) {
    trigMode.portTypeMask |= XL_DAIO_PORT_TYPE_MASK_DIGITAL;
  }
  trigMode.triggerType     = XL_DAIO_TRIGGER_TYPE_CYCLIC;
  trigMode.param.cycleTime = g_frequency * 1000;

  XLstatus xlStatus = xlIoSetTriggerMode(g_xlPortHandle, g_xlChannelMask, &trigMode);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoSetTriggerMode failed\n");
    return 0;
  }

  // Activate Channel
  xlStatus = xlActivateChannel(g_xlPortHandle, g_xlChannelMask, XL_BUS_TYPE_DAIO, 0);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlActivateChannel failed!\n");
    return xlStatus;
  }
  printf(" >> Channel Activated.\n");

  // VN16xx devices requires scaling of sample values.
  // One sample corresponds to 17.58 mV.
  g_sampleScaling = 17.58;

  return xlStatus;
}  // daioSetupBuiltIn

// set up piggy - this function really deals only with the piggies, that are added on to a device
XLstatus daioSetupPiggy(void) {
  //Configure the digital ports
  XLdaioSetPort portCfg;
  memset(&portCfg, 0x00, sizeof(portCfg));  //necessary to set reserved bits to zero
  portCfg.portType = XL_DAIO_PORT_TYPE_MASK_DIGITAL;
  portCfg.portMask = DIO_ALL;
  portCfg.portFunction[0] = XL_DAIO_PORT_DIGITAL_PUSHPULL;
  portCfg.portFunction[1] = XL_DAIO_PORT_DIGITAL_PUSHPULL;
  portCfg.portFunction[2] = XL_DAIO_PORT_DIGITAL_IN;
  portCfg.portFunction[3] = XL_DAIO_PORT_DIGITAL_IN;
  portCfg.portFunction[4] = XL_DAIO_PORT_DIGITAL_IN;
  portCfg.portFunction[5] = XL_DAIO_PORT_DIGITAL_IN;
  portCfg.portFunction[6] = XL_DAIO_PORT_DIGITAL_IN;
  portCfg.portFunction[7] = XL_DAIO_PORT_DIGITAL_IN;

  XLstatus xlStatus = xlIoConfigurePorts(g_xlPortHandle, g_xlChannelMask, &portCfg);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoConfigurePorts(digital) failed\n");
    return xlStatus;
  }

  xlStatus = xlIoSetDigOutLevel(g_xlPortHandle, g_xlChannelMask, XL_DAIO_DO_LEVEL_5V);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoSetDigOutLevel(digIn) failed\n");
    return xlStatus;
  }

  xlStatus = xlIoSetDigInThreshold(g_xlPortHandle, g_xlChannelMask, 1000 /*mV*/);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoSetDigInThreshold failed\n");
    return xlStatus;
  }

  //configure the analog ports
  memset(&portCfg, 0x00, sizeof(portCfg));
  portCfg.portType = XL_DAIO_PORT_TYPE_MASK_ANALOG;
  portCfg.portMask = AIO_ALL;
  portCfg.portFunction[0] = XL_DAIO_PORT_ANALOG_OUT;
  portCfg.portFunction[1] = XL_DAIO_PORT_ANALOG_IN;
  portCfg.portFunction[2] = XL_DAIO_PORT_ANALOG_IN;
  portCfg.portFunction[3] = XL_DAIO_PORT_ANALOG_IN;

  xlStatus = xlIoConfigurePorts(g_xlPortHandle, g_xlChannelMask, &portCfg);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoConfigurePorts(analog) failed\n");
    return xlStatus;
  }

  // Set Trigger mode
  XLdaioTriggerMode trigMode;
  memset(&trigMode, 0x00, sizeof(trigMode));
  trigMode.portTypeMask = XL_DAIO_PORT_TYPE_MASK_ANALOG;
  if (g_ioPiggyDigitalTriggerCyclic) {
    trigMode.portTypeMask |= XL_DAIO_PORT_TYPE_MASK_DIGITAL;
  }
  trigMode.triggerType     = XL_DAIO_TRIGGER_TYPE_CYCLIC;
  trigMode.param.cycleTime = g_frequency * 1000;

  xlStatus = xlIoSetTriggerMode(g_xlPortHandle, g_xlChannelMask, &trigMode);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoSetTriggerMode(cyclic) failed\n");
    return 0;
  }

  if (g_ioPiggyDigitalTriggerCyclic == 0) {
    memset(&trigMode, 0x00, sizeof(trigMode));
    trigMode.portTypeMask           = XL_DAIO_PORT_TYPE_MASK_DIGITAL;
    trigMode.triggerType            = XL_DAIO_TRIGGER_TYPE_PORT;
    trigMode.param.digital.portMask = DIO2 | DIO3 | DIO4 | DIO5 | DIO6 | DIO7;
    trigMode.param.digital.type = XL_DAIO_TRIGGER_TYPE_BOTH;

    xlStatus = xlIoSetTriggerMode(g_xlPortHandle, g_xlChannelMask, &trigMode);
    if (xlStatus != XL_SUCCESS) {
      printf("\nERROR: xlIoSetTriggerMode(on-edge) failed\n");
      return 0;
    }
  }

  // Activate Channel
  xlStatus = xlActivateChannel(g_xlPortHandle, g_xlChannelMask, XL_BUS_TYPE_DAIO, 0);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlActivateChannel failed!\n");
    return xlStatus;
  }
  printf(" >> Channel Activated.\n");

  // Start measurements
  xlStatus = xlIoStartSampling(g_xlPortHandle, g_xlChannelMask, XL_DAIO_PORT_TYPE_MASK_DIGITAL | XL_DAIO_PORT_TYPE_MASK_ANALOG);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoStartSampling failed!\n");
    return xlStatus;
  }
  printf(" >> Measurements started.\n");

  // Dealing with analog IO
  XLdaioAnalogParams anaPar;
  memset(&anaPar, 0x00, sizeof(anaPar));
  anaPar.portMask = AIO0;
  anaPar.value[0] = g_outputMilliVolt;  //mV
  xlStatus        = xlIoSetAnalogOutput(g_xlPortHandle, g_xlChannelMask, &anaPar);
  if (xlStatus != XL_SUCCESS) {
    printf("\nERROR: xlIoSetAnalogOutput failed\n");
    return xlStatus;
  }

  // The unit of the sample value is 1 mV.
  g_sampleScaling = 1.0;

  return xlStatus;
}


// toggle output
void ToggleSwitch() {
  //closes/opens all relays at the same time
  g_switchState = ~g_switchState;

  // IO-Cab
  if (g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_8444_OPTO) {
    if (xlDAIOSetDigitalOutput(g_xlPortHandle, g_xlChannelMask, SWITCH_DIO0_DIO01, g_switchState) != XL_SUCCESS) {
      printf("\nERROR: xlDAIOSetDigitalOutput failed\n");
    }
    return;
  }

  // Other transceivers
  XLdaioDigitalParams digPar;
  memset(&digPar, 0x00, sizeof(digPar));
  digPar.portMask  = DIO0;
  digPar.valueMask = g_switchState;
  if (xlIoSetDigitalOutput(g_xlPortHandle, g_xlChannelMask, &digPar) != XL_SUCCESS) {
    printf("\nERROR: xlIoSetDigitalOutput(digIn) failed\n");
  }
}

// Close XLAPI
void CloseExample() {
  // Close XL driver
  xlDeactivateChannel(g_xlPortHandle, g_xlChannelMask);
  xlClosePort(g_xlPortHandle);
  xlCloseDriver();
  printf("\n >> XL Driver Closed\n");

  // Close RX thread
  g_ThreadRun = 0;
  if (CloseHandle(g_Thread)) {
    printf(" >> RX Thread Closed.\n");
  }
  else {
    printf(" >> RX Thread NOT Closed!\n");
  }
}

// main [until end of file]
int main() {
  printf(drawLine);
  printf("                          xlDAIOexample                            \n");
  printf("                   Vector Informatik GmbH, " __DATE__ "             \n");
  printf(drawLine);

  // open driver, searching DAIO hardware
  XLstatus xlStatus = daioInit();
  if (xlStatus) {
    // xlStatus is 0/false, in case of successful init
    _getch();
    return xlStatus;
  }

  // create a RX thread to receive messages
  daioCreateRxThread();

  // check the kind of measurement
  printf("\n\n: Press <c>   for cyclic trigger");
  printf("  \n: Press <e>   for on-edge digital trigger\n");

  if ('e' == _getch()) {
    printf(" >> Using on-edge trigger for digital inputs.\n");
    g_ioPiggyDigitalTriggerCyclic = 0;
  }
  else {
    printf(" >> Using cyclic trigger for digital inputs.\n");
    g_ioPiggyDigitalTriggerCyclic = 1;
  }

  // setup the DAIO channel depending on the DAIO hardware
  // Legacy Cabs [-> depricated hardware]
  if (g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_8444_OPTO) {
    xlStatus = daioSetupCab();
  }
  // Piggys
  //--> VN7570, VN7572, VN8970, VN8972 [->8642]
  //--> VN1670 [->8644]
  else if (g_trcvType == XL_TRANSCEIVER_TYPE_PB_DAIO_8642 || g_trcvType == XL_TRANSCEIVER_TYPE_PB_DAIO_8644) {
    xlStatus = daioSetupPiggy();
  }
  // Built-In Transceivers
  //--> VH6501, VN0601, VN1630(A, LOG), VN1640(A), VN1641 [->1021_FIX;         /.::::\]
  //--> VN4610                                            [->1021_FIX_WITH_5V; /.::::\]
  //--> VN5640, VN5650, VN7640                            [->1021_FIX_WITH_AL; /.::::\]
  else if (g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_1021_FIX || g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_1021_FIX_WITH_AL
           || g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_1021_FIX_WITH_5V) {
    xlStatus = daioSetupBuiltIn();
  }
  //--> VN8810, VX0312                                    [->AL_ONLY;           (',') ]
  //--> VN5610A, VN5620, VN5620A                          [->AL_WU;              (;)  ]
  else if (g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_AL_ONLY || g_trcvType == XL_TRANSCEIVER_TYPE_DAIO_AL_WU) {
    printf("This example is unsuited for AL_ONLY and AL_WU transceiver types, as they don't support triggers");
  }
  else {
    printf("ERROR! no DAIO trx support!\n");
    return 0;
  }

  if (xlStatus) { // in case setup was tried and failed
    return xlStatus;
  }

  printf("\n: Press <t>   for toggle analog output");
  printf("\n: Press <ESC> for exit\n\n");

  // ------------------------------------
  // interactive key - commands
  // ------------------------------------
  int stop = 0;
  while (stop == 0) {
    unsigned long n;
    INPUT_RECORD  ir;
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &n);

    if ((n == 1) && (ir.EventType == KEY_EVENT) && ir.Event.KeyEvent.bKeyDown) {
      char c = ir.Event.KeyEvent.uChar.AsciiChar;
      switch (c) {
        case 't':
          ToggleSwitch();
          break;

        case 27:  // 'esc': end application
          stop = 1;
          break;

        default:
          break;
      }
    }
  }
  CloseExample();
}