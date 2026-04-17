/*------------------------------------------------------------------------------
| File:
|   xlCANdemo.C
| Project:
|   Sample for XL - Driver Library
|   Example application using 'vxlapi.dll'
|-------------------------------------------------------------------------------
|-------------------------------------------------------------------------------
| Copyright (c) 2014 by Vector Informatik GmbH.  All rights reserved.
 -----------------------------------------------------------------------------*/

#if defined(_Windows) || defined(_MSC_VER) || defined (__GNUC__)
 #define  STRICT
 #include <windows.h>
#endif

#include <stdio.h>

#define UNUSED_PARAM(a) { a=a; }

#define RECEIVE_EVENT_SIZE         1        // DO NOT EDIT! Currently 1 is supported only
#define RX_QUEUE_SIZE              4096     // internal driver queue size in CAN events
#define RX_QUEUE_SIZE_FD           16384    // driver queue size for CAN-FD Rx events
#define ENABLE_CAN_FD_MODE_NO_ISO  0        // Set to 1 to activate NO-ISO mode on capable CAN FD channels

#include "vxlapi.h"

// Track additional information about a channel
typedef struct {
  XLaccess channelIndex;
  unsigned int initPermission;
  unsigned int activated;
} t_additionalChannelInfo;

// Application context
typedef struct {
  unsigned int channelCount;
  t_additionalChannelInfo* channelInfo;
} t_appContext;

/////////////////////////////////////////////////////////////////////////////
// globals

char            g_AppName[XL_MAX_APPNAME+1]  = "xlCANdemo";               //!< Application name which is displayed in VHWconf
XLportHandle    g_xlPortHandle              = XL_INVALID_PORTHANDLE;      //!< Global porthandle (we use only one!)
XLapiIDriverConfigV1 g_xlDrvConfig;                                       //!< Contains the driver configuration
XLdeviceDrvConfigListV1 g_xlDevConfig;                                    //!< Contains the actual hardware configuration
XLchannelDrvConfigListV1 g_xlChannelConfig;                               //!< Contains the list of actual channels
XLaccess        g_xlChannelIndex            = XL_INVALID_CHANNEL_INDEX;   //!< Global channel index (of the currently selected channel)
unsigned int    g_BaudRate                  = 500000;                     //!< Default baudrate
int             g_silent                    = 0;                          //!< flag to visualize the message events (on/off)
unsigned int    g_TimerRate                 = 0;                          //!< Global timer rate (to toggle)
unsigned int    g_canFdSupport              = 0;                          //!< Global CAN FD support flag
unsigned int    g_canFdModeNoIso            = ENABLE_CAN_FD_MODE_NO_ISO;  //!< Global CAN FD ISO (default) / no ISO mode flag
t_appContext    g_appContext;                                             //!< Contains state managed by the application
unsigned int    g_appChannelIndex = 0;                                    //!< Application maintains own list of valid CAN channels. This index points to the currently active channel

// thread variables
XLhandle        g_hMsgEvent;                                          //!< notification handle for the receive queue
HANDLE          g_hRXThread;                                          //!< thread handle (RX)
HANDLE          g_hTXThread;                                          //!< thread handle (TX)
int             g_RXThreadRun;                                        //!< flag to start/stop the RX thread
int             g_TXThreadRun;                                        //!< flag to start/stop the TX thread (for the transmission burst)
int             g_RXCANThreadRun;                                     //!< flag to start/stop the RX thread
unsigned int    g_TXThreadCanId ;                                     //!< CAN-ID the TX thread transmits under
XLaccess        g_TXThreadTxIndex;                                    //!< channel index the TX thread uses for transmitting


////////////////////////////////////////////////////////////////////////////
// functions (Threads)

DWORD WINAPI RxCanFdThread( PVOID par );
DWORD WINAPI RxThread( PVOID par );
DWORD WINAPI TxThread( LPVOID par );

////////////////////////////////////////////////////////////////////////////
// functions (prototypes)
void     demoHelp(void);
void     demoPrintConfig(void);
XLstatus demoCreateRxThread(void);

#ifdef __GNUC__
static void strncpy_s(char *strDest, size_t numberOfElements, const char *strSource, size_t count)
{
  UNUSED_PARAM(numberOfElements);
  strncpy(strDest, strSource, count);	
}

static void sscanf_s(const char *buffer, const char *format, ...)
{
  va_list argList;
  va_start(argList, format);
  sscanf(buffer, format, argList);
}
#endif

////////////////////////////////////////////////////////////////////////////

//! demoHelp()

//! shows the program functionality
//!
////////////////////////////////////////////////////////////////////////////

