/*-------------------------------------------------------------------------------------------
| File        : xlEthBypassDemo.cpp
| Project     : Vector XL-API Command Line Example for Ethernet functions
|
| Description : Shows how to configure a Vector Ethernet device (VN56xx) for signal tapping
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2020 by Vector Informatik GmbH.  All rights reserved.
|--------------------------------------------------------------------------------------------*/



#include "windows.h"
#include "stdio.h"
#define DYNAMIC_XLDRIVER_DLL
#include "vxlapi.h"
#undef DYNAMIC_XLDRIVER_DLL


#ifdef _WIN64
  #define LOAD_DLL_NAME  "vxlapi64"
#else
  #define LOAD_DLL_NAME  "vxlapi"
#endif

#define SWAP_WORD(var)   ((((var)<<8)&0xff00) | (((var)>>8)&0x00ff))




/*********/
/* Types */
/*********/

//Struct of options configurable via command line
typedef struct {
  bool                 showHelp;
  unsigned int         hwIndex;             //Hardware index of device to use (0, 1, ...)
  unsigned int         chConfig[2];         //Channel configuration. Must be compatible with each other, otherwise the PHY bypass will not work.
  unsigned int         bypassMode;          //The bypass mode to use (see XL_ETH_BYPASS_* variables). 1 = PHY bapss, 2 = MAC bypass
} T_BYPASSDEMO_OPTIONS;

//Struct of function pointers into XL-API DLL.
typedef struct {
  XLOPENDRIVER         xlOpenDriver;
  XLCLOSEDRIVER        xlCloseDriver;
  XLGETERRORSTRING     xlGetErrorString;
  XLGETDRIVERCONFIG    xlGetDriverConfig;
  XLOPENPORT           xlOpenPort;
  XLCLOSEPORT          xlClosePort;
  XLSETNOTIFICATION    xlSetNotification;
  XLACTIVATECHANNEL    xlActivateChannel;
  XLDEACTIVATECHANNEL  xlDeactivateChannel;
  XLFP_ETHSETCONFIG    xlEthSetConfig;
  XLFP_ETHSETBYPASS    xlEthSetBypass;
  XLFP_ETHRECEIVE      xlEthReceive;
} T_XLAPI;


/*************/
/* Constants */
/*************/

//Channel configurations. 
static const T_XL_ETH_CONFIG ethConfigModes[] = {
/* speed                        duplex                   connector                   phy                           clockMode                    mdiMode               brPairs */ 
  {XL_ETH_MODE_SPEED_AUTO_100,  XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_RJ45, XL_ETH_MODE_PHY_IEEE_802_3,   XL_ETH_MODE_CLOCK_DONT_CARE, XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_DONT_CARE}, //100 MBit/s
  {XL_ETH_MODE_SPEED_AUTO_1000, XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_RJ45, XL_ETH_MODE_PHY_IEEE_802_3,   XL_ETH_MODE_CLOCK_AUTO,      XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_DONT_CARE}, //1000 MBit/s

  {XL_ETH_MODE_SPEED_AUTO_100,  XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_DSUB, XL_ETH_MODE_PHY_BROADR_REACH, XL_ETH_MODE_CLOCK_MASTER,    XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_1PAIR}, //BroadR-Reach, master mode
  {XL_ETH_MODE_SPEED_AUTO_100,  XL_ETH_MODE_DUPLEX_AUTO, XL_ETH_MODE_CONNECTOR_DSUB, XL_ETH_MODE_PHY_BROADR_REACH, XL_ETH_MODE_CLOCK_SLAVE,     XL_ETH_MODE_MDI_AUTO, XL_ETH_MODE_BR_PAIR_1PAIR}, //BroadR-Reach, slave mode
};




