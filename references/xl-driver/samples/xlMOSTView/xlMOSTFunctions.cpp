/*-------------------------------------------------------------------------------------------
| File        : xlMOSTFunctions.cpp
| Project     : Vector MOST Example 
|
| Description : Shows the basic MOST functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2005 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include ".\xlmostfunctions.h"
#include "vxlapi.h"
#include "debug.h"

#define ERROR_TIME         100
#define DEFAULT_USERHANDLE 0xab

//////////////////////////////////////////////////////////////////////
// globals
//////////////////////////////////////////////////////////////////////

BOOL    g_bThreadRun;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMOSTFunctions::CMOSTFunctions(void)
{
  unsigned int nChan;

  // setup the default node parameters
  for (nChan = 0; nChan < XL_CONFIG_MAX_CHANNELS; nChan++) {
    m_NodeParam[nChan].xlPortHandle   = XL_INVALID_PORTHANDLE;
    m_NodeParam[nChan].xlChannelMask  = 0;
    m_NodeParam[nChan].nNodeAdr       = 0x0fff;
    m_NodeParam[nChan].nGroupAdr      = 0x0300;
    m_NodeParam[nChan].nFrequency     = XL_MOST_FREQUENCY_44100;
    m_NodeParam[nChan].nBypass        = XL_MOST_MODE_DEACTIVATE;
    m_NodeParam[nChan].nTimingMode    = XL_MOST_TIMING_SLAVE;
  }
}

CMOSTFunctions::~CMOSTFunctions(void)
{
}

////////////////////////////////////////////////////////////////////////////

//! MOSTInit

//! Open the driver, get the channelmasks
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::MOSTInit()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[300];
  unsigned int    nbrOfBoxes;
  unsigned int    boxMask;
  unsigned int    boxSerial;
  unsigned int    i;
  XLuint64        licInfo[4], tmpLicInfo[4];

  xlStatus = xlOpenDriver();
  sprintf(tmp, "xlOpenDriver, stat: %d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus != XL_SUCCESS) return xlStatus;

  // ---------------------------------------
  // get license of keyman dongle 
  // ---------------------------------------
  memset(licInfo, 0, sizeof(licInfo));
  xlStatus = xlGetKeymanBoxes(&nbrOfBoxes);
  if (xlStatus == XL_SUCCESS) {
    sprintf(tmp, "xlGetKeymanBoxes: %d Keyman License Dongle(s) found!\n", nbrOfBoxes);
    XLDEBUG(DEBUG_ADV, tmp);
    for (i = 0; i<nbrOfBoxes; i++) {
      memset(tmpLicInfo, 0, sizeof(tmpLicInfo));
      xlStatus = xlGetKeymanInfo(i, &boxMask, &boxSerial, tmpLicInfo);
      if (xlStatus == XL_SUCCESS) {
        sprintf(tmp, "xlGetKeymanInfo: Keyman Dongle (%d) with SerialNumber: %d-%d\n", i, boxMask, boxSerial);
        XLDEBUG(DEBUG_ADV, tmp);
        licInfo[0] |= tmpLicInfo[0];
        licInfo[1] |= tmpLicInfo[1];
        licInfo[2] |= tmpLicInfo[2];
        licInfo[3] |= tmpLicInfo[3];
      }
    }
    sprintf(tmp, "xlGetKeymanInfo: licInfo[0]=0x%I64x, licInfo[1]=0x%I64x, licInfo[2]=0x%I64x, licInfo[3]=0x%I64x\n",
      licInfo[0], licInfo[1], licInfo[2], licInfo[3]);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  // ---------------------------------------
  // Get/Set the application within VHWConf
  // ---------------------------------------
  xlStatus = mostGetChannelMask();
 
  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTClose

//! stops the threads, close the porthandles and afterall close the driver.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::MOSTClose()
{
  // ---------------------------------------
  // stops the RX thread and close the ports
  // ---------------------------------------
  mostClose();

  // ---------------------------------------
  // unloads the DLL
  // ---------------------------------------
  xlCloseDriver();

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTActivate

//! opens the port, creates the RX thread, activate the device and 
//! open the eventsources
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::MOSTActivate(unsigned int nChan)
{
  XLstatus        xlStatus = XL_ERROR;
  
  m_channel = nChan;

  // ---------------------------------------
  // Open the ports
  // ---------------------------------------
  xlStatus = mostInit();
  if (xlStatus != XL_SUCCESS) {
    char str[256];
    sprintf_s(str, sizeof(str), "Initializing MOST failed with errorcode=%d!", xlStatus);
    AfxMessageBox(str);
    return xlStatus;
  }

  // ---------------------------------------
  // Create for every device one thread
  // ---------------------------------------
  xlStatus = mostCreateRxThread();
  if (xlStatus != XL_SUCCESS) {
    char str[256];
    sprintf_s(str, sizeof(str), "Creating Rx-Thread failed with errorcode=%d!", xlStatus);
    AfxMessageBox(str);
    return xlStatus;
  }

  // ---------------------------------------
  // activate the channel
  // ---------------------------------------
  xlStatus = mostActivate();

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTDeActivate

//! deactivate the device
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::MOSTDeActivate()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  if (m_NodeParam[m_channel].xlPortHandle == XL_INVALID_PORTHANDLE) return xlStatus;

  xlStatus = xlDeactivateChannel(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask);
  sprintf(tmp, "xlDeactivateChannel, PH: %d, stat: %d", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // close the ports, stop the RX thread
  // ---------------------------------------
  mostClose();

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! mostClose

//! stops the threads, close the porthandles and afterall close the driver.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::mostClose()
{
  //unsigned int    nChan;
  XLstatus        xlStatus;
  char            tmp[100];

  if (g_bThreadRun == TRUE) g_bThreadRun = FALSE;

  // Wait until the threads is gone
  WaitForSingleObject(m_NodeParam[m_channel].hThread, 10);
  xlStatus = xlClosePort(m_NodeParam[m_channel].xlPortHandle);
  sprintf(tmp, "xlClosePort, PH: %d, stat: %d", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
	CloseHandle(m_NodeParam[m_channel].hThread);

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! mostActivate

//! activate the device and open the eventsources
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::mostActivate()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  unsigned short  sourceMask; 

  // we switch on all possible events. It's depending on the licence you have.
  sourceMask = XL_MOST_SOURCE_ASYNC_SPY | XL_MOST_SOURCE_ASYNC_RX | XL_MOST_SOURCE_ASYNC_TX | XL_MOST_SOURCE_CTRL_OS8104A | XL_MOST_SOURCE_CTRL_SPY | XL_MOST_SOURCE_SYNCLINE | XL_MOST_SOURCE_ASYNC_RX_FIFO_OVER;
  
  xlStatus = xlActivateChannel(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, XL_BUS_TYPE_MOST, XL_ACTIVATE_RESET_CLOCK);
  sprintf(tmp, "xlActivateChannel, PH: %d, stat: %d",  m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMostSwitchEventSources(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, sourceMask);
  sprintf(tmp, "xlMostSwitchEventSources, PH: %d, sourceMask: 0x%x, stat: %d",  
    m_NodeParam[m_channel].xlPortHandle, sourceMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! mostSetApplConfig

//! parse driver config structure
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::mostSetApplConfig(XLdriverConfig xlDrvConfig, unsigned int appChannel)
{
  XLstatus      xlStatus;
  // default values
  unsigned int  hwType     = 0;
  unsigned int  hwIndex    = 0;
  unsigned int  hwChannel  = 0;
  unsigned int  busType    = XL_BUS_TYPE_MOST;   
  unsigned int  nChan; 
  char          tmp[100];

  // parse the structure
  for (nChan=0; nChan < xlDrvConfig.channelCount; nChan++) {

    sprintf (tmp, "CI: %d, CM: 0x%I64x, hwType: %d, bustype: %d, hwChannel: %d, cap: 0x%x", 
        xlDrvConfig.channel[nChan].channelIndex,
        xlDrvConfig.channel[nChan].channelMask,
        xlDrvConfig.channel[nChan].hwType, 
        xlDrvConfig.channel[nChan].connectedBusType,
        xlDrvConfig.channel[nChan].hwChannel,
        xlDrvConfig.channel[nChan].channelBusCapabilities);
      XLDEBUG(DEBUG_ADV,tmp);

      // we search the MOST channels 
      if ( (xlDrvConfig.channel[nChan].channelBusCapabilities & XL_BUS_ACTIVE_CAP_MOST)
        && (mostCheckChannelMask(xlDrvConfig.channel[nChan].channelMask)) ) {

        hwType    = xlDrvConfig.channel[nChan].hwType;
        hwIndex   = xlDrvConfig.channel[nChan].hwIndex;
        hwChannel = xlDrvConfig.channel[nChan].hwChannel;

        xlStatus = xlSetApplConfig( // Registration of Application with default settings
        "xlMOSTView",               // Application Name
        appChannel,                 // Application channel 0 or 1
        hwType,                     // hwType  (CANcardXL...)    
        hwIndex,                    // Index of hardware (slot) (0,1,...)
        hwChannel,                  // Index of channel (connector) (0,1,...)
        busType);                   // the application is for MOST.

        m_NodeParam[appChannel].xlChannelMask = xlGetChannelMask(hwType, hwIndex, hwChannel);
        sprintf (tmp, "Register MOST hWType: %d, hwIndex: %d,  CM: 0x%I64x", 
          hwType, hwChannel, m_NodeParam[appChannel].xlChannelMask);
        XLDEBUG(DEBUG_ADV,tmp);

        sprintf(tmp, "%s, (%06d)",xlDrvConfig.channel[nChan].name, xlDrvConfig.channel[nChan].serialNumber);
        m_plDevice->AddString(tmp);
     
        break;
      }
  }

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! mostCheckChannelMask

//! checks if the channel is allready assigned.
//!
//////////////////////////////////////////////////////////////////////////// 

BOOL CMOSTFunctions::mostCheckChannelMask(XLaccess xlChannelMask)
{
  BOOL bStatus  = TRUE;
  
  for (int i=0; i<XL_CONFIG_MAX_CHANNELS; i++) {
    if (m_NodeParam[i].xlChannelMask == xlChannelMask) {
      XLDEBUG(DEBUG_ADV,"Channel allready assigned");
      bStatus = FALSE;
    }
  }

  return bStatus;
}

////////////////////////////////////////////////////////////////////////////

//! mostGetChannelMask

//! parse the registry to get the channelmask
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::mostGetChannelMask()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  // default values
  unsigned int  hwType     = 0;
  unsigned int  hwIndex    = 0;
  unsigned int  hwChannel  = 0;
  unsigned int  busType    = XL_BUS_TYPE_MOST;  
  unsigned int  appChannel = 0;
  unsigned int  i;
  unsigned int  nChan;
  unsigned int  nChannelArray = 0;

  XLdriverConfig  xlDrvConfig;

  //check for hardware:
  xlStatus = xlGetDriverConfig(&xlDrvConfig);
  if (xlStatus) return xlStatus;

  m_nChannelCount = 0;

  // at first we check how much MOST devices we have
  for (nChan = 0; nChan < xlDrvConfig.channelCount; nChan++) {
    if ( xlDrvConfig.channel[nChan].channelBusCapabilities & XL_BUS_COMPATIBLE_MOST) {
      m_nChannelCount++;
    }
  }
  sprintf (tmp, "Found %d MOST channels", m_nChannelCount);
  XLDEBUG(DEBUG_ADV,tmp);
  if (m_nChannelCount == 0) {
    m_plDevice->AddString("NO MOST device found!");
    return XL_ERROR;
  }

  // get the channelmasks
  nChan = 0;
  while (nChan < m_nChannelCount) {

    // we check only if there is an application registered or not.
    xlStatus = xlGetApplConfig("xlMOSTView", appChannel, &hwType, &hwIndex, &hwChannel, busType); 
 
    // Set the params into registry (default values...!)
    if (xlStatus) {
      if (!mostSetApplConfig(xlDrvConfig, appChannel)) {
        appChannel++;
        nChan++;
        nChannelArray++;
      }
    }
    else {
      m_NodeParam[nChannelArray].xlChannelMask = xlGetChannelMask(hwType, hwIndex, hwChannel);

      // maybe we have more applications assigned... but less in the system
      if (m_NodeParam[nChannelArray].xlChannelMask) {
        sprintf (tmp, "Found MOST in VHWConf, hWType: %d, hwChannel: %d, CM: 0x%I64x, channel: %d", 
                hwType, hwChannel, m_NodeParam[nChannelArray].xlChannelMask, nChan);
        XLDEBUG(DEBUG_ADV,tmp);

        // grab the string names out of the DrvConfig structure...
        for (i=0; i<xlDrvConfig.channelCount; i++) {
          if (xlDrvConfig.channel[i].channelMask == m_NodeParam[nChannelArray].xlChannelMask){
            sprintf(tmp, "%s, (%06d)",xlDrvConfig.channel[i].name, xlDrvConfig.channel[i].serialNumber);
            m_plDevice->AddString(tmp);
          }
        }
        nChan++;
        nChannelArray++;
      }
      appChannel++;
      
    }  
  }
  
  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! mostInit

//! opens the found devices
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::mostInit()
{
  XLstatus         xlStatus = XL_ERROR;
  XLaccess         xlPermissionMask;
  char             tmp[100];
 
  xlPermissionMask = m_NodeParam[m_channel].xlChannelMask;

  xlStatus = xlOpenPort(&m_NodeParam[m_channel].xlPortHandle, "xlMOSTView", m_NodeParam[m_channel].xlChannelMask, &xlPermissionMask, 0x10000, XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_MOST); // receive buffer size is 65536
  sprintf(tmp, "xlOpenPort: PortHandle: %d; Permissionmask: 0x%I64x; Status: %d", m_NodeParam[m_channel].xlPortHandle, xlPermissionMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! mostCreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOSTFunctions::mostCreateRxThread()
{
  XLstatus       xlStatus;
  DWORD          ThreadId=0;
  char           tmp[100];

  static TStruct th[XL_CONFIG_MAX_CHANNELS];
     
  if (m_NodeParam[m_channel].xlPortHandle == XL_INVALID_PORTHANDLE) return XL_ERROR;

  // Send a event for each Msg!!!
  xlStatus = xlSetNotification (m_NodeParam[m_channel].xlPortHandle, &m_NodeParam[m_channel].hMsgEvent, 1);
  sprintf(tmp, "SetNotification, xlStatus: %d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // for the RxThread
  th[m_channel].xlPortHandle = m_NodeParam[m_channel].xlPortHandle;
  th[m_channel].hMsgEvent    = m_NodeParam[m_channel].hMsgEvent; 
  th[m_channel].pOutput      = m_pListBox;
  th[m_channel].pMostFunc      = this;
  
  m_NodeParam[m_channel].hThread = CreateThread(0, 0x1000, RxThread, (LPVOID)&th[m_channel], 0, &ThreadId);
  
  m_plRxBox = m_pListBox;

  xlStatus = MOSTSetupNode(m_NodeParam[m_channel].nTimingMode, m_NodeParam[m_channel].nFrequency, 
                           m_NodeParam[m_channel].nNodeAdr, m_NodeParam[m_channel].nGroupAdr,m_NodeParam[m_channel].nBypass);
  
  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTToggleLight

//! Toggle the TX light
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOSTFunctions::MOSTToggleLight(unsigned int nToggleLight)
{
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  xlStatus = xlMostSetTxLight(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, (unsigned char)nToggleLight);
  sprintf_s(tmp, sizeof(tmp), "xlMostSetTxLight PH: %d, Toggle: %d, stat:%d", m_NodeParam[m_channel].xlPortHandle, nToggleLight, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! MOSTSetupNode

//! setup a node depending on the dialog box parameter
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOSTFunctions::MOSTSetupNode(unsigned int nTimingMode, 
                                       unsigned int nFrequency, unsigned short nNodeAdr, unsigned short nGroupAdr,
                                       unsigned char nBypass)
{
  XLstatus      xlStatus = XL_ERROR;
  char          tmp[100];
  unsigned char regVal[1];

  // save the node parameters.
  m_NodeParam[m_channel].nTimingMode = nTimingMode;
  m_NodeParam[m_channel].nFrequency  = nFrequency;
  m_NodeParam[m_channel].nBypass     = nBypass;
  m_NodeParam[m_channel].nNodeAdr    = nNodeAdr;
  m_NodeParam[m_channel].nGroupAdr   = nGroupAdr;

  xlStatus = xlMostSetTimingMode(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, (unsigned char)nTimingMode);
  sprintf(tmp, "xlMostSetTimingMode, timingMode: %d, stat:%d", nTimingMode, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if ((nTimingMode == XL_MOST_TIMING_MASTER)
    || (nTimingMode == XL_MOST_TIMING_MASTER_SPDIF_MASTER)
    || (nTimingMode == XL_MOST_TIMING_MASTER_SPDIF_SLAVE)
    || (nTimingMode == XL_MOST_TIMING_MASTER_FROM_SPDIF_SLAVE)) {
    // write SBC register of OS8104 (only in master mode)
    regVal[0] = 11;    // just a valid value
    xlStatus = xlMostWriteRegister(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, 0x96, 1, regVal);
    sprintf(tmp, "xlMostWriteRegister, bSBC (Adr: 0x96) = %d", regVal[1]);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  xlStatus = xlMostSetFrequency(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, (unsigned short)nFrequency);
  sprintf(tmp, "xlMostSetFrequency, frequency: %d, stat:%d", nFrequency, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMostSetAllBypass(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, nBypass);
  sprintf(tmp, "xlMostSetAllBypass, bypassMode: %d, stat:%d", nBypass, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  unsigned char data[16];
  data[0] = (unsigned char) nGroupAdr;                   // Groupaddress is only one Byte -> High byte = 0x03
  data[1] = (unsigned char) ((nNodeAdr & 0xFF00) >> 8);  // High byte of the node address.
  data[2] = (unsigned char) nNodeAdr;                    // Low byte of the node address.
  
  sprintf(tmp, "xlMostSetFrequency, nNode: 0x%x, nGroup: 0x%x", nNodeAdr, nGroupAdr);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMostWriteRegister(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, XL_MOST_bGA, 3, data);
  sprintf(tmp, "xlMostWriteRegister, stat:%d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! MOSTGetInfo

//! request the node info's.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOSTFunctions::MOSTGetInfo()
{
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  xlStatus = xlMostGetTimingMode(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMostGetTimingMode, stat:%d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMostGetAllBypass(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMostGetAllBypass, stat:%d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMostGetFrequency(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMostGetFrequency, stat:%d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}
////////////////////////////////////////////////////////////////////////////

//! MOSTCtrlTransmit

//! transmit a control frame
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOSTFunctions::MOSTCtrlTransmit(unsigned short nTargetAdr)
{
  XLstatus      xlStatus = XL_ERROR;
  XLmostCtrlMsg xlCtrlMsg;  
  char          tmp[100];
  
  // here our data
  for (int i = 0; i<17;i++) xlCtrlMsg.ctrlData[i] = 0;

  xlCtrlMsg.ctrlPrio      = 0xFF;
  xlCtrlMsg.ctrlType      = 0;
  xlCtrlMsg.targetAddress = nTargetAdr;
  xlCtrlMsg.direction     = XL_MOST_DIRECTION_TX;

  xlStatus  = xlMostCtrlTransmit(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, &xlCtrlMsg);
  sprintf(tmp, "xlMostCtrlTransmit, TAdr: 0x%x, SAdr: 0x%x, stat:%d", 
    xlCtrlMsg.targetAddress, xlCtrlMsg.sourceAddress, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! MOSTTwinklePowerLED

//! The power LED of the selected device will twinkle
//!
////////////////////////////////////////////////////////////////////////////
  
XLstatus CMOSTFunctions::MOSTTwinklePowerLED()
{
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  xlStatus = xlMostTwinklePowerLed(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMostTwinklePowerLed, stat:%d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTGenerateLightError

//! generate 'nRepeat' light errors.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::MOSTGenerateLightError(unsigned short nRepeat)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  xlStatus = xlMostGenerateLightError(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, 100, 100, nRepeat);
  sprintf(tmp, "xlMostGenerateLightError, PH: %d, stat: %d", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTGenerateLockError

//! generate 'nRepeat' lock errors.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTFunctions::MOSTGenerateLockError(unsigned short nRepeat)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  xlStatus = xlMostGenerateLockError(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, 100, 100, nRepeat);
  sprintf(tmp, "xlMostGenerateLockError, PH: %d, stat: %d", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

///////////////////////////////////////////////////////////////////////////

//! RxThread

//! thread to readout the message queue and parse the incoming messages
//!
///////////////////////////////////////////////////////////////////////////

DWORD WINAPI RxThread(LPVOID par) 
{

  XLstatus        xlStatus;
  XLmostEvent     xlMostEvent; 
  CString         str;
  
  TStruct *th;
  th = (TStruct*) par;  
  th->pMostFunc->m_plRxBox = th->pOutput; 

  g_bThreadRun = TRUE;

  while (g_bThreadRun) { 
   
    WaitForSingleObject(th->hMsgEvent, 100);

    xlStatus = XL_SUCCESS;
  
    while (!xlStatus) {

      xlStatus = xlMostReceive(th->xlPortHandle, &xlMostEvent);
      
      if ( xlStatus!=XL_ERR_QUEUE_IS_EMPTY ) {
         
        // Handle the MOST events
        th->pMostFunc->parseEvent(&xlMostEvent);   
      
        // Handle the streaming events
        if ((xlMostEvent.tag == XL_MOST_STREAM_STATE)
          || (xlMostEvent.tag == XL_MOST_STREAM_BUFFER)) {
          th->pMostFunc->MOSTStreamParse(&xlMostEvent);
        }

      }
    
    } 
  }
 
  return NO_ERROR; 
}