void demoHelp(void) {

  printf("\n----------------------------------------------------------\n");
  printf("-                   xlCANdemo - HELP                     -\n");
  printf("----------------------------------------------------------\n");
  printf("- Keyboard commands:                                     -\n");
  printf("- 't'      Transmit a message                            -\n");
  printf("- 'b'      Transmit a message burst (toggle)             -\n");
  printf("- 'm'      Transmit a remote message                     -\n");
  printf("- 'g'      Request chipstate                             -\n");
  printf("- 's'      Start/Stop                                    -\n");
  printf("- 'r'      Reset clock                                   -\n");
  printf("- 'R'      Reset clock (new)                             -\n");
  printf("- '+'      Select channel      (up)                      -\n");
  printf("- '-'      Select channel      (down)                    -\n");
  printf("- 'i'      Select transmit Id  (up)                      -\n");
  printf("- 'I'      Select transmit Id  (down)                    -\n");
  printf("- 'x'      Toggle extended/standard Id                   -\n");
  printf("- 'o'      Toggle output mode                            -\n");
  printf("- 'a'      Toggle timer                                  -\n");
  printf("- 'v'      Toggle logging to screen                      -\n");
  printf("- 'p'      Show hardware configuration                   -\n");
  printf("- 'y'      Trigger HW-Sync pulse                         -\n");
  printf("- 'h'      Help                                          -\n");
  printf("- 'ESC'    Exit                                          -\n");
  printf("----------------------------------------------------------\n\n");

}

////////////////////////////////////////////////////////////////////////////

//! demoPrintConfig()

//! shows the actual hardware configuration
//!
////////////////////////////////////////////////////////////////////////////

void demoPrintConfig(void) {
  char         str[100];

  printf("----------------------------------------------------------\n");
  printf("-                Hardware Configuration                  -\n");
  printf("- %2d devices with %3d channels                           -\n", g_xlDevConfig.count, g_xlChannelConfig.count);
  printf("----------------------------------------------------------\n");

  for (unsigned int devIdx = 0; devIdx < g_xlDevConfig.count; devIdx++) {
    const XLdeviceDrvConfigV1* device = &g_xlDevConfig.item[devIdx];

    printf("%s (%d channels):\n", device->name, device->channelList.count);

    for (unsigned int chIdx = 0; chIdx < device->channelList.count; chIdx++) {
      printf("- Ch:%02d", device->channelList.item[chIdx].channelIndex);   

      if (device->channelList.item[chIdx].transceiver.type != XL_TRANSCEIVER_TYPE_NONE) {
        strncpy_s(str, 100, device->channelList.item[chIdx].transceiver.name, 48);
        printf(" %-48s -\n", str);
      }
      else {
        printf("    no Cab!   -\n");
      }
    }
    printf("\n");
  }
  
  printf("----------------------------------------------------------\n\n");
 
}

////////////////////////////////////////////////////////////////////////////

//! demoTransmit

//! transmit a CAN message (depending on an ID, channel)
//!
////////////////////////////////////////////////////////////////////////////