/*******************/
/* Local functions */
/*******************/
static const char *linkSpeed(const T_XL_ETH_CHANNEL_STATUS *status) {
  static char  result[80];
  const char  *phy = "Unknown";
  const char  *speed = "";
  const char  *duplex = "";
  const char  *mdi = "";
  const char  *connector = "";
  const char  *clock = "";
  const char  *brPair = "";

  switch(status->activePhy) {
  case XL_ETH_STATUS_PHY_IEEE_802_3:    phy = "IEEE ";  break;
  case XL_ETH_STATUS_PHY_BROADR_REACH:  phy = "BR ";    break;
  default:                              phy = NULL;       break;
  }

  switch(status->speed) {
  case XL_ETH_STATUS_SPEED_100:         speed = "100 MBit/s";  break;
  case XL_ETH_STATUS_SPEED_1000:        speed = "1000 MBit/s"; break;
  default:                              speed = "unknown speed"; break;
  }

  switch(status->duplex) {
  case XL_ETH_STATUS_DUPLEX_FULL:       duplex = " FD";    break;
  default:                              duplex = NULL;    break;
  }

  switch(status->mdiType) {
  case XL_ETH_STATUS_MDI_STRAIGHT:      mdi = "MDI";         break;
  case XL_ETH_STATUS_MDI_CROSSOVER:     mdi = "MDI-X";       break;
  default:                              mdi = "MDI unknown"; break;
  }

  switch(status->activeConnector) {
  case XL_ETH_STATUS_CONNECTOR_RJ45:    connector = "RJ45";    break;
  case XL_ETH_STATUS_CONNECTOR_DSUB:    connector = "DSub";    break;
  default:                              connector = "unknown"; break;
  }

  switch(status->clockMode) {
  case XL_ETH_STATUS_CLOCK_DONT_CARE:   clock = NULL;           break;
  case XL_ETH_STATUS_CLOCK_MASTER:      clock = "clock master"; break;
  case XL_ETH_STATUS_CLOCK_SLAVE:       clock = "clock slave";  break;
  default:                              clock = NULL;           break;
  }

  switch(status->brPairs) {
  case XL_ETH_STATUS_BR_PAIR_1PAIR:     brPair = "1-pair"; break;
  case XL_ETH_STATUS_BR_PAIR_DONT_CARE: brPair = NULL;     break;
  }

  _snprintf_s(result, sizeof(result), _TRUNCATE, "%s%s%s %s, on %s%s%s%s%s%s",
    phy?phy:"", speed, duplex?duplex:"", mdi, connector,
    clock || brPair?" (":"", clock?clock:"", clock && brPair?",":"", brPair?brPair:"", clock || brPair?")":"");

  return result;
}

static void showHelp(void) {
  printf("Signal tap example for Vector Ethernet devices\n\n");
  printf("Command line arguments:\n");
  printf("/h    - This help\n");
  printf("/dn   - Use device n (n = 0,1,2,...). Default: first device found\n");
  printf("/cX,Y - Use channel configurations X and Y (see below). Default: 0\n");
  printf("/bZ   - Use bypass mode Z (1 = PHY mode, 2 = MAC mode). Default: 1\n");
  printf("\n");
  printf("\n");
  printf("Config modes:\n");
  printf("0 - IEEE 100 MBit/s full duplex on RJ45 connector\n");
  printf("1 - IEEE 1000 MBit/s full duplex on RJ45 connector\n");
  printf("2 - BroadR-Reach 100 MBit/s full duplex on D-Sub connector (master clock)\n");
  printf("3 - BroadR-Reach 100 MBit/s full duplex on D-Sub connector (slave clock)\n");
  printf("\n");
}

