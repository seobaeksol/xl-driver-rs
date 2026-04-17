/*-------------------------------------------------------------------------------------------
| File        : xlMOST150Functions.cpp
| Project     : Vector MOST150 Example 
|
| Description : Shows the basic MOST150 functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2011 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "xlMOST150Functions.h"
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

CMOST150Functions::CMOST150Functions(void)
{
  unsigned int nChan;

  // setup the default node parameters
  for (nChan = 0; nChan < XL_CONFIG_MAX_CHANNELS; nChan++) {
    m_NodeParam[nChan].xlPortHandle   = XL_INVALID_PORTHANDLE;
    m_NodeParam[nChan].xlChannelMask  = 0;
    m_NodeParam[nChan].nNodeAdr       = 0xFFFF;
    m_NodeParam[nChan].nGroupAdr      = 0x0300;
    m_NodeParam[nChan].nFrequency     = XL_MOST150_FREQUENCY_48000;
    m_NodeParam[nChan].nDeviceMode    = XL_MOST150_DEVICEMODE_SLAVE;
  }
}

CMOST150Functions::~CMOST150Functions(void)
{
}

////////////////////////////////////////////////////////////////////////////

//! MOST150Init

//! Open the driver, get the channelmasks
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::MOST150Init(void)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[300];

  unsigned int     nbrOfBoxes;
  unsigned int     boxMask;
  unsigned int     boxSerial;
  unsigned int     i;
  XLuint64         licInfo[4], tmpLicInfo[4];

  xlStatus = xlOpenDriver();
  sprintf(tmp, "xlOpenDriver, xlStatus=0x%x\n", xlStatus);
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
  xlStatus = most150GetChannelMask();
 
  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150Close

//! stops the threads, close the porthandles and afterall close the driver.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::MOST150Close(void)
{
  // ---------------------------------------
  // stops the RX thread and close the ports
  // ---------------------------------------
  most150ClosePort();

  // ---------------------------------------
  // unloads the DLL
  // ---------------------------------------
  xlCloseDriver();

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150Activate

//! opens the port, creates the RX thread, activate the device and 
//! open the eventsources
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::MOST150Activate(unsigned int nChan)
{
  XLstatus        xlStatus = XL_ERROR;
  
  m_channel = nChan;

  // ---------------------------------------
  // Open the ports
  // ---------------------------------------
  xlStatus = most150OpenPort();
  if (xlStatus != XL_SUCCESS) {
    char str[256];
    sprintf_s(str, sizeof(str), "Open port of MOST150 failed with errorcode=%d!", xlStatus);
    AfxMessageBox(str);
    return xlStatus;
  }

  // ---------------------------------------
  // Create for every device one thread
  // ---------------------------------------
  xlStatus = most150CreateRxThread();
  if (xlStatus != XL_SUCCESS) {
    char str[256];
    sprintf_s(str, sizeof(str), "Creating Rx-Thread failed with errorcode=%d!", xlStatus);
    AfxMessageBox(str);
    return xlStatus;
  }

  // ---------------------------------------
  // activate the channel
  // ---------------------------------------
  xlStatus = most150Startup();

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150DeActivate

//! deactivate the device
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::MOST150DeActivate(void)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  if (m_NodeParam[m_channel].xlPortHandle == XL_INVALID_PORTHANDLE) return xlStatus;

  xlStatus = xlDeactivateChannel(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask);
  sprintf(tmp, "xlDeactivateChannel, PH: %d, xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // close the ports, stop the RX thread
  // ---------------------------------------
  most150ClosePort();

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! most150BuildCtrlMsgHeader

//! builds header information for ctrl tx msg
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

void CMOST150Functions::most150BuildCtrlMsgHeader(unsigned char fblockId, 
                                                  unsigned char instId,
                                                  unsigned char fktID, 
                                                  unsigned char opType,
                                                  unsigned char telId, 
                                                  unsigned short telLen,
                                                  XLmost150CtrlTxMsg* pCtrlMsg) {
  if(!pCtrlMsg) {
    return;
  }

  pCtrlMsg->ctrlData[0] = fblockId;
  pCtrlMsg->ctrlData[1] = instId;
  pCtrlMsg->ctrlData[2] = (fktID >> 4);
  pCtrlMsg->ctrlData[3] = ((fktID & 0x0F) << 4) | (opType & 0x0F);
  pCtrlMsg->ctrlData[4] = ((telId & 0x0F) << 4) | ((telLen >> 8) & 0x0F);
  pCtrlMsg->ctrlData[5] = (telLen & 0xFF);
}

////////////////////////////////////////////////////////////////////////////

//! most150ClosePort

//! stops the threads, close the porthandles and afterall close the driver.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::most150ClosePort(void)
{
  XLstatus        xlStatus;
  char            tmp[100];

  if (g_bThreadRun == TRUE) g_bThreadRun = FALSE;

  // Wait until the threads is gone
  WaitForSingleObject(m_NodeParam[m_channel].hThread, 1000);
  xlStatus = xlClosePort(m_NodeParam[m_channel].xlPortHandle);
  sprintf(tmp, "xlClosePort, PH: %d, xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  CloseHandle(m_NodeParam[m_channel].hThread);

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! most150Startup

//! activate the device and open the eventsources
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::most150Startup(void)
{
  XLstatus     xlStatus = XL_ERROR;
  char         tmp[100];
  unsigned int sourceMask; 

  // we switch on all possible events. It's depending on the licence you have.
  sourceMask = (XL_MOST150_SOURCE_SPECIAL_NODE |
                XL_MOST150_SOURCE_SYNC_ALLOC_INFO |
                XL_MOST150_SOURCE_CTRL_SPY | 
                XL_MOST150_SOURCE_ASYNC_SPY |
                XL_MOST150_SOURCE_ETH_SPY |
                XL_MOST150_SOURCE_SHUTDOWN_FLAG |
                XL_MOST150_SOURCE_SYSTEMLOCK_FLAG |
                XL_MOST150_SOURCE_LIGHTLOCK_INIC |
                XL_MOST150_SOURCE_ECL_CHANGE |
                XL_MOST150_SOURCE_LIGHT_STRESS |
                XL_MOST150_SOURCE_LOCK_STRESS |
                XL_MOST150_SOURCE_BUSLOAD_CTRL |
                XL_MOST150_SOURCE_BUSLOAD_ASYNC |
                XL_MOST150_SOURCE_CTRL_MLB |
                XL_MOST150_SOURCE_ASYNC_MLB |
                XL_MOST150_SOURCE_ETH_MLB |
                XL_MOST150_SOURCE_TXACK_MLB |
                XL_MOST150_SOURCE_STREAM_UNDERFLOW |
                XL_MOST150_SOURCE_STREAM_OVERFLOW |
                XL_MOST150_SOURCE_STREAM_RX_DATA |
                XL_MOST150_SOURCE_ECL_SEQUENCE);

  xlStatus = xlActivateChannel(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, XL_BUS_TYPE_MOST, XL_ACTIVATE_RESET_CLOCK);
  sprintf(tmp, "xlActivateChannel, PH: %d, xlStatus=0x%x\n",  m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMost150SwitchEventSources(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, sourceMask);
  sprintf(tmp, "xlMostSwitchEventSources, PH: %d, sourceMask: 0x%x, xlStatus=0x%x\n",  
    m_NodeParam[m_channel].xlPortHandle, sourceMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMost150SyncGetAllocTable(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150SyncGetAllocTable, PH: %d, xlStatus=0x%x\n",  
    m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = MOST150SetupNode(m_NodeParam[m_channel].nDeviceMode, 
                              m_NodeParam[m_channel].nFrequency, 
                              m_NodeParam[m_channel].nNodeAdr, 
                              m_NodeParam[m_channel].nGroupAdr);

  // perform initial startup only after device mode event is received
  m_DoInitialStartup = true;

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! most150SetApplConfig

//! parse driver config structure
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::most150SetApplConfig(XLdriverConfig xlDrvConfig, unsigned int appChannel)
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

    sprintf (tmp, "CI: %d, CM: 0x%I64x, hwType: %d, bustype: %d, hwChannel: %d, cap: 0x%x\n", 
        xlDrvConfig.channel[nChan].channelIndex,
        xlDrvConfig.channel[nChan].channelMask,
        xlDrvConfig.channel[nChan].hwType, 
        xlDrvConfig.channel[nChan].connectedBusType,
        xlDrvConfig.channel[nChan].hwChannel,
        xlDrvConfig.channel[nChan].channelBusCapabilities);
      XLDEBUG(DEBUG_ADV,tmp);

      // we search the MOST150 channels 
      if ((xlDrvConfig.channel[nChan].channelBusCapabilities & XL_BUS_ACTIVE_CAP_MOST) && 
          (xlDrvConfig.channel[nChan].busParams.data.most.compatibleSpeedGrade == XL_BUS_PARAMS_MOST_SPEED_GRADE_150) &&
          (most150CheckChannelMask(xlDrvConfig.channel[nChan].channelMask))) {

        hwType    = xlDrvConfig.channel[nChan].hwType;
        hwIndex   = xlDrvConfig.channel[nChan].hwIndex;
        hwChannel = xlDrvConfig.channel[nChan].hwChannel;

        xlStatus = xlSetApplConfig( // Registration of Application with default settings
        "xlMOST150View",            // Application Name
        appChannel,                 // Application channel 0 or 1
        hwType,                     // hwType  (CANcardXL...)    
        hwIndex,                    // Index of hardware (slot) (0,1,...)
        hwChannel,                  // Index of channel (connector) (0,1,...)
        busType);                   // the application is for MOST.

        m_NodeParam[appChannel].xlChannelMask = xlGetChannelMask(hwType, hwIndex, hwChannel);
        sprintf (tmp, "Register MOST150 hWType: %d, hwIndex: %d,  CM: 0x%I64x\n", 
          hwType, hwChannel, m_NodeParam[appChannel].xlChannelMask);
        XLDEBUG(DEBUG_ADV,tmp);

        sprintf(tmp, "%s, (%06d)", xlDrvConfig.channel[nChan].name, xlDrvConfig.channel[nChan].serialNumber);
        m_plDevice->AddString(tmp);
     
        break;
      }
  }

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! most150CheckChannelMask

//! checks if the channel is allready assigned.
//!
//////////////////////////////////////////////////////////////////////////// 

BOOL CMOST150Functions::most150CheckChannelMask(XLaccess xlChannelMask)
{
  BOOL bStatus  = TRUE;
  
  for (int i=0; i<XL_CONFIG_MAX_CHANNELS; i++) {
    if (m_NodeParam[i].xlChannelMask == xlChannelMask) {
      XLDEBUG(DEBUG_ADV,"Channel allready assigned\n");
      bStatus = FALSE;
    }
  }

  return bStatus;
}

////////////////////////////////////////////////////////////////////////////

//! most150GetChannelMask

//! parse the registry to get the channelmask
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::most150GetChannelMask(void)
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

  // at first we check how much MOST150 devices we have
  for (nChan = 0; nChan < xlDrvConfig.channelCount; nChan++) {
    if ((xlDrvConfig.channel[nChan].channelBusCapabilities & XL_BUS_COMPATIBLE_MOST) &&
        (xlDrvConfig.channel[nChan].busParams.data.most.compatibleSpeedGrade == XL_BUS_PARAMS_MOST_SPEED_GRADE_150)) {
      m_nChannelCount++;
    }
  }
  sprintf (tmp, "Found %d MOST150 channels\n", m_nChannelCount);
  XLDEBUG(DEBUG_ADV,tmp);
  if (m_nChannelCount == 0) {
    m_plDevice->AddString("NO MOST150 device found!");
    return XL_ERROR;
  }

  // get the channelmasks
  nChan = 0;
  while (nChan < m_nChannelCount) {

    // we check only if there is an application registered or not.
    xlStatus = xlGetApplConfig("xlMOST150View", appChannel, &hwType, &hwIndex, &hwChannel, busType); 
 
    // Set the params into registry (default values...!)
    if (xlStatus) {
      if (!most150SetApplConfig(xlDrvConfig, appChannel)) {
        appChannel++;
        nChan++;
        nChannelArray++;
      }
    }
    else {
      m_NodeParam[nChannelArray].xlChannelMask = xlGetChannelMask(hwType, hwIndex, hwChannel);

      // maybe we have more applications assigned... but less in the system
      if (m_NodeParam[nChannelArray].xlChannelMask) {
        sprintf (tmp, "Found MOST150 in VHWConf, hwType: %d, hwChannel: %d, CM: 0x%I64x, channel: %d\n", 
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

//! most150OpenPort

//! opens the found devices
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::most150OpenPort(void)
{
  XLstatus         xlStatus = XL_ERROR;
  XLaccess         xlPermissionMask;
  char             tmp[100];
 
  xlPermissionMask = m_NodeParam[m_channel].xlChannelMask;

  xlStatus = xlOpenPort(&m_NodeParam[m_channel].xlPortHandle, "xlMOST150View", m_NodeParam[m_channel].xlChannelMask, &xlPermissionMask, 0x10000, XL_INTERFACE_VERSION_V4, XL_BUS_TYPE_MOST); // receive buffer size is 65536
  sprintf(tmp, "xlOpenPort: PortHandle: %d; Permissionmask: 0x%I64x; xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlPermissionMask, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! most150CreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::most150CreateRxThread(void)
{
  XLstatus       xlStatus;
  DWORD          ThreadId=0;
  char           tmp[100];

  static TStruct th[XL_CONFIG_MAX_CHANNELS];
     
  if (m_NodeParam[m_channel].xlPortHandle == XL_INVALID_PORTHANDLE) return XL_ERROR;

  // Send a event for each Msg!!!
  xlStatus = xlSetNotification (m_NodeParam[m_channel].xlPortHandle, &m_NodeParam[m_channel].hMsgEvent, 1);
  sprintf(tmp, "SetNotification, xlStatus=0x%x\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // for the RxThread
  th[m_channel].xlPortHandle = m_NodeParam[m_channel].xlPortHandle;
  th[m_channel].hMsgEvent    = m_NodeParam[m_channel].hMsgEvent; 
  th[m_channel].pOutput      = m_pListBox;
  th[m_channel].pMost150Func = this;
  
  m_NodeParam[m_channel].hThread = CreateThread(0, 0x1000, RxThread, (LPVOID)&th[m_channel], 0, &ThreadId);

  m_plRxBox = m_pListBox;

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150NwStartup

//! Perform a startup
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::MOST150NwStartup(void)
{
  XLstatus xlStatus;
  char     tmp[100];
  
  xlStatus = xlMost150Startup(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150Startup PH: %d, xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! MOST150NwShutdown

//! Perform a shutdown
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::MOST150NwShutdown(void)
{
  XLstatus xlStatus;
  char     tmp[100];
  
  xlStatus = xlMost150Shutdown(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150Shutdown PH: %d, xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! MOST150SetupNode

//! setup a node depending on the dialog box parameter
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::MOST150SetupNode(unsigned int nDeviceMode, 
                                             unsigned int nFrequency, 
                                             unsigned short nNodeAdr, 
                                             unsigned short nGroupAdr)
{
  XLstatus      xlStatus = XL_ERROR;
  char          tmp[100];
  XLmost150SetSpecialNodeInfo specialNodeInfo;

  // save the node parameters.
  m_NodeParam[m_channel].nDeviceMode = nDeviceMode;
  m_NodeParam[m_channel].nFrequency  = nFrequency;
  m_NodeParam[m_channel].nNodeAdr    = nNodeAdr;
  m_NodeParam[m_channel].nGroupAdr   = nGroupAdr;

  xlStatus = xlMost150SetDeviceMode(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, nDeviceMode);
  sprintf(tmp, "xlMost150SetDeviceMode, DeviceMode: %d, xlStatus=0x%x\n", nDeviceMode, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  specialNodeInfo.changeMask   = XL_MOST150_NA_CHANGED;
  specialNodeInfo.nodeAddress  = nNodeAdr;
  xlStatus = xlMost150SetSpecialNodeInfo(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, &specialNodeInfo);
  sprintf(tmp, "xlMost150SetSpecialNodeInfo, nNodeAdr: %d, xlStatus=0x%x\n", nNodeAdr, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMost150SetFrequency(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, nFrequency);
  sprintf(tmp, "xlMost150SetFrequency, nFrequency: %d, xlStatus=0x%x\n", nFrequency, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! MOST150GetInfo

//! request the node info's.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::MOST150GetInfo(void)
{
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  xlStatus = xlMost150GetDeviceMode(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150GetDeviceMode, xlStatus=0x%x\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMost150GetSPDIFMode(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150GetSPDIFMode, xlStatus=0x%x\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMost150GetSpecialNodeInfo(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, XL_MOST150_SPECIAL_NODE_MASK_CHANGED);
  sprintf(tmp, "xlMost150GetSpecialNodeInfo, xlStatus=0x%x\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMost150SyncGetAllocTable(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150SyncGetAllocTable, PH: %d, xlStatus=0x%x\n",  
    m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150CtrlTransmit

//! transmit a control frame
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::MOST150CtrlTransmit(unsigned short nTargetAdr)
{
  XLstatus           xlStatus = XL_ERROR;
  XLmost150CtrlTxMsg ctrlTxMsg;  
  char               tmp[100];
  unsigned char      fblockId = 0x01;  // NetBlock
  unsigned char      instId   = 0;     // InstID: don't care
  unsigned char      fktID    = 0x003; // NodeAddress
  unsigned char      opType   = 0x01;  // Get
  unsigned char      telId    = 0;
  unsigned short     telLen   = 0;

  ctrlTxMsg.ctrlPrio = 1;
  ctrlTxMsg.ctrlSendAttempts = 1;
  ctrlTxMsg.targetAddress = nTargetAdr; 
  for (unsigned char i = 0; i < telLen; i++) {
    ctrlTxMsg.ctrlData[i] = i;
  }
  most150BuildCtrlMsgHeader(fblockId, instId, fktID, opType, telId, telLen, &ctrlTxMsg);

  xlStatus  = xlMost150CtrlTransmit(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, &ctrlTxMsg);
  sprintf(tmp, "xlMost150CtrlTransmit, tgtAddr: 0x%x, xlStatus=0x%x\n", ctrlTxMsg.targetAddress, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150AsyncTransmit

//! transmit a async frame
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CMOST150Functions::MOST150AsyncTransmit(unsigned short nTargetAdr)
{
  XLstatus            xlStatus = XL_ERROR;
  XLmost150AsyncTxMsg asyncTxMsg;  
  char                tmp[100];

  asyncTxMsg.priority = 1;
  asyncTxMsg.asyncSendAttempts = 1;

  asyncTxMsg.targetAddress = nTargetAdr;
  asyncTxMsg.length        = 20;
  for (unsigned char i = 0; i < asyncTxMsg.length; i++) {
    asyncTxMsg.asyncData[i] = i;
  }

  xlStatus  = xlMost150AsyncTransmit(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, &asyncTxMsg);
  sprintf(tmp, "xlMost150AsyncTransmit, tgtAddr: 0x%x, xlStatus=0x%x\n", asyncTxMsg.targetAddress, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150TwinklePowerLED

//! The power LED of the selected device will twinkle
//!
////////////////////////////////////////////////////////////////////////////
  
XLstatus CMOST150Functions::MOST150TwinklePowerLED(void)
{
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  xlStatus = xlMost150TwinklePowerLed(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
  sprintf(tmp, "xlMost150TwinklePowerLed, xlStatus=0x%x\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150GenerateLightError

//! generate 'nRepeat' light errors.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::MOST150GenerateLightError(unsigned short nRepeat)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  xlStatus = xlMost150GenerateLightError(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, 10, 10, nRepeat);
  sprintf(tmp, "xlMost150GenerateLightError, PH: %d, xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOST150GenerateLockError

//! generate 'nRepeat' lock errors.
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOST150Functions::MOST150GenerateLockError(unsigned short nRepeat)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
  
  xlStatus = xlMost150GenerateLockError(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab, 10, 100, nRepeat);
  sprintf(tmp, "xlMost150GenerateLockError, PH: %d, xlStatus=0x%x\n", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

void CMOST150Functions::parseEvent(XLmost150event *pxlMost150Event)
{
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  CMOST150ParseEvent::parseEvent(pxlMost150Event);

  if(m_DoInitialStartup && pxlMost150Event->tag == XL_MOST150_DEVICE_MODE)
  {
    m_DoInitialStartup = false;
    xlStatus = xlMost150Startup(m_NodeParam[m_channel].xlPortHandle, m_NodeParam[m_channel].xlChannelMask, 0xab);
    sprintf(tmp, "xlMost150Startup, xlStatus=0x%x\n", xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
  }
}

///////////////////////////////////////////////////////////////////////////

//! RxThread

//! thread to readout the message queue and parse the incoming messages
//!
///////////////////////////////////////////////////////////////////////////

DWORD WINAPI RxThread(LPVOID par) 
{

  XLstatus        xlStatus;
  XLmost150event  xlMost150Event; 
  
  TStruct *th;
  th = (TStruct*) par;  
  th->pMost150Func->m_plRxBox = th->pOutput; 

  g_bThreadRun = TRUE;

  while (g_bThreadRun) { 
   
    WaitForSingleObject(th->hMsgEvent, 10);

    xlStatus = XL_SUCCESS;
  
    while (!xlStatus) {

      xlStatus = xlMost150Receive(th->xlPortHandle, &xlMost150Event);
      
      if (xlStatus != XL_ERR_QUEUE_IS_EMPTY) {
        // Handle the MOST150 events
        th->pMost150Func->parseEvent(&xlMost150Event);

        // Handle the streaming events
        if( (xlMost150Event.tag == XL_MOST150_SYNC_ALLOC_INFO) ||
            (xlMost150Event.tag == XL_MOST150_STREAM_STATE) ||
            (xlMost150Event.tag == XL_MOST150_STREAM_RX_BUFFER) ||
            (xlMost150Event.tag == XL_MOST150_SPECIAL_NODE_INFO))
        {
          th->pMost150Func->MOST150StreamParseEvent(&xlMost150Event);
        }
      }
    
    } 
  }
 
  return NO_ERROR; 
}