XLstatus demoTransmit(unsigned int txID, XLaccess xlChanIndex)
{
  XLstatus             xlStatus;
  unsigned int         messageCount = 1;
  static int           cnt = 0;

  if(g_canFdSupport) {
    unsigned int  fl[3] = {
      
      0 , // CAN (no FD)
      XL_CAN_TXMSG_FLAG_EDL,
      XL_CAN_TXMSG_FLAG_EDL | XL_CAN_TXMSG_FLAG_BRS,
    };

    XLcanTxEvent canTxEvt;
    unsigned int cntSent;
    unsigned int i;

    memset(&canTxEvt, 0, sizeof(canTxEvt));
    canTxEvt.tag = XL_CAN_EV_TAG_TX_MSG;

    canTxEvt.tagData.canMsg.canId     = txID;
    canTxEvt.tagData.canMsg.msgFlags  = fl[cnt%(sizeof(fl)/sizeof(fl[0]))];
    canTxEvt.tagData.canMsg.dlc       = 8;

    // if EDL is set, demonstrate transmit with DLC=15 (64 bytes)
    if (canTxEvt.tagData.canMsg.msgFlags & XL_CAN_TXMSG_FLAG_EDL) {
      canTxEvt.tagData.canMsg.dlc = 15;
    }

    ++cnt;
    
    for(i=1; i<XL_CAN_MAX_DATA_LEN; ++i) {
      canTxEvt.tagData.canMsg.data[i] = (unsigned char)i-1;
    }
    canTxEvt.tagData.canMsg.data[0] = (unsigned char)cnt;
    xlStatus = xlCanTransmitEx(g_xlPortHandle, xlChanIndex, messageCount, &cntSent, &canTxEvt);
  }
  else {
    static XLevent       xlEvent;
   
    memset(&xlEvent, 0, sizeof(xlEvent));

    xlEvent.tag                 = XL_TRANSMIT_MSG;
    xlEvent.tagData.msg.id      = txID;
    xlEvent.tagData.msg.dlc     = 8;
    xlEvent.tagData.msg.flags   = 0;
    ++xlEvent.tagData.msg.data[0];
    xlEvent.tagData.msg.data[1] = 2;
    xlEvent.tagData.msg.data[2] = 3;
    xlEvent.tagData.msg.data[3] = 4;
    xlEvent.tagData.msg.data[4] = 5;
    xlEvent.tagData.msg.data[5] = 6;
    xlEvent.tagData.msg.data[6] = 7;
    xlEvent.tagData.msg.data[7] = 8;

    xlStatus = xlCanTransmit(g_xlPortHandle, xlChanIndex, &messageCount, &xlEvent);

  }

  printf("- Transmit         : Channel 0x%I64x, %s\n", xlChanIndex, xlGetErrorString(xlStatus));

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! demoStopTransmitBurst

//! Stop the TX thread if it is running.
//!
////////////////////////////////////////////////////////////////////////////

void demoStopTransmitBurst()
{
  if (g_hTXThread) {
    g_TXThreadRun = 0;
    WaitForSingleObject(g_hTXThread, 10);
    g_hTXThread = 0;
  }
}

////////////////////////////////////////////////////////////////////////////

//! demoTransmitBurst

//! transmit a message burst (also depending on an IC, channel).
//!
////////////////////////////////////////////////////////////////////////////

void demoTransmitBurst(unsigned int txID, XLaccess xlChanIndex) 
{
  // first collect old TX-Thread
  demoStopTransmitBurst();

  printf("- print txID: %d\n", txID);
  g_TXThreadCanId = txID;
  g_TXThreadTxIndex = xlChanIndex;
  g_TXThreadRun = 1;
  g_hTXThread = CreateThread(0, 0x1000, TxThread, NULL, 0, NULL);
}

////////////////////////////////////////////////////////////////////////////

//! demoTransmitRemote

//! transmit a remote frame
//!
////////////////////////////////////////////////////////////////////////////

XLstatus demoTransmitRemote(unsigned int txID, XLaccess xlChanIndex)
{
  XLstatus      xlStatus;
  unsigned int  messageCount = 1;

  if(g_canFdSupport) {
    XLcanTxEvent canTxEvt;
    unsigned int cntSent;

    memset(&canTxEvt, 0, sizeof(canTxEvt));

    canTxEvt.tag = XL_CAN_EV_TAG_TX_MSG;

    canTxEvt.tagData.canMsg.canId     = txID;
    canTxEvt.tagData.canMsg.msgFlags  = XL_CAN_TXMSG_FLAG_RTR;
    canTxEvt.tagData.canMsg.dlc       = 8;

    xlStatus = xlCanTransmitEx(g_xlPortHandle, xlChanIndex, messageCount, &cntSent, &canTxEvt);
  }
  else {
    XLevent       xlEvent;

    memset(&xlEvent, 0, sizeof(xlEvent));

    xlEvent.tag               = XL_TRANSMIT_MSG;
    xlEvent.tagData.msg.id    = txID;
    xlEvent.tagData.msg.flags = XL_CAN_MSG_FLAG_REMOTE_FRAME;
    xlEvent.tagData.msg.dlc   = 8;

    xlStatus = xlCanTransmit(g_xlPortHandle, xlChanIndex, &messageCount, &xlEvent);
  }

  printf("- Transmit REMOTE  : Channel %I64u, %s\n", xlChanIndex, xlGetErrorString(xlStatus));
  
  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! demoStartStop

//! toggle the channel activate/deactivate
//!
////////////////////////////////////////////////////////////////////////////

XLstatus demoStartStop(unsigned int appChannel)
{
  XLstatus xlStatus;

  if (g_appContext.channelInfo[appChannel].activated == 0) {
    xlStatus = xlActivateChannel(g_xlPortHandle, g_appContext.channelInfo[appChannel].channelIndex, XL_BUS_TYPE_CAN, XL_ACTIVATE_RESET_CLOCK);
    printf("- ActivateChannel : Channel index %I64u, %s\n", g_appContext.channelInfo[appChannel].channelIndex, xlGetErrorString(xlStatus));
    if (xlStatus == XL_SUCCESS) {
      g_appContext.channelInfo[appChannel].activated = 1;
    }
  }
  else {
    demoStopTransmitBurst();
    xlStatus = xlDeactivateChannel(g_xlPortHandle, g_appContext.channelInfo[appChannel].channelIndex);
    printf("- DeativateChannel: Channel index %I64u, %s\n", g_appContext.channelInfo[appChannel].channelIndex, xlGetErrorString(xlStatus));
    if (xlStatus == XL_SUCCESS) {
      g_appContext.channelInfo[appChannel].activated = 0;
    }
  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! demoSetOutput

//! toggle NORMAL/SILENT mode of a CAN channel
//!
////////////////////////////////////////////////////////////////////////////

XLstatus demoSetOutput(int outputMode, const char *sMode, XLaccess xlChanIndex) {
  
  XLstatus xlStatus;

  // to get an effect we deactivate the channel first.
  xlStatus = xlDeactivateChannel(g_xlPortHandle, g_xlChannelIndex);

  xlStatus = xlCanSetChannelOutput(g_xlPortHandle, xlChanIndex, outputMode);
  printf("- SetChannelOutput: Channel 0x%I64u, %s, %s, %d\n", xlChanIndex, sMode, xlGetErrorString(xlStatus), outputMode);
 
  // and activate the channel again.
  xlStatus = xlActivateChannel(g_xlPortHandle, g_xlChannelIndex, XL_BUS_TYPE_CAN, XL_ACTIVATE_RESET_CLOCK);
  
  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! demoCreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus demoCreateRxThread(void) {
  XLstatus      xlStatus = XL_ERROR;
  DWORD         ThreadId=0;
 
  if (g_xlPortHandle!= XL_INVALID_PORTHANDLE) {

      // Send a event for each Msg!!!
      xlStatus = xlSetNotification (g_xlPortHandle, &g_hMsgEvent, 1);

      if (g_canFdSupport) {
        g_hRXThread = CreateThread(0, 0x1000, RxCanFdThread, (LPVOID) 0, 0, &ThreadId);
      }
      else { 
        g_hRXThread = CreateThread(0, 0x1000, RxThread, (LPVOID) 0, 0, &ThreadId);
      }

  }
  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! demoInitDriver

//! initializes the driver with one port and all founded channels which
//! have a connected CAN cab/piggy.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus demoInitDriver() {

  XLstatus xlStatus;
  
  // ------------------------------------
  // open the driver
  // ------------------------------------
  xlStatus = xlOpenDriver();
  
  // ------------------------------------
  // get/print the hardware configuration
  // ------------------------------------
  if(XL_SUCCESS == xlStatus) {
    // Use xlCreateDriverConfig instead of xlGetDriverConfig
    xlStatus = xlCreateDriverConfig(XL_IDRIVER_CONFIG_VERSION_1, (struct XLIDriverConfig*)&g_xlDrvConfig);

    // Get the devices
    xlStatus = g_xlDrvConfig.fctGetDeviceConfig(g_xlDrvConfig.configHandle, &g_xlDevConfig);
    if (xlStatus != XL_SUCCESS) {
      printf("ERROR: Could not get device configuration. %s\n", xlGetErrorString(xlStatus));
      return xlStatus;
    }

    // Get the channels
    xlStatus = g_xlDrvConfig.fctGetChannelConfig(g_xlDrvConfig.configHandle, &g_xlChannelConfig);
    if (xlStatus != XL_SUCCESS) {
      printf("ERROR: Could not get channel configuration. %s\n", xlGetErrorString(xlStatus));
      return xlStatus;
    }
  }
  
  if (XL_SUCCESS == xlStatus) {
    demoPrintConfig();

    printf("Usage: xlCANdemo <BaudRate> <ApplicationName> <Identifier>\n\n");

    // ------------------------------------
    // Check if there are any CAN FD channels.
    // Based on this information V3 or V4 API is used.
    // ------------------------------------
    for (unsigned int device_iter = 0; device_iter < g_xlDevConfig.count; device_iter++) {
      const XLdeviceDrvConfigV1* device = &g_xlDevConfig.item[device_iter];

      for (unsigned int channel_iter = 0; channel_iter < device->channelList.count; channel_iter++) {
        const XLchannelDrvConfigV1* channel = &(device->channelList.item[channel_iter]);

        // check if we can use CAN FD - the virtual CAN driver supports CAN-FD, but we don't use it.
        if ((channel->channelCapabilities & XL_CHANNEL_FLAG_EX1_CANFD_ISO_SUPPORT)
            && (device->hwType != XL_HWTYPE_VIRTUAL)) {
          g_canFdSupport = 1;
        }
      }
    }

    // Allocate memory for additional channel access information required later.
    g_appContext.channelInfo = malloc(g_xlChannelConfig.count * sizeof(t_additionalChannelInfo));
    if (!g_appContext.channelInfo) {
      printf("ERROR: Could not allocate enough memory. Requested %u bytes.\n", (unsigned int)(g_xlChannelConfig.count * sizeof(t_additionalChannelInfo)) );
      return XL_ERROR;
    }
    // Initialize structure with zeros.
    memset(g_appContext.channelInfo, 0x00, g_xlChannelConfig.count * sizeof(t_additionalChannelInfo));


    // ------------------------------------
    // Create a port and add the desired channels later.
    // ------------------------------------
    if (g_canFdSupport > 0) {
      xlStatus = xlCreatePort(&g_xlPortHandle, g_AppName, RX_QUEUE_SIZE_FD, XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_CAN);
    }
    else {
      xlStatus = xlCreatePort(&g_xlPortHandle, g_AppName, RX_QUEUE_SIZE, XL_INTERFACE_VERSION_V3, XL_BUS_TYPE_CAN);
    }
    if (xlStatus != XL_SUCCESS) {
      printf("ERROR: Could not create port. %s\n", xlGetErrorString(xlStatus));
      return xlStatus;
    }


    // ------------------------------------
    // Add the desired channels.
    // ------------------------------------
    g_appContext.channelCount = 0;
    for (unsigned int device_iter = 0; device_iter < g_xlDevConfig.count; device_iter++) {
      const XLdeviceDrvConfigV1* device = &g_xlDevConfig.item[device_iter];
      for (unsigned int channel_iter = 0; channel_iter < device->channelList.count; channel_iter++) {
        const XLchannelDrvConfigV1* channel = &(device->channelList.item[channel_iter]);

        // we use all hardware we have found and supports CAN
        if (channel->channelBusActiveCapabilities & XL_BUS_TYPE_CAN) {
          // Skip if we want to use CAN-FD but current channel is not CAN-FD capable 
          if (g_canFdSupport && !(channel->channelCapabilities & XL_CHANNEL_FLAG_EX1_CANFD_ISO_SUPPORT)) {
            continue;
          }

          g_appContext.channelInfo[g_appContext.channelCount].channelIndex = channel->channelIndex;

          // Request init permissions
          unsigned int init_req = 1;

          xlStatus = xlAddChannelToPort(g_xlPortHandle, channel->channelIndex, init_req, &g_appContext.channelInfo[g_appContext.channelCount].initPermission, XL_BUS_TYPE_CAN);
          if (xlStatus == XL_SUCCESS) {
            printf("Successfully added channel %s (%u) to port %u\n", device->name, channel->channelIndex, g_xlPortHandle);
              
            if (g_xlChannelIndex == XL_INVALID_CHANNEL_INDEX) {
              g_xlChannelIndex = channel->channelIndex;
            }
          }
          else {
            printf("Error: Could not add channel to port. %s\n", xlGetErrorString(xlStatus));
            xlClosePort(g_xlPortHandle);
            return xlStatus;
          }

          g_appContext.channelCount++;
        }
      }
    }
  }

  if (g_xlChannelIndex == XL_INVALID_CHANNEL_INDEX) {
    printf("ERROR: no available channels found! (e.g. no CANcabs...)\n\n");
    xlStatus = XL_ERROR;
  }

  // ------------------------------------
  // Finalize port configuration.
  // ------------------------------------
  xlStatus = xlFinalizePort(g_xlPortHandle);
  if (xlStatus != XL_SUCCESS) {
    printf("ERROR: Could not finalize port configuration. %s\n", xlGetErrorString(xlStatus));
  }
  else {
    printf("Successfully finalized the port configuration. No more channels can be added now.\n");
  }

  if ( (XL_SUCCESS == xlStatus) && (XL_INVALID_PORTHANDLE != g_xlPortHandle) ) {
    // ------------------------------------
    // if we have permission we set the
    // bus parameters (baudrate)
    // ------------------------------------
    for (unsigned int channel_iter = 0; channel_iter < g_appContext.channelCount; channel_iter++) {
      const XLchannelDrvConfigV1* channel = &(g_xlChannelConfig.item[g_appContext.channelInfo[channel_iter].channelIndex]);
      if (channel->channelBusCapabilities & XL_BUS_COMPATIBLE_CAN) {
        // Check if we have init access for this CAN channel.
        printf("[%d/%d] Channel: %u, Name: %s\n", (channel_iter + 1), g_appContext.channelCount, channel->channelIndex, channel->transceiver.name);
        if (g_appContext.channelInfo[channel_iter].initPermission > 0) {
          if (g_canFdSupport) {
            XLcanFdConf fdParams;

            memset(&fdParams, 0, sizeof(fdParams));

            // arbitration bitrate
            fdParams.arbitrationBitRate = 1000000;
            fdParams.tseg1Abr = 6;
            fdParams.tseg2Abr = 3;
            fdParams.sjwAbr = 2;

            // data bitrate
            fdParams.dataBitRate = fdParams.arbitrationBitRate * 2;
            fdParams.tseg1Dbr = 6;
            fdParams.tseg2Dbr = 3;
            fdParams.sjwDbr = 2;

            if (g_canFdModeNoIso && (channel->channelCapabilities & XL_CHANNEL_FLAG_EX1_CANFD_BOSCH_SUPPORT)) {
              fdParams.options = CANFD_CONFOPT_NO_ISO;
            }

            xlStatus = xlCanFdSetConfiguration(g_xlPortHandle, channel->channelIndex, &fdParams);
            printf("- SetFdConfig.     : ABaudr.=%u, DBaudr.=%u, %s\n", fdParams.arbitrationBitRate, fdParams.dataBitRate, xlGetErrorString(xlStatus));

          }
          else {
            xlStatus = xlCanSetChannelBitrate(g_xlPortHandle, channel->channelIndex, g_BaudRate);
            printf("- SetChannelBitrate: baudr.=%u, %s\n", g_BaudRate, xlGetErrorString(xlStatus));
          }
        }
        else {
          printf("-                  : we have NO init access!\n");
        }
      }
    }
  }
  else {
    xlClosePort(g_xlPortHandle);
    g_xlPortHandle = XL_INVALID_PORTHANDLE;
    xlStatus = XL_ERROR;
  }
  
  return xlStatus;
}                    

////////////////////////////////////////////////////////////////////////////

//! demoCleanUp()

//! close the port and the driver
//!
////////////////////////////////////////////////////////////////////////////

static XLstatus demoCleanUp(void)
{
  XLstatus xlStatus;
    
  if (g_xlPortHandle != XL_INVALID_PORTHANDLE) {
    xlStatus = xlClosePort(g_xlPortHandle);
    printf("- ClosePort        : PH(0x%x), %s\n", g_xlPortHandle, xlGetErrorString(xlStatus));
  }

  xlStatus = xlDestroyDriverConfig(g_xlDrvConfig.configHandle);
  if (xlStatus != XL_SUCCESS) {
    printf("ERROR: Could not destroy driver config. %s", xlGetErrorString(xlStatus));
  }

  g_xlPortHandle = XL_INVALID_PORTHANDLE;
  xlCloseDriver();

  free(g_appContext.channelInfo);

  return XL_SUCCESS;    // No error handling
}

////////////////////////////////////////////////////////////////////////////

//! main

//! 
//!
////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  XLstatus      xlStatus;

  int           stop = 0;
  int           c;
  unsigned int  txID = 0x01;
  int           outputMode = XL_OUTPUT_MODE_NORMAL;
  XLuint64      timeOffset = 0;


  printf("----------------------------------------------------------\n");
  printf("- xlCANdemo - Test Application for XL Family Driver API  -\n");
  printf("-             Vector Informatik GmbH,  " __DATE__"       -\n");
#ifdef _WIN64
  printf("-             - 64bit Version -                          -\n");
#endif
  printf("----------------------------------------------------------\n");

  // ------------------------------------
  // commandline may specify application 
  // name and baudrate
  // ------------------------------------
  if (argc > 1) {
    g_BaudRate = atoi(argv[1]);
    if (g_BaudRate) {
      printf("Baudrate = %u\n", g_BaudRate);
      argc--;
      argv++;
    }
  }
  if (argc > 1) {
    strncpy_s(g_AppName, XL_MAX_APPNAME, argv[1], XL_MAX_APPNAME);
    g_AppName[XL_MAX_APPNAME] = 0;
    printf("AppName = %s\n", g_AppName);
    argc--;
    argv++;
  }
  if (argc > 1) {
    sscanf_s (argv[1], "%x", &txID ) ;
    if (txID) {
      printf("TX ID = %x\n", txID);
    }
  }

  // ------------------------------------
  // initialize the driver structures 
  // for the application
  // ------------------------------------
  xlStatus = demoInitDriver();
  printf("- Init             : %s\n",  xlGetErrorString(xlStatus));
  
  if(XL_SUCCESS == xlStatus) {
    // ------------------------------------
    // create the RX thread to read the
    // messages
    // ------------------------------------
    xlStatus = demoCreateRxThread();
    printf("- Create RX thread : %s\n",  xlGetErrorString(xlStatus));
  }

  if(XL_SUCCESS == xlStatus) {
    // ------------------------------------
    // Use all selected channels on our port.
    // ------------------------------------
    xlStatus = xlActivateChannel(g_xlPortHandle, XL_USE_ALL_CHANNELS, XL_BUS_TYPE_CAN, XL_ACTIVATE_RESET_CLOCK);
    printf("- ActivateChannel  : %s\n", xlGetErrorString(xlStatus));
    if (xlStatus == XL_SUCCESS) {
      for (unsigned int i = 0; i < g_appContext.channelCount; i++) {
        g_appContext.channelInfo[i].activated = 1;
      }
    }
  }

  printf("\n: Press <h> for help - actual channel index Ch=%I64u\n", g_xlChannelIndex);

  // ------------------------------------
  // parse the key - commands
  // ------------------------------------
  while (stop == 0) {

    unsigned long n;
    INPUT_RECORD ir;

    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &n);

    if ((n == 1) && (ir.EventType == KEY_EVENT)) {

      if (ir.Event.KeyEvent.bKeyDown) {

        c = ir.Event.KeyEvent.uChar.AsciiChar;
        switch (c) {

          case 'v':
            if (g_silent) { g_silent = 0; printf("- screen on\n"); }
            else {g_silent = 1; printf("- screen off\n"); }
            break;

          case 't': // transmit a message
            demoTransmit(txID, g_xlChannelIndex);
            break;

          case 'b':  // transmit message burst 
            if (g_TXThreadRun) {
              demoStopTransmitBurst();
            }
            else {
              demoTransmitBurst(txID, g_xlChannelIndex);
            }
            break; 

          case 'm': // transmit a remote message
            demoTransmitRemote(txID, g_xlChannelIndex);
            break;

          case '-': // channel selection
            if (g_appChannelIndex == 0) g_appChannelIndex = g_appContext.channelCount;
            g_appChannelIndex--;

            g_xlChannelIndex = g_appContext.channelInfo[g_appChannelIndex].channelIndex;
           
            printf("- TX Channel set to channel: %I64u, %s\n", 
                   g_xlChannelIndex, g_xlChannelConfig.item[g_xlChannelIndex].transceiver.name);
            break;
            
          case '+': // channel selection
            g_appChannelIndex++;
            if (g_appChannelIndex >= g_appContext.channelCount) g_appChannelIndex = 0;

            g_xlChannelIndex = g_appContext.channelInfo[g_appChannelIndex].channelIndex;

            printf("- TX Channel set to channel: %I64u, %s\n", 
                   g_xlChannelIndex, g_xlChannelConfig.item[g_xlChannelIndex].transceiver.name);
            break;

          case 'x':
            txID ^= XL_CAN_EXT_MSG_ID; // toggle ext/std
            printf("- Id set to 0x%08X\n", txID);
            break;

          case 'I': // id selection
            if (txID & XL_CAN_EXT_MSG_ID) txID = (txID-1) | XL_CAN_EXT_MSG_ID;
            else if (txID == 0) txID = 0x7FF;
            else txID--;
            printf("- Id set to 0x%08X\n", txID);
            break;

          case 'i':
            if (txID & XL_CAN_EXT_MSG_ID) txID = (txID+1) | XL_CAN_EXT_MSG_ID;
            else if (txID == 0x7FF) txID = 0;
            else txID++;
            printf("- Id set to 0x%08X\n", txID);
            break;

          case 'g':
            xlStatus = xlCanRequestChipState(g_xlPortHandle, g_xlChannelIndex);
            printf("- RequestChipState : Index %I64u, %s\n", g_xlChannelIndex, xlGetErrorString(xlStatus));
            break;

          case 'a':
            if (g_TimerRate) g_TimerRate = 0; 
            else g_TimerRate = 20000;
 
            xlStatus = xlSetTimerRate(g_xlPortHandle, g_TimerRate);
            printf("- SetTimerRate     : %d, %s\n", g_TimerRate, xlGetErrorString(xlStatus));
            break;

          case 'o':
            if (outputMode == XL_OUTPUT_MODE_NORMAL) {
              outputMode = XL_OUTPUT_MODE_SILENT;
              demoSetOutput(outputMode, "SILENT", g_xlChannelIndex);
            }
            else {
              outputMode = XL_OUTPUT_MODE_NORMAL;
              demoSetOutput(outputMode, "NORMAL", g_xlChannelIndex);
            }
            break;

          case 'r':
            xlStatus = xlResetClock(g_xlPortHandle);
            printf("- ResetClock       : %s\n", xlGetErrorString(xlStatus));
            break;

          case 'R':
            xlStatus = xlTsResetClocks(g_xlPortHandle, NULL, NULL, NULL, &timeOffset);
            printf("- TsResetClock     : %s\n", xlGetErrorString(xlStatus));
            break;

          case 's':
            demoStartStop(g_appChannelIndex);
            break;

          case 'p':
            demoPrintConfig();
            break;

          case 'y':
            xlStatus = xlGenerateSyncPulse(g_xlPortHandle, g_xlChannelIndex);
            printf("- xlGenerateSyncPulse : Index %I64u, %s\n", g_xlChannelIndex, xlGetErrorString(xlStatus));
            break;

          case 27: // end application
            stop=1;
            break;

          case 'h':
            demoHelp();
            break;

          default:
            break;
                                                            // end switch
        }
      }
    }
  }                                                         // end while
  

  if((XL_SUCCESS != xlStatus)) { 
    xlStatus = xlDeactivateChannel(g_xlPortHandle, XL_USE_ALL_CHANNELS);
    printf("- DeactivateChannel: Index %I64u, %s\n", g_xlChannelIndex, xlGetErrorString(xlStatus));
  } 
  demoCleanUp();

  return(0);
}                                                  // end main()


////////////////////////////////////////////////////////////////////////////

//! TxThread

//! 
//!
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI TxThread( LPVOID par ) 
{
  XLstatus      xlStatus = XL_SUCCESS;
  unsigned int  n = 1;
  XLcanTxEvent  canTxEvt;
  XLevent       xlEvent;
  unsigned int  cntSent;

  UNREFERENCED_PARAMETER(par);

  if(g_canFdSupport) {
  
    unsigned int i;

    memset(&canTxEvt, 0, sizeof(canTxEvt));
    canTxEvt.tag = XL_CAN_EV_TAG_TX_MSG;

    canTxEvt.tagData.canMsg.canId     = g_TXThreadCanId;
    canTxEvt.tagData.canMsg.msgFlags  = XL_CAN_TXMSG_FLAG_EDL | XL_CAN_TXMSG_FLAG_BRS;
    canTxEvt.tagData.canMsg.dlc       = 15;

    for(i=1; i<XL_CAN_MAX_DATA_LEN; ++i) {
      canTxEvt.tagData.canMsg.data[i] = (unsigned char)i-1;
    }
  }
  else {

    memset(&xlEvent, 0, sizeof(xlEvent));

    xlEvent.tag                 = XL_TRANSMIT_MSG;
    xlEvent.tagData.msg.id      = g_TXThreadCanId;
    xlEvent.tagData.msg.dlc     = 8;
    xlEvent.tagData.msg.flags   = 0;
    xlEvent.tagData.msg.data[0] = 1;
    xlEvent.tagData.msg.data[1] = 2;
    xlEvent.tagData.msg.data[2] = 3;
    xlEvent.tagData.msg.data[3] = 4;
    xlEvent.tagData.msg.data[4] = 5;
    xlEvent.tagData.msg.data[5] = 6;
    xlEvent.tagData.msg.data[6] = 7;
    xlEvent.tagData.msg.data[7] = 8;
  
  }

  while (g_TXThreadRun && XL_SUCCESS == xlStatus) {

    if(g_canFdSupport) {
      ++canTxEvt.tagData.canMsg.data[0];
      xlStatus = xlCanTransmitEx(g_xlPortHandle, g_TXThreadTxIndex, n, &cntSent, &canTxEvt);
    }
    else {
      ++xlEvent.tagData.msg.data[0];
      xlStatus = xlCanTransmit(g_xlPortHandle, g_TXThreadTxIndex, &n, &xlEvent);
    }
    
    Sleep(10);
 
  }

  if(XL_SUCCESS != xlStatus) {
    printf("Error xlCanTransmit:%s\n", xlGetErrorString(xlStatus));
  }

  g_TXThreadRun = 0;
  return NO_ERROR; 
}

///////////////////////////////////////////////////////////////////////////

//! RxThread

//! thread to readout the message queue and parse the incoming messages
//!
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI RxThread(LPVOID par) 
{
  XLstatus        xlStatus;
  
  unsigned int    msgsrx = RECEIVE_EVENT_SIZE;
  XLevent         xlEvent; 
  
  UNUSED_PARAM(par); 
  
  g_RXThreadRun = 1;

  while (g_RXThreadRun) { 
   
    WaitForSingleObject(g_hMsgEvent,10);

    xlStatus = XL_SUCCESS;
    
    while (!xlStatus) {
      
      msgsrx = RECEIVE_EVENT_SIZE;

      xlStatus = xlReceive(g_xlPortHandle, &msgsrx, &xlEvent);      
      if ( xlStatus!=XL_ERR_QUEUE_IS_EMPTY ) {

        if (!g_silent) {
          printf("%s\n", xlGetEventString(&xlEvent));
        }

      }  
    }
          
  }
  return NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////

//! RxCANThread

//! thread to read the message queue and parse the incoming messages
//!
////////////////////////////////////////////////////////////////////////////
DWORD WINAPI RxCanFdThread(LPVOID par)
{
  XLstatus        xlStatus = XL_SUCCESS;
  DWORD           rc;
  XLcanRxEvent    xlCanRxEvt;
  
  UNUSED_PARAM(par); 

  g_RXCANThreadRun = 1;
  
  while (g_RXCANThreadRun) {
    rc = WaitForSingleObject(g_hMsgEvent, 10);
    if(rc != WAIT_OBJECT_0) continue;
    
    do {
      xlStatus = xlCanReceive(g_xlPortHandle, &xlCanRxEvt);
      if(xlStatus==XL_ERR_QUEUE_IS_EMPTY ) {
        break;
      }
     if (!g_silent) {
      printf("%s\n", xlCanGetEventString(&xlCanRxEvt));
     }

    } while(XL_SUCCESS == xlStatus);
  }
  
  return(NO_ERROR); 
} // RxCanFdThread