static XLstatus loadDLL(HINSTANCE *vectorDll, T_XLAPI *xlapi) {
  XLstatus result = XL_SUCCESS;

  if(!vectorDll || !xlapi) {
    result = XL_ERROR;
  }
  else {
    *vectorDll = LoadLibraryA( LOAD_DLL_NAME );
    if(!*vectorDll) {
      printf("Cannot load library - error %u.\n", GetLastError());
      result = XL_ERR_CANNOT_OPEN_DRIVER;
    }
    else {
      unsigned int notFound = 0;

      memset(xlapi, 0, sizeof(*xlapi));
      if(NULL == (xlapi->xlGetErrorString    = (XLGETERRORSTRING)   GetProcAddress(*vectorDll,"xlGetErrorString")))    { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlOpenDriver        = (XLOPENDRIVER)       GetProcAddress(*vectorDll,"xlOpenDriver")))        { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlCloseDriver       = (XLCLOSEDRIVER)      GetProcAddress(*vectorDll,"xlCloseDriver")))       { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlGetDriverConfig   = (XLGETDRIVERCONFIG)  GetProcAddress(*vectorDll,"xlGetDriverConfig")))   { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlOpenPort          = (XLOPENPORT)         GetProcAddress(*vectorDll,"xlOpenPort")))          { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlClosePort         = (XLCLOSEPORT)        GetProcAddress(*vectorDll,"xlClosePort")))         { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlSetNotification   = (XLSETNOTIFICATION)  GetProcAddress(*vectorDll,"xlSetNotification")))   { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlActivateChannel   = (XLACTIVATECHANNEL)  GetProcAddress(*vectorDll,"xlActivateChannel")))   { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlDeactivateChannel = (XLDEACTIVATECHANNEL)GetProcAddress(*vectorDll,"xlDeactivateChannel"))) { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlEthSetConfig      = (XLFP_ETHSETCONFIG)  GetProcAddress(*vectorDll,"xlEthSetConfig")))      { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlEthSetBypass      = (XLFP_ETHSETBYPASS)  GetProcAddress(*vectorDll,"xlEthSetBypass")))      { result = XL_ERROR; notFound++; }
      if(NULL == (xlapi->xlEthReceive        = (XLFP_ETHRECEIVE)    GetProcAddress(*vectorDll,"xlEthReceive")))        { result = XL_ERROR; notFound++; }

      if(result != XL_SUCCESS) {
        printf("Error, %u functions not found in DLL.\n", notFound);
      }
    }
  }

  return result;
}

static XLstatus unloadDLL(HINSTANCE *vectorDll, T_XLAPI *xlapi) {
  XLstatus result = XL_SUCCESS;

  if(vectorDll) {
    if(!FreeLibrary(*vectorDll)) {
      result = XL_ERROR;
    }
    else {
      *vectorDll = NULL;
      memset(xlapi, 0, sizeof(*xlapi));
    }
  }
  return result;
}



