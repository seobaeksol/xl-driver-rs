/*----------------------------------------------------------------------------
| File        : xlCANFunctions.cpp
| Project     : Vector CAN Example 
|
| Description : Shows the basic CAN functionality for the XL Driver Library
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlCANcontrol.h"
#include "xlCANFunctions.h"
#include "debug.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// globals
//////////////////////////////////////////////////////////////////////

TStruct g_th;
BOOL    g_bThreadRun;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCANFunctions::CCANFunctions()
{
  m_xlChannelMask[CHAN01] = 0;
  m_xlChannelMask[CHAN02] = 0;
  m_xlChannelIndex[CHAN01] = -1;
  m_xlChannelIndex[CHAN02] = -1;
  m_xlDrvConfig = {};
}

CCANFunctions::~CCANFunctions()
{
  if (m_bInitDone) {
    CloseHandle(m_hThread);
    CloseHandle(m_hMsgEvent);
    m_bInitDone = FALSE;
  }
}

////////////////////////////////////////////////////////////////////////////

//! CANInit

//! Open the driver, get the channelmasks and create the RX thread.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CCANFunctions::CANInit()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  m_bInitDone = FALSE;

  xlStatus = xlOpenDriver();
  sprintf_s(tmp, sizeof(tmp), "xlOpenDriver, stat: %d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus != XL_SUCCESS) {
    AfxMessageBox("Error when opening driver!\nMaybe the DLL is too old.");
    return xlStatus;
  }

  // ---------------------------------------
  // Get/Set the application within VHWConf
  // ---------------------------------------
  xlStatus = canGetChannelMask();
  if ( (xlStatus) || (m_xlChannelMask[CHAN01] == 0) || (m_xlChannelMask[CHAN02] == 0) ) return XL_ERROR;

  // ---------------------------------------
  // Open ONE port for both channels
  // ---------------------------------------
  xlStatus = canInit();
  if (xlStatus) return xlStatus;

  // ---------------------------------------
  // Create ONE thread for both channels
  // ---------------------------------------
  xlStatus = canCreateRxThread();
  if (xlStatus) return xlStatus;

  m_bInitDone = TRUE;

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! canGetChannelMask

//! parse the registry to get the channelmask
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CCANFunctions::canGetChannelMask()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  // default values
  unsigned int  hwType     = 0;
  unsigned int  hwIndex    = 0;
  unsigned int  hwChannel  = 0;
  unsigned int  appChannel = 0;
  unsigned int  busType    = XL_BUS_TYPE_CAN;   
  unsigned int  i;
  unsigned int  chan1Found = 0;
  unsigned int  chan2Found = 0;

   //check for hardware:
  xlStatus = xlGetDriverConfig(&m_xlDrvConfig);
  if (xlStatus) return xlStatus;
  
  // we check only if there is an application registered or not.
  xlStatus = xlGetApplConfig("xlCANcontrol", CHAN01, &hwType, &hwIndex, &hwChannel, busType); 
 
  // Set the params into registry (default values...!)
  if (xlStatus) {
    XLDEBUG(DEBUG_ADV,"set in VHWConf");

    for (i=0; i < m_xlDrvConfig.channelCount; i++) {

      sprintf_s (tmp, sizeof(tmp), "hwType: %d, bustype: %d, hwChannel: %d, cap: 0x%x", 
        m_xlDrvConfig.channel[i].hwType, 
        m_xlDrvConfig.channel[i].connectedBusType,
        m_xlDrvConfig.channel[i].hwChannel,
        m_xlDrvConfig.channel[i].channelBusCapabilities);
      XLDEBUG(DEBUG_ADV,tmp);

      // we search not the first CAN cabs
      if ( (m_xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_CAN) && (appChannel < 2) ) {

        hwType    = m_xlDrvConfig.channel[i].hwType;
        hwIndex   = m_xlDrvConfig.channel[i].hwIndex;
        hwChannel = m_xlDrvConfig.channel[i].hwChannel;

        xlStatus = xlSetApplConfig( // Registration of Application with default settings
        "xlCANcontrol",             // Application Name
        appChannel,                 // Application channel 0 or 1
        hwType,                     // hwType  (CANcardXL...)    
        hwIndex,                    // Index of hardware (slot) (0,1,...)
        hwChannel,                  // Index of channel (connector) (0,1,...)
        busType);                   // the application is for CAN.

        m_xlChannelMask[appChannel] = xlGetChannelMask(hwType, hwIndex, hwChannel);
        m_xlChannelIndex[appChannel] = xlGetChannelIndex(hwType, hwIndex, hwChannel);
        sprintf_s (tmp, sizeof(tmp), "Register CAN hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[appChannel]);
        XLDEBUG(DEBUG_ADV,tmp);

        appChannel++;
      }
     
    }
  }
  // application is registered, get two CAN channels which are assigned
  else {
    
    // get the first channel (check if channel is assigned)
    m_xlChannelMask[CHAN01] = xlGetChannelMask(hwType, hwIndex, hwChannel);
    m_xlChannelIndex[CHAN01] = xlGetChannelIndex(hwType, hwIndex, hwChannel);
    sprintf_s (tmp, sizeof(tmp), "Found CAN in VHWConf, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN01]);
    XLDEBUG(DEBUG_ADV,tmp);
 
    for (i=0; i < m_xlDrvConfig.channelCount; i++) {

      if ((m_xlDrvConfig.channel[i].channelMask == m_xlChannelMask[CHAN01]) &&
          (m_xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_CAN)) {
        chan1Found = 1;
        break;
       }
    }
  
    if (!chan1Found) {
      sprintf_s (tmp, sizeof(tmp), "No assigned CAN in VHWConf for channel 0, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN01]);
      XLDEBUG(DEBUG_ADV,tmp);
      return XL_ERROR;
    }


    // get the second channel
    xlStatus = xlGetApplConfig("xlCANcontrol", CHAN02, &hwType, &hwIndex, &hwChannel, busType); 
    if (xlStatus) return xlStatus;

    m_xlChannelMask[CHAN02] = xlGetChannelMask(hwType, hwIndex, hwChannel);
    m_xlChannelIndex[CHAN02] = xlGetChannelIndex(hwType, hwIndex, hwChannel);
    sprintf_s (tmp, sizeof(tmp), "Found CAN in VHWConf, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN02]);
    XLDEBUG(DEBUG_ADV,tmp);

    for (i=0; i < m_xlDrvConfig.channelCount; i++) {

      if ((m_xlDrvConfig.channel[i].channelMask == m_xlChannelMask[CHAN02]) &&
          (m_xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_CAN)) {
        chan2Found = 1;
        break;
      }
    }

    if (!chan2Found) {
      sprintf_s (tmp, sizeof(tmp), "No assigned CAN in VHWConf for channel 1, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN01]);
      XLDEBUG(DEBUG_ADV,tmp);
      return XL_ERROR;
    }

  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! canInit

//! xlCANcontrol use ONE port for both channels.
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CCANFunctions::canInit()
{
  XLstatus         xlStatus = XL_ERROR;
  XLaccess         xlPermissionMask;
  char             tmp[100];

  // ---------------------------------------
  // Open ONE port for both channels 
  // ---------------------------------------

  // calculate the channelMask for both channel 
  m_xlChannelMask_both = m_xlChannelMask[CHAN01] + m_xlChannelMask[CHAN02];
  xlPermissionMask = m_xlChannelMask_both;

  xlStatus = xlOpenPort(&m_xlPortHandle, "xlCANcontrol", m_xlChannelMask_both, &xlPermissionMask, 256, XL_INTERFACE_VERSION, XL_BUS_TYPE_CAN); 
  sprintf_s(tmp, sizeof(tmp), "xlOpenPort: PortHandle: %d; Permissionmask: 0x%I64x; Status: %d", m_xlPortHandle, xlPermissionMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if (m_xlPortHandle == XL_INVALID_PORTHANDLE) return XL_ERROR;
  if (xlStatus == XL_ERR_INVALID_ACCESS) return xlStatus;

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! canCreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CCANFunctions::canCreateRxThread()
{
  
  XLstatus      xlStatus = XL_ERROR;
  DWORD         ThreadId=0;
  char          tmp[100];
  
  if (m_xlPortHandle!= XL_INVALID_PORTHANDLE) {

    // Send a event for each Msg!!!
    xlStatus = xlSetNotification (m_xlPortHandle, &m_hMsgEvent, 1);
    sprintf_s(tmp, sizeof(tmp), "SetNotification '%p', xlStatus: %d", m_hMsgEvent, xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);

    // for the RxThread
    g_th.xlPortHandle = m_xlPortHandle;
    g_th.hMsgEvent    = m_hMsgEvent; 
    g_th.pOutput      = m_pOutput;
   
    m_hThread = CreateThread(0, 0x1000, RxThread, (LPVOID) &g_th, 0, &ThreadId);
    sprintf_s(tmp, sizeof(tmp), "CreateThread %p", m_hThread);
    XLDEBUG(DEBUG_ADV, tmp);
    
  }
  return xlStatus;
}


////////////////////////////////////////////////////////////////////////////

//! CANGoOnBus

//! set the selected baudrate and go on bus.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CCANFunctions::CANGoOnBus(unsigned long baudrate)
{
  XLstatus      xlStatus = XL_ERROR;
  char          tmp[100];

  xlStatus = xlCanSetChannelBitrate(m_xlPortHandle, m_xlChannelMask_both, baudrate);
  sprintf_s(tmp, sizeof(tmp), "SetBaudrate: %d, stat: %d", baudrate, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlActivateChannel(m_xlPortHandle, m_xlChannelMask_both, XL_BUS_TYPE_CAN, XL_ACTIVATE_RESET_CLOCK);
  sprintf_s(tmp, sizeof(tmp), "ActivateChannel, stat: %d",  xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! CANGoOffBus

//! Deactivate the channel
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CCANFunctions::CANGoOffBus()
{
  XLstatus      xlStatus = XL_ERROR;
  char          tmp[100];

  xlStatus = xlDeactivateChannel(m_xlPortHandle, m_xlChannelMask_both);
  sprintf_s(tmp, sizeof(tmp), "DeactivateChannel, stat: %d",  xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! CANSend

//! transmit a CAN message to the selected channel with the give values.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CCANFunctions::CANSend(XLevent xlEvent, int channel)
{
  
  XLstatus      xlStatus;
  char          tmp[100];
  unsigned int  messageCount = 1;

  xlStatus = xlCanTransmit(m_xlPortHandle, m_xlChannelMask[channel], &messageCount, &xlEvent);
  sprintf_s(tmp, sizeof(tmp), "Transmit, mc: %d, channel: %d, stat: %d",  messageCount, channel, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! CANResetFilter

//! Reset the acceptancefilter
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CCANFunctions::CANResetFilter()
{
  XLstatus      xlStatus;
  char          tmp[100];

  xlStatus = xlCanResetAcceptance(m_xlPortHandle, m_xlChannelMask_both, XL_CAN_STD);
  sprintf_s(tmp, sizeof(tmp), "CanResetAcceptance, stat: %d",  xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! CANSetFilter

//! Reset the acceptancefilter
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CCANFunctions::CANSetFilter(unsigned long first_id, unsigned long last_id)
{
  XLstatus      xlStatus;
  char          tmp[100];
  
  // because there all filters open, we close all.
  xlStatus = xlCanSetChannelAcceptance(m_xlPortHandle, m_xlChannelMask_both, 0xFFF, 0xFFF, XL_CAN_STD);
  sprintf_s(tmp, sizeof(tmp), "CanSetChannelAcceptance, stat: %d",  xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  
  // and now we can set the acceptance filter range.
  xlStatus = xlCanAddAcceptanceRange(m_xlPortHandle, m_xlChannelMask_both, first_id, last_id);
  sprintf_s(tmp, sizeof(tmp), "CanAddAcceptanceRange, firstID: %d, lastID: %d, stat: %d",  first_id, last_id, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  
  
  return xlStatus;
}

///////////////////////////////////////////////////////////////////////////

//! GetChannelName

//! Retrieve the name of a channel from the driver config.
//!
////////////////////////////////////////////////////////////////////////////
const char* CCANFunctions::GetChannelName(int channel)
{
  if (channel != CHAN01 && channel != CHAN02) {
    return "<no channel>";
  }

  int chIdx = m_xlChannelIndex[channel];
  
  if (chIdx < 0 || (unsigned)chIdx >= m_xlDrvConfig.channelCount) {
    return "<no channel>";
  }

  return m_xlDrvConfig.channel[chIdx].name;
}

///////////////////////////////////////////////////////////////////////////

//! AppendLicenseInfo

//! Reads license of one channel and write result into cstr.
//!
////////////////////////////////////////////////////////////////////////////
void CCANFunctions::AppendLicenseInfo(CString& cstr, int channel)
{
  // Show available licenses
  XLstatus xlStatus;
  XLlicenseInfo licenseArray[1024];
  unsigned int licArraySize = 1024;

  cstr.AppendFormat("%s:\n", GetChannelName(channel));
  xlStatus = xlGetLicenseInfo(m_xlChannelMask[channel], licenseArray, licArraySize);
  if (xlStatus == XL_SUCCESS) {
    int numLicensesFound = 0;
    for (unsigned int i = 0; i < licArraySize; i++) {
      if (licenseArray[i].bAvailable) {
        cstr.AppendFormat("  ID 0x%03x: %s\n", i, licenseArray[i].licName);
        numLicensesFound++;
      }
    }
    if (numLicensesFound == 0) {
      cstr.Append("  - no licenses -\n");
    }
  }
  else {
    cstr.AppendFormat("  xlGetLicenseInfo() returned %s\n", xlGetErrorString(xlStatus));
  }
}


///////////////////////////////////////////////////////////////////////////

//! RxThread

//! thread to readout the message queue and parse the incoming messages
//!
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI RxThread(LPVOID par) 
{

  XLstatus        xlStatus;

  unsigned int    msgsrx = 1;
  XLevent         xlEvent; 
  char            tmp[110];
  CString         str;
  
  g_bThreadRun = TRUE;

  TStruct *pTh;

  pTh = (TStruct*) par;  

  sprintf_s(tmp, sizeof(tmp), "thread: SetNotification '%p'", pTh->hMsgEvent);
  XLDEBUG(DEBUG_ADV, tmp);

  while (g_bThreadRun) { 
   
    WaitForSingleObject(pTh->hMsgEvent,10);
    
    xlStatus = XL_SUCCESS;
   
    while (!xlStatus) {
        
    msgsrx = 1;
    xlStatus = xlReceive(pTh->xlPortHandle, &msgsrx, &xlEvent);	
    
    if ( xlStatus!=XL_ERR_QUEUE_IS_EMPTY ) {
          
        sprintf_s(tmp, sizeof(tmp), "%s", xlGetEventString(&xlEvent));
        XLDEBUG(DEBUG_ADV, tmp);
        pTh->pOutput->InsertString(-1, tmp);

        pTh->pOutput->SetCurSel(pTh->pOutput->GetCount()-1);
      }
    
    }
          
  }
  return NO_ERROR; 
}
