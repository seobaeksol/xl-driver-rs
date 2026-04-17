/*----------------------------------------------------------------------------
| File        : LINFunctions.cpp
| Project     : Vector LIN Example - manage the LIN access
|
| Description : 
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlLINExample.h"
#include "xlLINFunctions.h"
#include "debug.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// ---------------------------------------------------
// globals
BOOL    g_bThreadRun;
TStruct g_th;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLINFunctions::CLINFunctions() {
  m_xlPortHandle          = XL_INVALID_PORTHANDLE;
  m_xlChannelMask[MASTER] = m_xlChannelMask[SLAVE] = 0;
}

CLINFunctions::~CLINFunctions() {}

////////////////////////////////////////////////////////////////////////////

//! LINGetDevice

//! readout the registry to get the hardware information. If there is no
//! application which is named "xlLINExample" a new one is generated.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::LINGetDevice() {
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  xlStatus = xlOpenDriver();
  sprintf_s(tmp, sizeof(tmp), "xlOpenDriver, stat: %d", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus != XL_SUCCESS) {
    return xlStatus;
  }

  xlStatus = linGetChannelMask();

  // we need minimum one LIN channel for MASTER/SLAVE config
  if (m_xlChannelMask[MASTER] == 0) {
    return XL_ERROR;
  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! LINInit

//! LINExample use ONE port for master and slave which is opened. Also a
//! thread for all incoming messages is created.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::LINInit(int linID) {
  XLstatus xlStatus = XL_ERROR;
  XLaccess m_xlChannelMask_both;
  XLaccess xlPermissionMask;
  char     tmp[100];

  // ---------------------------------------
  // Open ONE port for both channels master+slave
  // ---------------------------------------

  // calculate the channelMask for both channel
  m_xlChannelMask_both = m_xlChannelMask[MASTER] | m_xlChannelMask[SLAVE];
  xlPermissionMask     = m_xlChannelMask_both;

  xlStatus = xlOpenPort(&m_xlPortHandle, "LIN Example", m_xlChannelMask_both, &xlPermissionMask, 256, XL_INTERFACE_VERSION,
                        XL_BUS_TYPE_LIN);
  sprintf_s(tmp, sizeof(tmp), "xlOpenPort: PortHandle: %d; Permissionmask: 0x%I64x; Status: %d", m_xlPortHandle, xlPermissionMask,
            xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  m_pStatusBox->InsertString(-1, _T("xlOpenPort done"));

  if (m_xlPortHandle == XL_INVALID_PORTHANDLE) {
    return XL_ERROR;
  }
  if (xlStatus != XL_SUCCESS) {
    m_xlPortHandle = XL_INVALID_PORTHANDLE;
    return xlStatus;
  }

  // ---------------------------------------
  // Create ONE thread for both channels
  // ---------------------------------------

  linCreateRxThread();

  // ---------------------------------------
  // Init each channel (master+slave)
  // ---------------------------------------

  xlStatus = linInitMaster(linID);
  if (xlStatus) {
    m_pStatusBox->InsertString(-1, "Init Master failed!");
    return xlStatus;
  }
  sprintf_s(tmp, sizeof(tmp), "Init M/Slave id:%d", linID);
  m_pStatusBox->InsertString(-1, tmp);

  // for the next slave we take the next ID
  linID++;

  // if we have a second channel we setup a LIN slave
  if (m_xlChannelMask[SLAVE]) {
    xlStatus = linInitSlave(linID);
    if (xlStatus) {
      m_pStatusBox->InsertString(-1, "Init Slave failed!");
      return xlStatus;
    }
    sprintf_s(tmp, sizeof(tmp), "Init Slave id:%d", linID);
    m_pStatusBox->InsertString(-1, tmp);
  }

  return xlStatus;
}

///////////////////////////////////////////////////////////////////////////

//! LINClose()

//! Close the port
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::LINClose() {
  XLstatus xlStatus           = XL_SUCCESS;
  XLaccess xlChannelMask_both = m_xlChannelMask[MASTER] | m_xlChannelMask[SLAVE];
  char     tmp[100];

  g_bThreadRun = FALSE;

  // Wait until the thread is done...
  Sleep(100);

  if (XL_INVALID_PORTHANDLE == m_xlPortHandle) {
    return (xlStatus);
  }

  if (xlChannelMask_both) {
    xlStatus = xlDeactivateChannel(m_xlPortHandle, xlChannelMask_both);
    sprintf_s(tmp, sizeof(tmp), "xlDeactivateChannel, status: %d\n", xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
    if (xlStatus) {
      return xlStatus;
    }
  }

  xlStatus = xlClosePort(m_xlPortHandle);
  sprintf_s(tmp, sizeof(tmp), "xlClosePort, status: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus) {
    return xlStatus;
  }

  m_xlPortHandle = XL_INVALID_PORTHANDLE;

  xlStatus = xlCloseDriver();
  sprintf_s(tmp, sizeof(tmp), "xlCloseDriver, status: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);
  if (xlStatus) {
    return xlStatus;
  }

  m_pStatusBox->InsertString(-1, "Close All");

  return xlStatus;
}

///////////////////////////////////////////////////////////////////////////

//! LINSendMasterReq()

//! Sends a master request to the LIN bus.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::LINSendMasterReq(BYTE data, int linID) {
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  // send the master request
  xlStatus = xlLinSendRequest(m_xlPortHandle, m_xlChannelMask[MASTER], (unsigned char)linID, 0);
  sprintf_s(tmp, sizeof(tmp), "SendRq CM: '%I64u', status: %d", m_xlChannelMask[MASTER], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  m_pStatusBox->InsertString(-1, "Master Request");

  // setup the only slave channel (LIN ID + 1)
  xlStatus = linSetSlave(linID + 1, data);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! linGetChannelMask

//! parse the registry to get the channelmask
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::linGetChannelMask() {
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  XLdriverConfig xlDrvConfig;

  // default values
  unsigned int hwType;
  unsigned int hwIndex;
  unsigned int hwChannel;
  unsigned int appChannel;
  int          channelIndex;
  unsigned int busType = XL_BUS_TYPE_LIN;
  unsigned int i;

  //check for hardware:
  xlStatus = xlGetDriverConfig(&xlDrvConfig);
  if (xlStatus) {
    XLDEBUG(DEBUG_ADV, "Error in xlGetDriverConfig...");
    return xlStatus;
  }

  for (appChannel = 0; appChannel < 2; appChannel++) {
    xlStatus = xlGetApplConfig("xlLINExample", appChannel, &hwType, &hwIndex, &hwChannel, busType);

    if (xlStatus != XL_SUCCESS) {
      // Set the params into registry (default values...!)
      XLDEBUG(DEBUG_ADV, "set in registry");

      hwChannel = appChannel;

      for (i = 0; i < xlDrvConfig.channelCount; i++) {
        // check PC for hardware with LINCabs or LINPiggy's
        if (xlDrvConfig.channel[i].channelBusCapabilities & XL_BUS_ACTIVE_CAP_LIN) {
          hwType = xlDrvConfig.channel[i].hwType;
          sprintf_s(tmp, sizeof(tmp), "Found LIN hWType: %d;\n", hwType);
          XLDEBUG(DEBUG_ADV, tmp);

          xlStatus = xlSetApplConfig(  // Registration of Application with default settings
            "xlLINExample",            // Application Name
            appChannel,                // Application channel 0 or 1
            hwType,                    // hwType  (CANcardXL...)
            hwIndex,                   // Index of hardware (slot) (0,1,...)
            hwChannel,                 // Index of channel (connector) (0,1,...)
            busType);                  // the application is for LIN.
        }
      }
    }
    else {
      XLDEBUG(DEBUG_ADV, "found in registry");
    }

    channelIndex = xlGetChannelIndex(hwType, hwIndex, hwChannel);

    // check if we have a valid LIN cab/piggy
    if (xlDrvConfig.channel[channelIndex].channelBusCapabilities & XL_BUS_ACTIVE_CAP_LIN) {
      XLDEBUG(DEBUG_ADV, "Found LIN cab/piggy\n");
      // and check the right hardwaretype
      if (xlDrvConfig.channel[channelIndex].hwType == hwType) {
        m_xlChannelMask[appChannel + 1] = xlGetChannelMask(hwType, hwIndex, hwChannel);
      }
    }
    else {
      XLDEBUG(DEBUG_ADV, "No LIN cab/piggy found\n");
    }

    sprintf_s(tmp, sizeof(tmp), "Init LIN hWType: %d; hWIndex: %d; hwChannel: %d; channelMask: 0x%I64x for appChannel: %d\n", hwType,
              hwIndex, hwChannel, m_xlChannelMask[appChannel + 1], appChannel);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! linCreateRxThread

//! set the notification and creates the thread.
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::linCreateRxThread() {
  XLstatus xlStatus = XL_ERROR;
  DWORD    ThreadId = 0;
  char     tmp[100];

  if (m_xlPortHandle != XL_INVALID_PORTHANDLE) {
    sprintf_s(tmp, sizeof(tmp), "Found OpenPort: %d", m_xlPortHandle);
    XLDEBUG(DEBUG_ADV, tmp);

    // Send a event for each Msg!!!
    xlStatus = xlSetNotification(m_xlPortHandle, &m_hMsgEvent, 1);
    sprintf_s(tmp, sizeof(tmp), "SetNotification '%p', xlStatus: %d", m_hMsgEvent, xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);

    // for the RxThread
    g_th.xlPortHandle = m_xlPortHandle;
    g_th.hMsgEvent    = m_hMsgEvent;
    g_th.pListRX      = m_pRXBox;
    g_th.pStatusBox   = m_pStatusBox;

    m_hThread = CreateThread(0, 0x1000, RxThread, (LPVOID)&g_th, 0, &ThreadId);
    sprintf_s(tmp, sizeof(tmp), "CreateThread %p", m_hThread);
    XLDEBUG(DEBUG_ADV, tmp);
  }
  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! linInitMaster

//! initialize the LIN master, set the master DLC's, opens the
//! message filter and activate the LIN channel (-> bus on).
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::linInitMaster(int linID) {
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  unsigned char data[8];
  unsigned char DLC[64];

  // ---------------------------------------
  // Setup the channel as a MASTER
  // ---------------------------------------

  XLlinStatPar xlStatPar;

  xlStatPar.LINMode    = XL_LIN_MASTER;
  xlStatPar.baudrate   = DEFAULT_LIN_BAUDRATE;  // set the baudrate
  xlStatPar.LINVersion = XL_LIN_VERSION_1_3;    // use LIN 1.3

  xlStatus = xlLinSetChannelParams(m_xlPortHandle, m_xlChannelMask[MASTER], xlStatPar);

  sprintf_s(tmp, sizeof(tmp), "Init Master PH: '%d', CM: '0x%I64x', status: %d\n", m_xlPortHandle, m_xlChannelMask[MASTER], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // Setup the Master DLC's
  // ---------------------------------------

  // set the DLC for all ID's to DEFAULT_LIN_DLC
  for (int i = 0; i < 64; i++) {
    DLC[i] = DEFAULT_LIN_DLC;
  }

  xlStatus = xlLinSetDLC(m_xlPortHandle, m_xlChannelMask[MASTER], DLC);
  sprintf_s(tmp, sizeof(tmp), "xlLinSetDLC, CM: '0x%I64x', status: %d", m_xlChannelMask[MASTER], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // Setup the channel as a SLAVE also
  // ---------------------------------------

  memset(data, 0, 8);

  xlStatus = xlLinSetSlave(m_xlPortHandle, m_xlChannelMask[MASTER], (unsigned char)linID, data, DEFAULT_LIN_DLC, XL_LIN_CALC_CHECKSUM);
  sprintf_s(tmp, sizeof(tmp), "Set Slave id:%d, CM: '0x%I64x', status: %d\n", linID, m_xlChannelMask[MASTER], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // Activate the Master Channel
  // ---------------------------------------

  xlStatus = xlActivateChannel(m_xlPortHandle, m_xlChannelMask[MASTER], XL_BUS_TYPE_LIN, XL_ACTIVATE_RESET_CLOCK);
  sprintf_s(tmp, sizeof(tmp), "Activate Channel, CM: '0x%I64x', status: %d\n", m_xlChannelMask[MASTER], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if (xlStatus != XL_SUCCESS) {
    return xlStatus;
  }

  xlStatus = xlFlushReceiveQueue(m_xlPortHandle);
  sprintf_s(tmp, sizeof(tmp), "FlushReceiveQueue stat: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! linInitSlave

//! initialize the LIN slave, define the slave (id, dlc, data), opens the
//! message filter and activate the LIN channel (-> bus on).
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::linInitSlave(int linID) {
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  unsigned char data[8];
  unsigned char DLC[64];

  // ---------------------------------------
  // Setup the channel as a SLAVE
  // ---------------------------------------

  XLlinStatPar xlStatPar;

  xlStatPar.LINMode    = XL_LIN_SLAVE;
  xlStatPar.baudrate   = DEFAULT_LIN_BAUDRATE;  // set the baudrate
  xlStatPar.LINVersion = XL_LIN_VERSION_1_3;    // use LIN 1.3

  xlStatus = xlLinSetChannelParams(m_xlPortHandle, m_xlChannelMask[SLAVE], xlStatPar);

  sprintf_s(tmp, sizeof(tmp), "Init Slave PH: '%d', CM: '0x%I64x', status: %d\n", m_xlPortHandle, m_xlChannelMask[SLAVE], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // Setup the Slave DLC's
  // ---------------------------------------

  // set the DLC for all ID's to DEFAULT_LIN_DLC
  for (int i = 0; i < 64; i++) {
    DLC[i] = DEFAULT_LIN_DLC;
  }

  xlStatus = xlLinSetDLC(m_xlPortHandle, m_xlChannelMask[SLAVE], DLC);
  sprintf_s(tmp, sizeof(tmp), "xlLinSetDLC, CM: '0x%I64x', status: %d", m_xlChannelMask[SLAVE], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // Setup the SLAVE
  // ---------------------------------------

  memset(data, 0, 8);

  xlStatus = xlLinSetSlave(m_xlPortHandle, m_xlChannelMask[SLAVE], (unsigned char)linID, data, DEFAULT_LIN_DLC, XL_LIN_CALC_CHECKSUM);
  sprintf_s(tmp, sizeof(tmp), "Set Slave id:%d, CM: '0x%I64x', status: %d\n", linID, m_xlChannelMask[SLAVE], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  // ---------------------------------------
  // Activate the Slave Channel
  // ---------------------------------------

  xlStatus = xlActivateChannel(m_xlPortHandle, m_xlChannelMask[SLAVE], XL_BUS_TYPE_LIN, XL_ACTIVATE_RESET_CLOCK);
  sprintf_s(tmp, sizeof(tmp), "Activate Channel CM: '0x%I64x', status: %d\n", m_xlChannelMask[SLAVE], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  if (xlStatus != XL_SUCCESS) {
    return xlStatus;
  }

  xlStatus = xlFlushReceiveQueue(m_xlPortHandle);
  sprintf_s(tmp, sizeof(tmp), "FlushReceiveQueue stat: %d\n", xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! linSetSlave

//! change the slave
//!
////////////////////////////////////////////////////////////////////////////

XLstatus CLINFunctions::linSetSlave(int linID, byte databyte) {
  XLstatus xlStatus = XL_ERROR;
  char     tmp[100];

  unsigned char data[8];

  data[0] = databyte;
  data[1] = 0x00;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x00;
  data[6] = 0x00;
  data[7] = 0x00;

  xlStatus = xlLinSetSlave(m_xlPortHandle, m_xlChannelMask[SLAVE], (unsigned char)linID, data, DEFAULT_LIN_DLC, XL_LIN_CALC_CHECKSUM);
  sprintf_s(tmp, sizeof(tmp), "Set Slave ID CM: '0x%I64x', status: %d", m_xlChannelMask[SLAVE], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

///////////////////////////////////////////////////////////////////////////

//! RxThread

//! thread to readout the message queue and parse the incoming messages
//!
////////////////////////////////////////////////////////////////////////////

DWORD WINAPI RxThread(LPVOID par) {
  XLstatus xlStatus;

  //char            tmp[100];
  unsigned int msgsrx = RECEIVE_EVENT_SIZE;
  XLevent      xlEvent;
  char         tmp[100];
  CString      str;

  g_bThreadRun = TRUE;

  TStruct* pTh;

  pTh = (TStruct*)par;

  sprintf_s(tmp, sizeof(tmp), "thread: SetNotification '%p'", pTh->hMsgEvent);
  XLDEBUG(DEBUG_ADV, tmp);

  while (g_bThreadRun) {
    WaitForSingleObject(pTh->hMsgEvent, 10);

    xlStatus = XL_SUCCESS;

    while (!xlStatus) {
      msgsrx   = RECEIVE_EVENT_SIZE;
      xlStatus = xlReceive(pTh->xlPortHandle, &msgsrx, &xlEvent);

      if (xlStatus != XL_ERR_QUEUE_IS_EMPTY) {
        //sprintf_s(tmp, sizeof(tmp), "thread: xlReceive tag: '%d'", xlEvent.tag);
        //DEBUG(DEBUG_ADV, tmp);

        switch (xlEvent.tag) {
          // CAN events
          case XL_SYNC_PULSE:
            sprintf_s(tmp, sizeof(tmp), "SYNC_PULSE: on Ch: '%d'", xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;

          case XL_TRANSCEIVER:
            sprintf_s(tmp, sizeof(tmp), "TRANSCEIVER: on Ch: '%d'", xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;

            // LIN events

          case XL_LIN_NOANS:
            sprintf_s(tmp, sizeof(tmp), "LIN NOANS ID: '0x%x' on Ch: '%d', time: %I64u", xlEvent.tagData.linMsgApi.linNoAns.id,
                      xlEvent.chanIndex, xlEvent.timeStamp);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;

          case XL_LIN_MSG: {
            CString str1, sData;
            str = "RX: ";
            if (xlEvent.tagData.linMsgApi.linMsg.flags & XL_LIN_MSGFLAG_TX) {
              str = "TX: ";
            }

            str1 = "";
            for (int i = 0; i < xlEvent.tagData.linMsgApi.linMsg.dlc; i++) {
              str1.Format(_T("%02x"), xlEvent.tagData.linMsgApi.linMsg.data[i]);
              sData = sData + str1;
            }

            sprintf_s(tmp, sizeof(tmp), "ID: 0x%02x, dlc: '%d', Data: 0x%s, time: %I64u, Ch: '%d'", xlEvent.tagData.linMsgApi.linMsg.id,
                      xlEvent.tagData.linMsgApi.linMsg.dlc, sData.GetBuffer(), xlEvent.timeStamp, xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, str + tmp);
            break;
          }

          case XL_LIN_SLEEP:
            sprintf_s(tmp, sizeof(tmp), "LIN SLEEP flag: 0x%x, time: %I64u, Ch: '%d'", xlEvent.tagData.linMsgApi.linSleep.flag,
                      xlEvent.timeStamp, xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;

          case XL_LIN_ERRMSG:
            sprintf_s(tmp, sizeof(tmp), "LIN ERROR, Ch: '%d'", xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;

          case XL_LIN_SYNCERR:
            sprintf_s(tmp, sizeof(tmp), "LIN SYNCERR on Ch: '%d'", xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;

          case XL_LIN_WAKEUP:
            sprintf_s(tmp, sizeof(tmp), "LIN WAKEUP flags: 0x%x on Ch: '%d'", xlEvent.tagData.linMsgApi.linWakeUp.flag, xlEvent.chanIndex);
            XLDEBUG(DEBUG_ADV, tmp);
            pTh->pListRX->InsertString(-1, tmp);
            break;
        }
        ResetEvent(pTh->hMsgEvent);

        //int nCount = pmyListBox->GetCount();
        //if (nCount > 0)
        pTh->pListRX->SetCurSel(pTh->pListRX->GetCount() - 1);
        pTh->pStatusBox->SetCurSel(pTh->pStatusBox->GetCount() - 1);
      }
    }
  }
  return NO_ERROR;
}