static XLstatus readCmdLineOptions(T_BYPASSDEMO_OPTIONS *options, int argc, char* argv[]) {
  XLstatus result = XL_SUCCESS;
  
  for(int argIndex = 1; argIndex < argc && result == XL_SUCCESS; ++argIndex) {
    const char *arg = argv[argIndex];

    switch(arg[0]) {
    case '/':
    case '-':
      arg++;
      break;
    }

    switch(arg[0]) {
    case 'h':
      options->showHelp = true;
      break;
    case 'd':
      {
        unsigned int device = 0;

        switch(sscanf_s(arg+1, "%u", &device)) {
        case 1:
          options->hwIndex = device;
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'c':
      {
        unsigned int configs[2] = {0, 0};

        switch(sscanf_s(arg+1, "%u,%u", &configs[0], &configs[1])) {
        case 1:
          if(configs[0] < _countof(ethConfigModes)) {
            options->chConfig[0] = configs[0];
            options->chConfig[1] = configs[0];
          }
          else {
            result = XL_ERR_WRONG_PARAMETER;
          }
          break;
        case 2:
          if(configs[0] < _countof(ethConfigModes) && configs[1] < _countof(ethConfigModes)) {
            options->chConfig[0] = configs[0];
            options->chConfig[1] = configs[1];
          }
          else {
            result = XL_ERR_WRONG_PARAMETER;
          }
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    case 'b':
      {
        unsigned int bypassMode = 0;

        switch(sscanf_s(arg+1, "%u", &bypassMode)) {
        case 1:
          if(bypassMode > 0 && bypassMode <= 2) {
            options->bypassMode = bypassMode;
          }
          else {
            result = XL_ERR_WRONG_PARAMETER;
          }
          break;
        default:
          result = XL_ERR_WRONG_PARAMETER;
          break;
        }
      }
      break;
    default:
      result = XL_ERR_WRONG_PARAMETER;
      break;
    }
  }

  return result;
}

static XLstatus handleInputEvent(bool *abort) {
  XLstatus       result = XL_SUCCESS;
  DWORD          numberOfEventsRead = 0;
  INPUT_RECORD   input;

  if(!ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input, 1, &numberOfEventsRead)) {
    //Read failed
  }
  else if(numberOfEventsRead > 0) {
    //Check if ESC has been pressed
    switch(input.EventType) {
    case KEY_EVENT:
      switch(input.Event.KeyEvent.uChar.AsciiChar) {
      case VK_ESCAPE: //Escape key
        if(abort) {
          *abort = true;
        }
        break;
      }
    }
  }

  return result;
}

static XLstatus handleEthernetEvent(XLportHandle port, T_XLAPI *xlapi, bool *abort) {
  XLstatus result = XL_SUCCESS;
  
  do {
    T_XL_ETH_EVENT    ethEvent;

    result = xlapi->xlEthReceive(port, &ethEvent);
    switch(result) {
    case XL_SUCCESS:
      switch(ethEvent.tag) {
      case XL_ETH_EVENT_TAG_FRAMERX:
        {
          const unsigned char *srcMac = ethEvent.tagData.frameRxOk.sourceMAC;
          const unsigned char *destMac = ethEvent.tagData.frameRxOk.destMAC;
          unsigned short       ethType = ethEvent.tagData.frameRxOk.frameData.ethFrame.etherType;
          unsigned short       payloadLen = ethEvent.tagData.frameRxOk.dataLen;
          static unsigned int  rxCount = 0;

          printf("%5u. CH%u: %02X-%02X-%02X-%02X-%02X-%02X -> %02X-%02X-%02X-%02X-%02X-%02X: Type 0x%04x (%u bytes)\n",
            ++rxCount, ethEvent.channelIndex,
            srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5],
            destMac[0], destMac[1], destMac[2], destMac[3], destMac[4], destMac[5],
            SWAP_WORD(ethType), payloadLen - sizeof(ethType)); //
        }
        break;
      case XL_ETH_EVENT_TAG_FRAMERX_ERROR:
        {
          const unsigned char *srcMac = ethEvent.tagData.frameRxOk.sourceMAC;
          const unsigned char *destMac = ethEvent.tagData.frameRxOk.destMAC;
          unsigned short       ethType = ethEvent.tagData.frameRxOk.frameData.ethFrame.etherType;
          unsigned short       payloadLen = ethEvent.tagData.frameRxOk.dataLen;
          static unsigned int  rxCount = 0;

          printf("%5u. ERROR %x - CH%u: %02X-%02X-%02X-%02X-%02X-%02X -> %02X-%02X-%02X-%02X-%02X-%02X: Type 0x%04x (%u bytes)\n",
            ++rxCount, ethEvent.tagData.frameRxError.errorFlags, ethEvent.channelIndex,
            srcMac[0], srcMac[1], srcMac[2], srcMac[3], srcMac[4], srcMac[5],
            destMac[0], destMac[1], destMac[2], destMac[3], destMac[4], destMac[5],
            SWAP_WORD(ethType), payloadLen - sizeof(ethType)); //
        }
        break;
      case XL_ETH_EVENT_TAG_FRAMETX_ERROR:
      case XL_ETH_EVENT_TAG_FRAMETX_ACK:
        //These events will not happen - we do not send.
        break;
      case XL_ETH_EVENT_TAG_FRAMETX_ACK_SWITCH:
      case XL_ETH_EVENT_TAG_FRAMETX_ERROR_SWITCH:
        //These events are only received in MAC bypass mode and indicate that a packet has passed the switch (and was or was not successfully sent).
        break;
      case XL_ETH_EVENT_TAG_CHANNEL_STATUS:
        switch(ethEvent.tagData.channelStatus.link) {
        case XL_ETH_STATUS_LINK_DOWN:
          printf("Link status: channel %u is DOWN.\n", ethEvent.channelIndex);
          break;
        case XL_ETH_STATUS_LINK_UP:
          printf("Link status: channel %u is UP with %s.\n", ethEvent.channelIndex, linkSpeed(&ethEvent.tagData.channelStatus));
          break;
        default:
          break;
        }
        break;
      case XL_ETH_EVENT_TAG_CONFIGRESULT:
        switch(ethEvent.tagData.configResult.result) {
        case 0:
          printf("Configuration for channel %u successful! Wait for link to come up...\n", ethEvent.channelIndex);
          break;
        default:
          printf("Configuration for channel %u failed - reason: %u\n", ethEvent.channelIndex, ethEvent.tagData.configResult.result);
          *abort = true;
          break;
        }
        break;
      case XL_ETH_EVENT_TAG_ERROR:
        printf("Unknown error for channel %u - abort\n", ethEvent.channelIndex);
        *abort = true;
        break;
      }
      break;
    }
  } while(result == XL_SUCCESS);

  
  return result;
}

int main (int argc, char* argv[]) {
  XLstatus        result = XL_SUCCESS;
  T_BYPASSDEMO_OPTIONS options;
  HINSTANCE       vectorDll = NULL;
  T_XLAPI xlapi;
  XLdriverConfig  driverConfig;
  XLportHandle    port = XL_INVALID_PORTHANDLE;
  char           *username = "xlEthBypassDemo";
  XLhandle        notificationHandle = NULL;

  printf("---------------------------------------------------------------\n");
  printf("- xlEthBypassDemo - Test Application for XL Family Driver API -\n");
  printf("- (C) 2025 Vector Informatik GmbH                             -\n");
  printf("---------------------------------------------------------------\n");

  memset(&options, 0, sizeof(options));
  
  DWORD fdwMode;
  if (!GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &fdwMode)) {
    printf("GetConsoleMode() failed with cause %u (0x%x). Stop.\n", GetLastError(), GetLastError());
    result = XL_ERROR;
  }
  else if (!SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), fdwMode & ~ENABLE_MOUSE_INPUT)) {
    printf("SetConsoleMode() failed with cause %u (0x%x). Stop.\n", GetLastError(), GetLastError());
    result = XL_ERROR;
  }

  //Set default options
  options.showHelp = false;
  options.hwIndex = (unsigned)-1; //First device present
  options.chConfig[0] = 0;
  options.chConfig[1] = 0;
  options.bypassMode = XL_ETH_BYPASS_PHY;

  if(XL_SUCCESS != result) {
  }
  else if(XL_SUCCESS != (result = readCmdLineOptions(&options, argc, argv))) {
  }
  else if(options.showHelp) {
    showHelp();
  }
  else if(XL_SUCCESS != (result = loadDLL(&vectorDll, &xlapi))) {
    printf("Error, failed to open Vector XL-API DLL. Reason: %u\n", result);
  }
  else if(XL_SUCCESS != (result = xlapi.xlOpenDriver())) {
    printf("Error, could not open driver. Reason: %s.\n", xlapi.xlGetErrorString(result));
  }
  else if(XL_SUCCESS != (result = xlapi.xlGetDriverConfig(&driverConfig))) {
    printf("Error, could not read driver config. Reason: %s.\n", xlapi.xlGetErrorString(result));

    if(XL_SUCCESS != (result = xlapi.xlCloseDriver())) {
      printf("Error, failed to close driver! Reason: %u\n", result);
    }
  }
  else {
    XLaccess     accessMask = 0;
    XLaccess     permissionMask = 0;
    unsigned int channelsFound = 0;
    unsigned int channels[2];
    unsigned short userHandle = 123;  //Arbitrary value to

    //Determine the device to work on
    for(unsigned int ch = 0; ch < driverConfig.channelCount && channelsFound < 2; ++ch) {
      if(options.hwIndex == driverConfig.channel[ch].hwIndex || options.hwIndex == (unsigned)-1) {
        if(driverConfig.channel[ch].channelBusCapabilities & XL_BUS_ACTIVE_CAP_ETHERNET) {
          accessMask |= driverConfig.channel[ch].channelMask;
          channels[channelsFound] = ch;
          channelsFound++;
        }
      }
    }

    permissionMask = accessMask;
    
    if(channelsFound != 2) {
      printf("Error, device not found or device does not have two useable Ethernet channels!\n");
    }
    else if(XL_SUCCESS != (result = xlapi.xlOpenPort(&port, 
                                                      username, 
                                                      accessMask, 
                                                     &permissionMask, 
                                                      8*1024*1024,              //8 MB receive buffer
                                                      XL_INTERFACE_VERSION_V4,  //V4 is the required version for Ethernet
                                                      XL_BUS_TYPE_ETHERNET))) {
      printf("Error, failed to open Ethernet port! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    else if(accessMask != permissionMask) {
      printf("Error, could not get init permission for all required channels!\n");
    }
    else if(XL_SUCCESS != (result = xlapi.xlSetNotification(port, &notificationHandle, 1))) { //Notify for each single event in queue
      printf("Error, failed to set notification handle! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    //xlActivateChannel
    else if(XL_SUCCESS != (result = xlapi.xlActivateChannel(port, accessMask, XL_BUS_TYPE_ETHERNET, XL_ACTIVATE_NONE))) {
      printf("Error, failed to activate channel! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    //Disable bypass first (if present), because configuration may fail otherwise
    else if(XL_SUCCESS != (result = xlapi.xlEthSetBypass(port, permissionMask, userHandle, XL_ETH_BYPASS_INACTIVE))) {
      printf("Error, failed to activate bypass! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    //Set channel configuration
    else if(XL_SUCCESS != (result = xlapi.xlEthSetConfig(port, driverConfig.channel[channels[0]].channelMask, userHandle, &ethConfigModes[options.chConfig[0]]))) {
      printf("Error, failed to configure Ethernet channel %u! Reason: %s\n", channels[0], xlapi.xlGetErrorString(result));
    }
    else if(XL_SUCCESS != (result = xlapi.xlEthSetConfig(port, driverConfig.channel[channels[1]].channelMask, userHandle, &ethConfigModes[options.chConfig[1]]))) {
      printf("Error, failed to configure Ethernet channel %u! Reason: %s\n", channels[1], xlapi.xlGetErrorString(result));
    }
    //Enable bypass again
    else if(XL_SUCCESS != (result = xlapi.xlEthSetBypass(port, permissionMask, userHandle, options.bypassMode))) {
      printf("Error, failed to activate bypass! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    else {
      DWORD         numWaitHandles = 0;
      HANDLE        waitHandles[2] = {NULL, NULL};
      bool          demoFinished = FALSE;

      waitHandles[numWaitHandles++] = GetStdHandle(STD_INPUT_HANDLE);
      waitHandles[numWaitHandles++] = notificationHandle;

      while(!demoFinished) {
        DWORD         waitResult = 0;

        waitResult = WaitForMultipleObjects(numWaitHandles, waitHandles, FALSE, INFINITE);

        switch(waitResult) {
        case WAIT_OBJECT_0: //Input event (from console) received
          if(XL_SUCCESS != (result = handleInputEvent(&demoFinished))) {
          }
          break;
        case WAIT_OBJECT_0+1: //XL-API event received
          if(XL_SUCCESS != (result = handleEthernetEvent(port, &xlapi, &demoFinished))) {
          }
          break;
        case WAIT_TIMEOUT:  //The wait timeout expired. Abort test.
          printf("Timeout detected! Abort test.\n");
          demoFinished = true;
          break;
        default:
          break;
        }
      }
    }

    if(XL_SUCCESS != (result = xlapi.xlDeactivateChannel(port, accessMask))) {
      printf("Error, failed to deactivate channel! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    if(XL_SUCCESS != (result = xlapi.xlClosePort(port))) {
      printf("Error, failed to close port! Reason: %s\n", xlapi.xlGetErrorString(result));
    }
    if(XL_SUCCESS != (result = xlapi.xlCloseDriver())) {
      printf("Error, failed to close driver! Reason: %u\n", result);
    }
    if(XL_SUCCESS != (result = unloadDLL(&vectorDll, &xlapi))) {
      printf("Error, failed to unload Vector DLL! Reason: %u\n", result);
    }
  }

  return 0;
}