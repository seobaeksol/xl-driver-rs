/*----------------------------------------------------------------------------
| File        : xlA429Functions.cpp
| Project     : Vector A429 Example 
|
| Description : Shows the basic A429 functionality for the XL Driver Library
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlA429control.h"
#include "xlA429Functions.h"
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

CA429Functions::CA429Functions()
{
  m_xlChannelMask[CHAN01] = 0;
  m_xlChannelMask[CHAN02] = 0;
}

CA429Functions::~CA429Functions()
{
  if (m_bInitDone) {
    CloseHandle(m_hThread);
    CloseHandle(m_hMsgEvent);
    m_bInitDone = FALSE;
  }
}

////////////////////////////////////////////////////////////////////////////

//! A429Init

//! Open the driver, get the channelmasks and create the RX thread.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CA429Functions::A429Init()
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
  xlStatus = a429GetChannelMask();
  if ( (xlStatus) || (m_xlChannelMask[CHAN01] == 0) || (m_xlChannelMask[CHAN02] == 0) ) return XL_ERROR;

  // ---------------------------------------
  // Open ONE port for both channels
  // ---------------------------------------
  xlStatus = a429Init();
  if (xlStatus) return xlStatus;

  // ---------------------------------------
  // Create ONE thread for both channels
  // ---------------------------------------
  xlStatus = a429CreateRxThread();
  if (xlStatus) return xlStatus;

  m_bInitDone = TRUE;

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! a429GetChannelMask

//! parse the registry to get the channelmask
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CA429Functions::a429GetChannelMask()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  // default values
  unsigned int  hwType     = 0;
  unsigned int  hwIndex    = 0;
  unsigned int  hwChannel  = 0;
  unsigned int  appChannel = 0;
  unsigned int  busType    = XL_BUS_TYPE_A429;   
  unsigned int  i;
  unsigned int  chan1Found = 0; // A429 TX channel
  unsigned int  chan2Found = 0; // A429 RX channel
 
  XLdriverConfig  xlDrvConfig;

   //check for hardware:
  xlStatus = xlGetDriverConfig(&xlDrvConfig);
  if (xlStatus) return xlStatus;
  
  // we check only if there is an application registered or not.
  xlStatus = xlGetApplConfig("xlA429control", CHAN01, &hwType, &hwIndex, &hwChannel, busType); 
 
  // Set the params into registry (default values...!)
  if (xlStatus) {
    XLDEBUG(DEBUG_ADV,"set in VHWConf");

    for (i=0; i < xlDrvConfig.channelCount; i++) {

      sprintf_s (tmp, sizeof(tmp), "hwType: %d, bustype: %d, hwChannel: %d, cap: 0x%x", 
        xlDrvConfig.channel[i].hwType, 
        xlDrvConfig.channel[i].connectedBusType,
        xlDrvConfig.channel[i].hwChannel,
        xlDrvConfig.channel[i].channelBusCapabilities);
      XLDEBUG(DEBUG_ADV,tmp);

      // we search not the first A429 cabs
      if ( (xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_A429) && (appChannel < 2) ) {

        if ( (xlDrvConfig.channel[i].busParams.data.a429.channelDirection == XL_A429_MSG_CHANNEL_DIR_TX) && (!chan1Found) ) {
          chan1Found = 1;

          hwType    = xlDrvConfig.channel[i].hwType;
          hwIndex   = xlDrvConfig.channel[i].hwIndex;
          hwChannel = xlDrvConfig.channel[i].hwChannel;

          xlStatus = xlSetApplConfig( // Registration of Application with default settings
            "xlA429control",            // Application Name
            appChannel,                 // Application channel 0 or 1
            hwType,                     // hwType  (CANcardXL...)    
            hwIndex,                    // Index of hardware (slot) (0,1,...)
            hwChannel,                  // Index of channel (connector) (0,1,...)
            busType);                   // the application is for A429.

          m_xlChannelMask[appChannel] = xlGetChannelMask(hwType, hwIndex, hwChannel);
          sprintf_s (tmp, sizeof(tmp), "Register A429 Tx hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[appChannel]);
          XLDEBUG(DEBUG_ADV,tmp);

          m_pHardware->InsertString(-1, xlDrvConfig.channel[i].name);

          appChannel++;
        
        }

        if ( (xlDrvConfig.channel[i].busParams.data.a429.channelDirection == XL_A429_MSG_CHANNEL_DIR_RX) && (!chan2Found) ) {
          chan2Found = 1;

          hwType    = xlDrvConfig.channel[i].hwType;
          hwIndex   = xlDrvConfig.channel[i].hwIndex;
          hwChannel = xlDrvConfig.channel[i].hwChannel;

          xlStatus = xlSetApplConfig( // Registration of Application with default settings
            "xlA429control",            // Application Name
            appChannel,                 // Application channel 0 or 1
            hwType,                     // hwType  (CANcardXL...)    
            hwIndex,                    // Index of hardware (slot) (0,1,...)
            hwChannel,                  // Index of channel (connector) (0,1,...)
            busType);                   // the application is for CAN.

          m_xlChannelMask[appChannel] = xlGetChannelMask(hwType, hwIndex, hwChannel);
          sprintf_s (tmp, sizeof(tmp), "Register A429 Rx hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[appChannel]);
          XLDEBUG(DEBUG_ADV,tmp);

          m_pHardware->InsertString(-1, xlDrvConfig.channel[i].name);

          appChannel++;
        
        }

      }
     
    }
  }
  // application is registered, get two A429 channels which are assigned
  else {
    
    // get the first channel (check if channel is assigned)
	  m_xlChannelMask[CHAN01] = xlGetChannelMask(hwType, hwIndex, hwChannel);
    sprintf_s (tmp, sizeof(tmp), "Found A429 Tx in VHWConf, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN01]);
    XLDEBUG(DEBUG_ADV,tmp);
 
    for (i=0; i < xlDrvConfig.channelCount; i++) {
      if ((xlDrvConfig.channel[i].channelMask == m_xlChannelMask[CHAN01]) &&
          (xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_A429) &&
          (xlDrvConfig.channel[i].busParams.data.a429.channelDirection == XL_A429_MSG_CHANNEL_DIR_TX)) {
          m_pHardware->AddString(xlDrvConfig.channel[i].name);
		      chan1Found = 1;
		      break;
	    }
    }
	
    if (!chan1Found) {
      sprintf_s (tmp, sizeof(tmp), "No assigned A429 Tx in VHWConf for channel 0, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN01]);
      XLDEBUG(DEBUG_ADV,tmp);
      return XL_ERROR;
    }

    // get the second channel
    xlStatus = xlGetApplConfig("xlA429control", CHAN02, &hwType, &hwIndex, &hwChannel, busType); 
    if (xlStatus) return xlStatus;

    m_xlChannelMask[CHAN02] = xlGetChannelMask(hwType, hwIndex, hwChannel);
    sprintf_s (tmp, sizeof(tmp), "Found A429 Rx in VHWConf, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN02]);
    XLDEBUG(DEBUG_ADV,tmp);

    for (i=0; i < xlDrvConfig.channelCount; i++) {
      if ((xlDrvConfig.channel[i].channelMask == m_xlChannelMask[CHAN02]) &&
    		  (xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_A429) && 
          (xlDrvConfig.channel[i].busParams.data.a429.channelDirection == XL_A429_MSG_CHANNEL_DIR_RX)) {
        m_pHardware->AddString(xlDrvConfig.channel[i].name);
		    chan2Found = 1;
		    break;
	    }
    }

    if (!chan2Found) {
      sprintf_s (tmp, sizeof(tmp), "No assigned A429 Rx in VHWConf for channel 1, hWType: %d, CM: 0x%I64x", hwType, m_xlChannelMask[CHAN01]);
      XLDEBUG(DEBUG_ADV,tmp);
      return XL_ERROR;
    }

  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! a429Init

//! xlA429control use ONE port for both channels.
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CA429Functions::a429Init()
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

  xlStatus = xlOpenPort(&m_xlPortHandle, "xlA429control", m_xlChannelMask_both, &xlPermissionMask, 256, XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_A429); 
  sprintf_s(tmp, sizeof(tmp), "xlOpenPort: PortHandle: %d; Permissionmask: 0x%I64x; Status: %d", m_xlPortHandle, xlPermissionMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if (m_xlPortHandle == XL_INVALID_PORTHANDLE) return XL_ERROR;
  if (xlStatus == XL_ERR_INVALID_ACCESS) return xlStatus;

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! a429CreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CA429Functions::a429CreateRxThread()
{
  
  XLstatus      xlStatus = XL_ERROR;
  DWORD         ThreadId=0;
  char          tmp[100];
  
  if (m_xlPortHandle != XL_INVALID_PORTHANDLE) {

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

//! A429GoOnBus

//! set the selected baudrate and go on bus.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CA429Functions::A429GoOnBus(unsigned int txParity, unsigned int txMinGap, unsigned int txBitrate,
                                     unsigned int rxParity, unsigned int rxMinGap, unsigned int rxAutoBaudrate, unsigned int rxBitrate)
{
  XLstatus       xlStatus = XL_ERROR;
  char           tmp[100];
  XL_A429_PARAMS xlA429Params;

  xlA429Params.channelDirection = XL_A429_MSG_CHANNEL_DIR_TX;
  xlA429Params.data.tx.bitrate  = txBitrate;
  xlA429Params.data.tx.minGap   = txMinGap;
  xlA429Params.data.tx.parity   = txParity;

  xlStatus = xlA429SetChannelParams(m_xlPortHandle, m_xlChannelMask[CHAN01], &xlA429Params);
  printf("- xlA429SetChannelParams : PH=0x%02X, CM=0x%I64x, %s\n", 
    m_xlPortHandle, m_xlChannelMask[CHAN01], xlGetErrorString(xlStatus));

  xlA429Params.channelDirection     = XL_A429_MSG_CHANNEL_DIR_RX;
  xlA429Params.data.rx.autoBaudrate = rxAutoBaudrate;
  if (rxAutoBaudrate == XL_A429_MSG_AUTO_BAUDRATE_ENABLED) {
    xlA429Params.data.rx.bitrate    = 0;
  }
  else {
    xlA429Params.data.rx.bitrate    = rxBitrate;
  }
  xlA429Params.data.rx.maxBitrate   = XL_A429_MSG_BITRATE_RX_MAX; // 120000 bit/s
  xlA429Params.data.rx.minBitrate   = XL_A429_MSG_BITRATE_RX_MIN; // 10000 bit/s
  xlA429Params.data.rx.minGap       = rxMinGap;
  xlA429Params.data.rx.parity       = rxParity;

  xlStatus = xlA429SetChannelParams(m_xlPortHandle, m_xlChannelMask[CHAN02], &xlA429Params);
  printf("- xlA429SetChannelParams : PH=0x%02X, CM=0x%I64x, %s\n", 
    m_xlPortHandle, m_xlChannelMask[CHAN02], xlGetErrorString(xlStatus));

  xlStatus = xlActivateChannel(m_xlPortHandle, m_xlChannelMask_both, XL_BUS_TYPE_A429, XL_ACTIVATE_RESET_CLOCK);
  sprintf_s(tmp, sizeof(tmp), "ActivateChannel, stat: %d",  xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! A429GoOffBus

//! Deactivate the channel
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CA429Functions::A429GoOffBus()
{
  XLstatus      xlStatus = XL_ERROR;
  char          tmp[100];

  xlStatus = xlDeactivateChannel(m_xlPortHandle, m_xlChannelMask_both);
  sprintf_s(tmp, sizeof(tmp), "DeactivateChannel, stat: %d",  xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! A429Send

//! transmit a A429 message to the selected channel with the give values.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CA429Functions::A429Send(XL_A429_MSG_TX xlA429MsgTx, int channel)
{
  
  XLstatus      xlStatus;
  char          tmp[100];
  unsigned int  messageCount = 1;
  unsigned int  cntSent;

  xlStatus = xlA429Transmit(m_xlPortHandle, m_xlChannelMask[channel], messageCount, &cntSent, &xlA429MsgTx);
  sprintf_s(tmp, sizeof(tmp), "Transmit, mc: %d, cntSent:%d, channel: %d, stat: %d",  messageCount, cntSent, channel, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

///////////////////////////////////////////////////////////////////////////

//! ShowLicenses

//! Reads licenses from the selected channels and displays it.
//!
////////////////////////////////////////////////////////////////////////////
XLstatus CA429Functions::ShowLicenses()
{
  XLstatus xlStatus;
  char licAvail[2048];
  char strtmp[512];

  // Show available licenses
  XLlicenseInfo licenseArray[1024];
  unsigned int licArraySize = 1024;
  xlStatus = xlGetLicenseInfo(m_xlChannelMask[CHAN01] | m_xlChannelMask[CHAN02], licenseArray, licArraySize);
  if (xlStatus == XL_SUCCESS) {
    strcpy_s(licAvail, sizeof(licAvail), "Licenses found:\n\n");
    for (unsigned int i = 0; i < licArraySize; i++) {
      if (licenseArray[i].bAvailable) {
        sprintf_s(strtmp, sizeof(strtmp), "ID 0x%03x: %s\n", i, licenseArray[i].licName);
        if ((strlen(licAvail) + strlen(strtmp)) < sizeof(licAvail)) {
          strcat_s(licAvail, sizeof(licAvail), strtmp);
        }
        else {
          // Too less memory for printing licenses
          sprintf_s(licAvail, sizeof(licAvail), "Internal Error: String size in CA429Functions::ShowLicenses() is too small!");
          xlStatus = XL_ERROR;
        }
      }
    }
  }
  else {
    sprintf_s(licAvail, sizeof(licAvail), "Error %d when calling xlGetLicenseInfo()!", xlStatus);
  }
  
  AfxMessageBox(licAvail);
  return xlStatus;
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
  XLa429RxEvent   xlA429Event; 
  char            tmp[1000];
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
      xlStatus = xlA429Receive(pTh->xlPortHandle, &xlA429Event);	

      if (xlStatus != XL_ERR_QUEUE_IS_EMPTY) {

        switch(xlA429Event.tag) {
        case XL_A429_EV_TAG_TX_OK:
          sprintf_s(tmp, sizeof(tmp), "TX_OK: ch:%d, label:%d, data:%d, bitrate:%dbit/s, frameLength:%dns, msgCtrl:%d", 
            xlA429Event.channelIndex,
            xlA429Event.tagData.a429TxOkMsg.label,
            xlA429Event.tagData.a429TxOkMsg.data,
            xlA429Event.tagData.a429TxOkMsg.bitrate,
            xlA429Event.tagData.a429TxOkMsg.frameLength,
            xlA429Event.tagData.a429TxOkMsg.msgCtrl);
          break;
        case XL_A429_EV_TAG_TX_ERR:
          sprintf_s(tmp, sizeof(tmp), "TX_ERR: ch:%d, label:%d, data:%d, bitrate:%dbit/s, frameLength:%dns, , errRea:%d, errPos:%d", 
            xlA429Event.channelIndex,
            xlA429Event.tagData.a429TxErrMsg.label,
            xlA429Event.tagData.a429TxErrMsg.data,
            xlA429Event.tagData.a429TxErrMsg.bitrate,
            xlA429Event.tagData.a429TxErrMsg.frameLength,
            xlA429Event.tagData.a429TxErrMsg.errorReason,
            xlA429Event.tagData.a429TxErrMsg.errorPosition);
          break;
        case XL_A429_EV_TAG_RX_OK:
         sprintf_s(tmp, sizeof(tmp), "RX_OK: ch:%d, label:%d, data:%d, bitrate:%dbit/s, frameLength:%dns", 
            xlA429Event.channelIndex,
            xlA429Event.tagData.a429RxOkMsg.label,
            xlA429Event.tagData.a429RxOkMsg.data,
            xlA429Event.tagData.a429RxOkMsg.bitrate,
            xlA429Event.tagData.a429RxOkMsg.frameLength);
          break;
        case XL_A429_EV_TAG_RX_ERR:
          sprintf_s(tmp, sizeof(tmp), "RX_ERR: ch:%d, label:%d, data:%d, bitrate:%dbit/s, frameLength:%dns, errRea:%d, errPos:%d", 
            xlA429Event.channelIndex,
            xlA429Event.tagData.a429RxErrMsg.label,
            xlA429Event.tagData.a429RxErrMsg.data,
            xlA429Event.tagData.a429RxErrMsg.bitrate,
            xlA429Event.tagData.a429RxErrMsg.frameLength,
            xlA429Event.tagData.a429RxErrMsg.errorReason,
            xlA429Event.tagData.a429RxErrMsg.errorPosition);
          break;
        case XL_A429_EV_TAG_BUS_STATISTIC:
          break;
        default:
          break;
        }
        
        if (xlA429Event.tag != XL_A429_EV_TAG_BUS_STATISTIC) {
          XLDEBUG(DEBUG_ADV, tmp);
          pTh->pOutput->InsertString(-1, tmp);
          pTh->pOutput->SetCurSel(pTh->pOutput->GetCount()-1);
        }
      }

    }
          
  }
  return NO_ERROR; 
}
