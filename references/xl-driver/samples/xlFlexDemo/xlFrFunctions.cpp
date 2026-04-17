/*-------------------------------------------------------------------------------------------
| File        : xlFrFunctions.cpp
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2013 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include ".\xlFrFunctions.h"
#include ".\xlFrParseEvent.h"
#include "vxlapi.h"
#include "debug.h"

#define ERROR_TIME         100

////////////////////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////////////////////

BOOL    g_bThreadRun;
TStruct g_th[XL_CONFIG_MAX_CHANNELS];

////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////////////

CFrFunctions::CFrFunctions(void)
{
  m_nChannelCount = 0;
  m_lic = TRUE;
}

CFrFunctions::~CFrFunctions(void)
{
}

////////////////////////////////////////////////////////////////////////////

//! FrInit

//! Open the driver, get the channelmasks and create the RX thread.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CFrFunctions::FrInit()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  xlStatus = xlOpenDriver();
  sprintf(tmp, "xlOpenDriver, stat: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus != XL_SUCCESS) {
    AfxMessageBox("Opening vxlapi.dll failed!");
    return xlStatus;
  }

  xlStatus = frGetChannelMask();
  
  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! FrToggleTrace

//! toggles the FlexRay view
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

void CFrFunctions::FrToggleTrace(unsigned int nChan, BOOL bShowTrace)
{
  char            tmp[100];

  sprintf(tmp, "FrToggleTrace, showTrace: %d\n", bShowTrace);
  XLDEBUG(DEBUG_ADV, tmp);

  g_th[nChan].bShowTrace = bShowTrace;
}

////////////////////////////////////////////////////////////////////////////

//! FrAddTxFrame

//! adds a FlexRay frame to the node. Uses the following IN parameters:
//! 
//!  nChan        : our XL API channel 
//!  flagsChip    : the FR channel like XL_FR_CHANNEL_A
//!  txFlags      : e.g. XL_FR_FRAMEFLAG_REQ_TXACK for Tx ack events
//!  txMode       : cyclic, single shot or removing the frame. 
//!  payloadInc   : only advanced library.
//!  slotId       : slot Id (0..2047)
//!  offset       : 0..63
//!  repetition   : 1,2,4,8,16,32,64
//!  payloadLength: up to 127 words (254 bytes)
//!  *data        : payload data
////////////////////////////////////////////////////////////////////////////

XLstatus CFrFunctions::FrAddTxFrame(unsigned int    nChan,
                                    unsigned short  flagsChip,
                                    unsigned short  txFlags,
                                    unsigned char   txMode,
                                    unsigned char   payloadInc,
                                    unsigned short  slotId,
                                    unsigned char   offset,
                                    unsigned char   repetition,
                                    unsigned char   payloadLength,
                                    unsigned char   *data)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  XLfrEvent       xlFrEvent;

  // -------------------------------------------
  // fill up the FlexRay TX structure
  // -------------------------------------------

  xlFrEvent.tag                               = XL_FR_TX_FRAME;
  xlFrEvent.flagsChip                         = flagsChip; 
  xlFrEvent.size                              = 0;    // calculated inside XL-API DLL
  xlFrEvent.userHandle                        = 0;
  xlFrEvent.tagData.frTxFrame.flags           = txFlags;
  xlFrEvent.tagData.frTxFrame.offset          = offset;
  xlFrEvent.tagData.frTxFrame.repetition      = repetition;
  xlFrEvent.tagData.frTxFrame.payloadLength   = payloadLength;  
  xlFrEvent.tagData.frTxFrame.slotID          = slotId;
  xlFrEvent.tagData.frTxFrame.txMode          = txMode;
  xlFrEvent.tagData.frTxFrame.incrementOffset = 0;
  xlFrEvent.tagData.frTxFrame.incrementSize   = payloadInc; 

  memcpy(xlFrEvent.tagData.frTxFrame.data, data, payloadLength * 2);

  // -------------------------------------------
  // add the TX frame
  // -------------------------------------------
    
  xlStatus = xlFrTransmit(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask,
                          &xlFrEvent);    
  sprintf(tmp, "xlFrTransmit, sl: %d, off: %d, rep: %d, stat: %d\n", 
                xlFrEvent.tagData.frTxFrame.slotID,
                xlFrEvent.tagData.frTxFrame.offset,
                xlFrEvent.tagData.frTxFrame.repetition,
                xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! FrActivate

//! activate the device and open the eventsources
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CFrFunctions::FrActivate(unsigned int nChan, 
                                  unsigned short erayId, 
                                  unsigned short coldId,
                                  unsigned char flags)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
 
  // ----------------------------------
  // Open the ports
  // ----------------------------------
  xlStatus = frInit(nChan);
  // if we don't have INIT access we can't configure the node!
  if (xlStatus != XL_ERR_INVALID_ACCESS) {
    // ----------------------------------
    // Download the FlexRay configuration
    // ----------------------------------
    xlStatus = frSetConfig(nChan);
    if (xlStatus) return xlStatus;

    if (!(flags & USE_SPY_MODE)) {
      // ----------------------------------
      // Setup the startup and sync frames
      // not needed in spy mode
      // ----------------------------------
      xlStatus = frStartUpSync(nChan, erayId, coldId, flags);
      if (xlStatus != XL_SUCCESS) {
        if (xlStatus == XL_ERR_NO_LICENSE) {
          MessageBox(NULL,"No license for Coldstart CC.\n Only E-Ray functionality!","xlFlexDemo",MB_OK|MB_ICONEXCLAMATION);
          sprintf(tmp, "FrActivate, NO LICENSE - only E-RAY functionality!\n");
          XLDEBUG(DEBUG_ADV, tmp);
          m_lic = FALSE;
        }
        else { return xlStatus; }
      }
    }
  }

  // ----------------------------------
  // if needed activate the SPY mode
  // ----------------------------------
  if (flags & USE_SPY_MODE) {
    xlStatus = xlFrActivateSpy(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, 
                             XL_FR_SPY_MODE_ASYNCHRONOUS);
    sprintf(tmp, "frActivateSpy, PH: %d, stat: %d\n",  m_NodeParam[nChan].xlPortHandle, xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
    if (xlStatus != XL_SUCCESS) {
      AfxMessageBox(tmp);
      return xlStatus;
    }
  }

  // ----------------------------------
  // Create for every device one thread
  // ----------------------------------
  xlStatus = frCreateRxThread(nChan);
  if (xlStatus) return xlStatus;

  // ----------------------------------
  // Activate the device (channel)
  // ----------------------------------
  if (m_NodeParam[nChan].xlPortHandle == XL_INVALID_PORTHANDLE) return xlStatus;

  xlStatus = xlActivateChannel(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, 
                               XL_BUS_TYPE_FLEXRAY, XL_ACTIVATE_RESET_CLOCK);
  sprintf(tmp, "xlActivateChannel, PH: %d, stat: %d\n",  m_NodeParam[nChan].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus != XL_SUCCESS) {
    AfxMessageBox(tmp);
    return xlStatus;
  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! FrDeActivate

//! deactivate the device
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CFrFunctions::FrDeactivate(unsigned int nChan)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  if (m_NodeParam[nChan].xlPortHandle == XL_INVALID_PORTHANDLE) return xlStatus;

  xlStatus = xlDeactivateChannel(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask);
  sprintf(tmp, "xlDeactivateChannel, PH: %d, stat: %d\n", m_NodeParam[nChan].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if (g_bThreadRun == TRUE) g_bThreadRun = FALSE;
  
  // Wait until the thread is gone
  WaitForSingleObject(m_NodeParam[nChan].hThread, 10);
  xlStatus = xlClosePort(m_NodeParam[nChan].xlPortHandle);
  sprintf(tmp, "xlClosePort, PH: %d, stat: %d\n", m_NodeParam[nChan].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  CloseHandle(m_NodeParam[nChan].hThread);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! frGetChannelMask

//! parse the registry to get the channelmask
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CFrFunctions::frGetChannelMask()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  unsigned int    nChan;
  unsigned int    nChannelArray = 0;

  XLdriverConfig  xlDrvConfig;

  //check for hardware:
  xlStatus = xlGetDriverConfig(&xlDrvConfig);
  if (xlStatus) return xlStatus;

  // at first we check how much FlexRay devices we have
  for (nChan = 0; nChan < xlDrvConfig.channelCount; nChan++) {
    if ( xlDrvConfig.channel[nChan].channelBusCapabilities & XL_BUS_COMPATIBLE_FLEXRAY) {
    
      m_NodeParam[nChannelArray].xlChannelMask = xlDrvConfig.channel[nChan].channelMask;
      sprintf(tmp, "%s, (%06d)",xlDrvConfig.channel[nChan].name, xlDrvConfig.channel[nChan].serialNumber);
      XLDEBUG(DEBUG_ADV,tmp);
      m_plDevice->AddString(tmp);
            
      nChannelArray++;
      m_nChannelCount++;
    }
  }
  
  sprintf (tmp, "Found %d FlexRay channels\n", m_nChannelCount);
  XLDEBUG(DEBUG_ADV,tmp);
  if (m_nChannelCount == 0) {
    m_plDevice->AddString("NO FlexRay device found!");
    return XL_ERROR;
  }
  
  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! frInit

//! opens the found devices
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CFrFunctions::frInit(unsigned int nChan)
{
  XLstatus         xlStatus = XL_ERROR;
  XLaccess         xlPermissionMask;
  char             tmp[200];
  unsigned int     nbrOfBoxes;
  unsigned int     boxMask;
  unsigned int     boxSerial;
  unsigned int     i;
  XLuint64         licInfo[4], tmpLicInfo[4];
  
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


  xlPermissionMask = m_NodeParam[nChan].xlChannelMask;

  xlStatus = xlOpenPort(&m_NodeParam[nChan].xlPortHandle, "xlFlexDemo", 
                         m_NodeParam[nChan].xlChannelMask, &xlPermissionMask, RX_FIFO_SIZE, 
                         XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_FLEXRAY);
  sprintf(tmp, "xlOpenPort, PH: %d; CM: 0x%I64x, Permissionmask: 0x%I64x; Status: %d\n", 
                m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask,
                xlPermissionMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if (m_NodeParam[nChan].xlPortHandle == XL_INVALID_PORTHANDLE) return XL_ERROR;

  if (!(m_NodeParam[nChan].xlChannelMask && xlPermissionMask)) {
    XLDEBUG(DEBUG_ADV, "xlOpenPort, no init access!\n");
    return XL_ERR_INVALID_ACCESS;
  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! frSetConfig

//! setup a standard FlexRay configuration
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CFrFunctions::frSetConfig(unsigned int nChan)
{
  XLstatus          xlStatus = XL_ERROR;
  XLfrClusterConfig xlConf;
  char              tmp[100];
 
  memset(&xlConf, 0, sizeof(xlConf));

  xlConf.busGuardianEnable                  = 0;
  xlConf.baudrate                           = 10000;
  xlConf.busGuardianTick                    = 0;
  xlConf.externalClockCorrectionMode        = 0;
  xlConf.gColdStartAttempts                 = 0x00000014;
  xlConf.gListenNoise                       = 11;
  xlConf.gMacroPerCycle                     = 0x00001388;
  xlConf.gMaxWithoutClockCorrectionFatal    = 0x00000001;
  xlConf.gMaxWithoutClockCorrectionPassive  = 0x00000001;
  xlConf.gNetworkManagementVectorLength     = 0;
  xlConf.gNumberOfMinislots                 = 0x000001E5;
  xlConf.gNumberOfStaticSlots               = 60;
  xlConf.gOffsetCorrectionStart             = 0x00001343;
  xlConf.gPayloadLengthStatic               = STATIC_PAYLOAD_LENGTH;
  xlConf.gSyncNodeMax                       = 4;
  xlConf.gdActionPointOffset                = 5;
  xlConf.gdDynamicSlotIdlePhase             = 1;
  xlConf.gdMacrotick                        = 0;
  xlConf.gdMinislot                         = 7;
  xlConf.gdMiniSlotActionPointOffset        = 3;
  xlConf.gdNIT                              = 103;
  xlConf.gdStaticSlot                       = 0x00000019;
  xlConf.gdSymbolWindow                     = 0;
  xlConf.gdTSSTransmitter                   = 5;
  xlConf.gdWakeupSymbolRxIdle               = 0;
  xlConf.gdWakeupSymbolRxLow                = 0;
  xlConf.gdWakeupSymbolRxWindow             = 0;
  xlConf.gdWakeupSymbolTxIdle               = 0;
  xlConf.gdWakeupSymbolTxLow                = 0;
  xlConf.pAllowHaltDueToClock               = 0;
  xlConf.pAllowPassiveToActive              = 7;
  xlConf.pChannels                          = 3;
  xlConf.pClusterDriftDamping               = 1;
  xlConf.pDecodingCorrection                = 0x24;
  xlConf.pDelayCompensationA                = 10;
  xlConf.pDelayCompensationB                = 10;
  xlConf.pExternOffsetCorrection            = 1;
  xlConf.pExternRateCorrection              = 2;
  xlConf.pKeySlotUsedForStartup             = 1; // must be set for xlFrStartUpAndSync
  xlConf.pKeySlotUsedForSync                = 1; // must be set for xlFrStartUpAndSync
  xlConf.pLatestTx                          = 0x000001DF;
  xlConf.pMacroInitialOffsetA               = 7;
  xlConf.pMacroInitialOffsetB               = 7;
  xlConf.pMaxPayloadLengthDynamic           = 2;
  xlConf.pMicroInitialOffsetA               = 0x22;
  xlConf.pMicroInitialOffsetB               = 0x22;
  xlConf.pMicroPerCycle                     = 0x00030D40;
  xlConf.pMicroPerMacroNom                  = 0;
  xlConf.pOffsetCorrectionOut               = 0x00000057;
  xlConf.pRateCorrectionOut                 = 0x00000259;
  xlConf.pSamplesPerMicrotick               = 0;
  xlConf.pSingleSlotEnabled                 = 0;
  xlConf.pWakeupChannel                     = 0;
  xlConf.pWakeupPattern                     = 0;
  xlConf.pdAcceptedStartupRange             = 0xCD;
  xlConf.pdListenTimeout                    = 0x61CD8;
  xlConf.pdMaxDrift                         = 0x0000012C;
  xlConf.pdMicrotick                        = 0;
  xlConf.gdCASRxLowMax                      = 7;
  xlConf.gChannels                          = 0;
  xlConf.vExternOffsetControl               = 0;
  xlConf.vExternRateControl                 = 0;
  xlConf.pChannelsMTS                       = 0;

  // write the configuration to the device
  xlStatus = xlFrSetConfiguration(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, &xlConf);
  sprintf(tmp, "xlFrSetConfiguration, xlStatus: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! frStartUpSync

//! defines the startup and sync frames
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CFrFunctions::frStartUpSync(unsigned int nChan, 
                                     unsigned short erayId, 
                                     unsigned short coldId,
                                     unsigned char flags)
{
  XLstatus        xlStatus = XL_SUCCESS;
  XLfrEvent       xlFrConfigFrame;
  XLfrMode	      xlFrMode;
  char            tmp[100];
  char            payload[4]; // here we have bytes
  
  memset(&xlFrConfigFrame, 0, sizeof(xlFrConfigFrame));

  if (flags & USE_STARTUP_ERAY) {

    // -----------------------------------------------
    // setup the startup and sync frames for the E-Ray
    // -----------------------------------------------

    xlFrConfigFrame.tag                               = XL_FR_TX_FRAME;
    xlFrConfigFrame.flagsChip                         = XL_FR_CHANNEL_A;
    xlFrConfigFrame.size                              = 0;   // calculated inside XL-API DLL
    xlFrConfigFrame.userHandle                        = 0;
    xlFrConfigFrame.tagData.frTxFrame.flags           = XL_FR_FRAMEFLAG_STARTUP|XL_FR_FRAMEFLAG_SYNC|XL_FR_FRAMEFLAG_REQ_TXACK;
    xlFrConfigFrame.tagData.frTxFrame.offset          = 0;
    xlFrConfigFrame.tagData.frTxFrame.repetition      = 2;
    xlFrConfigFrame.tagData.frTxFrame.payloadLength   = sizeof(payload)/2; // -> FR calculates in WORDS
    xlFrConfigFrame.tagData.frTxFrame.slotID          = erayId;
    xlFrConfigFrame.tagData.frTxFrame.txMode          = XL_FR_TX_MODE_CYCLIC;
    xlFrConfigFrame.tagData.frTxFrame.incrementOffset = 0;
    xlFrConfigFrame.tagData.frTxFrame.incrementSize   = 0;

    memset(payload, 0x88, sizeof(payload));
    memcpy(xlFrConfigFrame.tagData.frTxFrame.data, payload, sizeof(payload));

    xlStatus = xlFrInitStartupAndSync(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, &xlFrConfigFrame);
    sprintf(tmp, "xlFrInitStartupAndSync, E_RAY xlStatus: %d\n", xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
    if (xlStatus) return xlStatus;

    // -------------------------------------------------
    // setup the mode for the E-Ray CC
    // -------------------------------------------------

    xlFrMode.frMode               = XL_FR_MODE_NORMAL;
    xlFrMode.frStartupAttributes  = XL_FR_MODE_COLDSTART_LEADING;

    xlStatus = xlFrSetMode(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, &xlFrMode);
    sprintf(tmp, "xlFrSetMode, (for XL_FR_MODE_NORMAL), stat: %d\n", xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
    if (xlStatus) return xlStatus;
  }
 
  if (flags & USE_STARTUP_COLD) {
    // -------------------------------------------------
    // setup the startup and sync frames for the COLD CC
    // NEEDS advanced LICENSE!
    // -------------------------------------------------

    xlFrConfigFrame.tag                               = XL_FR_TX_FRAME;
    xlFrConfigFrame.flagsChip                         = XL_FR_CC_COLD_A;
    xlFrConfigFrame.size                              = 0;     // calculated inside XL-API DLL
    xlFrConfigFrame.userHandle                        = 0;
    xlFrConfigFrame.tagData.frTxFrame.flags           = XL_FR_FRAMEFLAG_STARTUP|XL_FR_FRAMEFLAG_SYNC|XL_FR_FRAMEFLAG_REQ_TXACK;
    xlFrConfigFrame.tagData.frTxFrame.offset          = 0;
    xlFrConfigFrame.tagData.frTxFrame.repetition      = 2;
    xlFrConfigFrame.tagData.frTxFrame.payloadLength   = sizeof(payload)/2; // -> FR calculates in WORDS
    xlFrConfigFrame.tagData.frTxFrame.slotID          = coldId;
    xlFrConfigFrame.tagData.frTxFrame.txMode          = XL_FR_TX_MODE_CYCLIC;
    xlFrConfigFrame.tagData.frTxFrame.incrementOffset = 0;
    xlFrConfigFrame.tagData.frTxFrame.incrementSize   = 0;

    memset(payload, 0x88, sizeof(payload));
    memcpy(xlFrConfigFrame.tagData.frTxFrame.data, payload, sizeof(payload));

    xlStatus = xlFrInitStartupAndSync(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, &xlFrConfigFrame);
    sprintf(tmp, "xlFrInitStartupAndSync, COLD CC, xlStatus: %d\n", xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
    if (xlStatus) return xlStatus;

    // -------------------------------------------------
    // setup the mode for the COLD CC
    // -------------------------------------------------

    xlFrMode.frMode               = XL_FR_MODE_COLD_NORMAL;
    xlFrMode.frStartupAttributes  = XL_FR_MODE_COLDSTART_LEADING;

    xlStatus = xlFrSetMode(m_NodeParam[nChan].xlPortHandle, m_NodeParam[nChan].xlChannelMask, &xlFrMode);
    sprintf(tmp, "xlFrSetMode, (for XL_FR_MODE_COLD_NORMAL), stat: %d\n", xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! frCreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CFrFunctions::frCreateRxThread(unsigned int nChan)
{
  XLstatus       xlStatus;
  DWORD          ThreadId=0;
  char           tmp[100];
 
  if (m_NodeParam[nChan].xlPortHandle == XL_INVALID_PORTHANDLE) return XL_ERROR;

  // Send a event for each Msg!!!
  xlStatus = xlSetNotification (m_NodeParam[nChan].xlPortHandle, &m_NodeParam[nChan].hMsgEvent, 1);
  sprintf(tmp, "xlSetNotification, xlStatus: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // for the RxThread
  g_th[nChan].xlPortHandle = m_NodeParam[nChan].xlPortHandle;
  g_th[nChan].hMsgEvent    = m_NodeParam[nChan].hMsgEvent; 
  g_th[nChan].plMsg        = m_plMsg;
  g_th[nChan].bShowTrace   = TRUE;

  m_NodeParam[nChan].hThread = CreateThread(0, 0x1000, RxThread, (LPVOID)&g_th[nChan], 0, &ThreadId);

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
  XLfrEvent       xlFrEvent;
  //char            tmp[100];
  CString         str;
  
  TStruct *th;

  th = (TStruct*) par;  
  CFrParseEvent parse(th->plMsg);

  g_bThreadRun = TRUE;

  while (g_bThreadRun) { 
   
    WaitForSingleObject(th->hMsgEvent, 100);

    xlStatus = XL_SUCCESS;
  
    while (xlStatus == XL_SUCCESS) {

      xlStatus = xlFrReceive(th->xlPortHandle, &xlFrEvent);
      
      if ( (xlStatus != XL_ERR_QUEUE_IS_EMPTY) && (xlStatus != XL_ERROR) ) {
         
        //sprintf(tmp, "thread, tag: 0x%x\n", xlFrEvent.tag);
        //XLDEBUG(DEBUG_ADV, tmp);
        if (th->bShowTrace) {
          parse.parseEvent(&xlFrEvent);
        }
      }
    } 
  }
 
  return NO_ERROR; 
}
