/*-------------------------------------------------------------------------------------------
| File        : xlMOST150ParseEvent.cpp
| Project     : Vector MOST150 Example 
|
| Description : Module to parse and print the received MOST150 events.
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2011 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include "xlMOST150ParseEvent.h"
#include "debug.h"


CMOST150ParseEvent::CMOST150ParseEvent(void)
{
}

CMOST150ParseEvent::~CMOST150ParseEvent(void)
{
}

void CMOST150ParseEvent::printEvent(CString strEvent)
{
  char tmp[1024];
  sprintf_s(tmp, sizeof(tmp), "%s", strEvent.GetBuffer());
  m_plRxBox->InsertString(-1, tmp);
  m_plRxBox->SetCurSel(m_plRxBox->GetCount()-1);
}

unsigned int CMOST150ParseEvent::bitCount(unsigned int mask)
{
  unsigned int i, numBitsSet = 0;

  for(i=0; i<32; ++i)
  {
    if(mask & (1<<i))
      numBitsSet++;
  }

  return numBitsSet;
}

void CMOST150ParseEvent::parseEvent(XLmost150event *pxlMost150Event)
{
  CFixedStringT<CString, 1024> cstr;

  if ((pxlMost150Event->flagsChip) & XL_MOST150_QUEUE_OVERFLOW) {
    printEvent(_T("++ MOST150 OVERFLOW FLAG WAS SET! ++"));
  }

  switch (pxlMost150Event->tag) {

    case XL_TIMER: // do not print timer events
      break;

    case XL_START:
      cstr.Format(_T("START: chIdx=%d, fC=0x%x"), 
        pxlMost150Event->channelIndex, 
        pxlMost150Event->flagsChip);
      printEvent(cstr);
      break;

    case XL_STOP:
      cstr.Format(_T("STOP: chIdx=%d, fC=0x%x"), 
        pxlMost150Event->channelIndex, 
        pxlMost150Event->flagsChip);
      printEvent(cstr);
      break;

    case XL_MOST150_EVENT_SOURCE:
      cstr.Format(_T("EVENT SOURCE: chIdx=%d, fC=0x%x, srcMask=0x%x"), 
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostEventSource.sourceMask);
      printEvent(cstr);
      break;

    case XL_MOST150_DEVICE_MODE:
      cstr.Format(_T("DEVICE MODE: chIdx=%d, fC=0x%x, devMode=0x%x"), 
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostDeviceMode.deviceMode); 
      if (pxlMost150Event->tagData.mostDeviceMode.deviceMode == XL_MOST150_DEVICEMODE_SLAVE) {
        cstr.AppendFormat(_T(" (SLAVE)"));
      }
      else if (pxlMost150Event->tagData.mostDeviceMode.deviceMode == XL_MOST150_DEVICEMODE_MASTER) {
        cstr.AppendFormat(_T(" (MASTER)"));
      }
      else if (pxlMost150Event->tagData.mostDeviceMode.deviceMode == XL_MOST150_DEVICEMODE_STATIC_MASTER) {
        cstr.AppendFormat(_T(" (STATIC MASTER)"));
      }
      else if (pxlMost150Event->tagData.mostDeviceMode.deviceMode == XL_MOST150_DEVICEMODE_RETIMED_BYPASS_SLAVE) {
        cstr.AppendFormat(_T(" (BYPASS SLAVE)"));
      }
      else if (pxlMost150Event->tagData.mostDeviceMode.deviceMode == XL_MOST150_DEVICEMODE_RETIMED_BYPASS_MASTER) {
        cstr.AppendFormat(_T(" (BYPASS MASTER)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_SPECIAL_NODE_INFO:
      {
        unsigned int userRequest = 0;        
        if(bitCount(pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask) > 1) {
          userRequest = 1;
        }
        cstr.Format(_T("SPECIAL NODE INFO: chIdx=%d, fC=0x%x, changeMask=0x%x"),
          pxlMost150Event->channelIndex,
          pxlMost150Event->flagsChip,
          pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask);
        if(userRequest) {
          cstr.AppendFormat(_T(":")); 
          printEvent(cstr);
        }

        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_NA_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (NODE ADDRESS: nodeAddr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.nodeAddress);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (NODE ADDRESS changed: nodeAddr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.nodeAddress);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_GA_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (GROUP ADDRESS: groupAddr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.groupAddress);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (GROUP ADDRESS changed: groupAddr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.groupAddress);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_NPR_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (NPR: npr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.npr);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (NPR changed: npr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.npr);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_MPR_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (MPR: mpr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.mpr);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (MPR changed: mpr=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.mpr);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_SBC_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (SBC: sbc=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.sbc);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (SBC changed: sbc=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.sbc);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_CTRL_RETRY_PARAMS_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (CTRL RETRY PARAM: ctrlRetryTime=0x%x, ctrlSendAttempts=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.ctrlRetryTime,
              pxlMost150Event->tagData.mostSpecialNodeInfo.ctrlSendAttempts);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (CTRL RETRY PARAM changed: ctrlRetryTime=0x%x, ctrlSendAttempts=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.ctrlRetryTime,
              pxlMost150Event->tagData.mostSpecialNodeInfo.ctrlSendAttempts);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_ASYNC_RETRY_PARAMS_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (ASYNC RETRY PARAM: asyncRetryTime=0x%x, asyncSendAttempts=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.asyncRetryTime,
              pxlMost150Event->tagData.mostSpecialNodeInfo.asyncSendAttempts);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (ASYNC RETRY PARAM changed: asyncRetryTime=0x%x, asyncSendAttempts=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.asyncRetryTime,
              pxlMost150Event->tagData.mostSpecialNodeInfo.asyncSendAttempts);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_MAC_ADDR_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (MAC ADDR: macAddr= %02x %02x %02x %02x %02x %02x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[0],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[1],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[2],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[3],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[4],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[5]);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (MAC ADDR changed: macAddr= %02x %02x %02x %02x %02x %02x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[0],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[1],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[2],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[3],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[4],
              pxlMost150Event->tagData.mostSpecialNodeInfo.macAddr[5]);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_NPR_SPY_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (NPR SPY: nprSpy=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.nprSpy);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (NPR SPY changed: nprSpy=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.nprSpy);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_MPR_SPY_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (MPR SPY: mprSpy=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.mprSpy);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (MPR SPY changed: mprSpy=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.mprSpy);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_SBC_SPY_CHANGED) {
          if(userRequest) {
            cstr.Format(_T(" (SBC SPY: sbcSpy=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.sbcSpy);
            printEvent(cstr);
          }
          else {
            cstr.AppendFormat(_T(" (SBC SPY changed: sbcSpy=0x%x)"), 
              pxlMost150Event->tagData.mostSpecialNodeInfo.sbcSpy);
          }
        }
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_INIC_NISTATE_CHANGED) {
          if(pxlMost150Event->tagData.mostSpecialNodeInfo.inicNIState == XL_MOST150_INIC_NISTATE_NET_OFF) {
            cstr.AppendFormat(_T(" (INIC NI STATE: NET_OFF)"));
          } else if(pxlMost150Event->tagData.mostSpecialNodeInfo.inicNIState == XL_MOST150_INIC_NISTATE_NET_INIT) {
            cstr.AppendFormat(_T(" (INIC NI STATE: NET_INIT)"));
          } else if(pxlMost150Event->tagData.mostSpecialNodeInfo.inicNIState == XL_MOST150_INIC_NISTATE_NET_ON) {
            cstr.AppendFormat(_T(" (INIC NI STATE: NET_ON)"));
          } else if(pxlMost150Event->tagData.mostSpecialNodeInfo.inicNIState == XL_MOST150_INIC_NISTATE_NET_RBD) {
            cstr.AppendFormat(_T(" (INIC NI STATE: NET_RBD)"));
          } else if(pxlMost150Event->tagData.mostSpecialNodeInfo.inicNIState == XL_MOST150_INIC_NISTATE_NET_RBD_RESULT) {
            cstr.AppendFormat(_T(" (INIC NI STATE: NET_RBD_RESULT)"));
          }
        }
        if(!userRequest) {
          printEvent(cstr);
        }
      }      
      break;

    case XL_MOST150_CTRL_TX_ACK:
      cstr.Format(_T("CTRL TX ACK: chIdx=%d, fC=0x%x, srcAddr=0x%X, tgtAddr=0x%X, status=%d"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostCtrlTxAck.sourceAddress, 
        pxlMost150Event->tagData.mostCtrlTxAck.targetAddress,
        pxlMost150Event->tagData.mostCtrlTxAck.status);
      if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_OK) {
        cstr.AppendFormat(_T(" (TX OK)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_FAILED_FORMAT_ERROR) {
        cstr.AppendFormat(_T(" (TX FAILED: FORMATERROR)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_FAILED_NETWORK_OFF) {
        cstr.AppendFormat(_T(" (TX FAILED: NETWORK OFF)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_FAILED_TIMEOUT) {
        cstr.AppendFormat(_T(" (TX FAILED: TIMEOUT)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_FAILED_WRONG_TARGET) {
        cstr.AppendFormat(_T(" (TX FAILED: WRONG TARGET)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_OK_ONE_SUCCESS) {
        cstr.AppendFormat(_T(" (TX OK: ONE SUCCESS)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_FAILED_BAD_CRC) {
        cstr.AppendFormat(_T(" (TX FAILED: BAD CRC)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlTxAck.status == XL_MOST150_TX_FAILED_RECEIVER_BUFFER_FULL) {
        cstr.AppendFormat(_T(" (TX FAILED: RECEIVER BUFFER FULL)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_FREQUENCY:
      cstr.Format(_T("FREQUENCY: chIdx=%d, fC=0x%x, frequency=%d"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostFrequency.frequency);
      if (pxlMost150Event->tagData.mostFrequency.frequency == XL_MOST150_FREQUENCY_44100) {
        cstr.AppendFormat(_T(" (44,1 KHZ)"));
      }
      else if (pxlMost150Event->tagData.mostFrequency.frequency == XL_MOST150_FREQUENCY_48000) {
        cstr.AppendFormat(_T(" (48 KHZ)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_CTRL_RX:
      cstr.Format(_T("CTRL RX: chIdx=%d, fC=0x%x, srcAddr=0x%X, tgtAddr=0x%X, fBlockId=0x%x, instId=0x%x, functionId=0x%x, opType=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostCtrlRx.sourceAddress, 
        pxlMost150Event->tagData.mostCtrlRx.targetAddress,
        pxlMost150Event->tagData.mostCtrlRx.fblockId,
        pxlMost150Event->tagData.mostCtrlRx.instId,
        pxlMost150Event->tagData.mostCtrlRx.functionId,
        pxlMost150Event->tagData.mostCtrlRx.opType);
      printEvent(cstr);
      cstr.Format(_T(" ctrlRxData (telLen=%d):"), pxlMost150Event->tagData.mostCtrlRx.telLen);
      for (int i = 0; i < pxlMost150Event->tagData.mostCtrlRx.telLen; i++) {
        cstr.AppendFormat(_T(" %02x"), pxlMost150Event->tagData.mostCtrlRx.ctrlData[i]);
      }
      printEvent(cstr);
      break;

    case XL_MOST150_CTRL_SPY: 
      cstr.Format(_T("CTRL SPY: chIdx=%d, fC=0x%x, srcAddr=0x%x, tgtAddr=0x%x, sta=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostCtrlSpy.sourceAddress, 
        pxlMost150Event->tagData.mostCtrlSpy.targetAddress,
        pxlMost150Event->tagData.mostCtrlSpy.status);
      printEvent(cstr);
      cstr.Format(_T(" ctrlSpyData (ctrlDataLen=%d):"), pxlMost150Event->tagData.mostCtrlSpy.ctrlDataLen);
      for (int i = 0; i < pxlMost150Event->tagData.mostCtrlSpy.ctrlDataLen; i++) {
        cstr.AppendFormat(_T(" %02x"), pxlMost150Event->tagData.mostCtrlSpy.ctrlData[i]);
      }
      printEvent(cstr);
      break;

    case XL_MOST150_ASYNC_TX_ACK:
      cstr.Format(_T("ASYNC TX ACK: chIdx=%d, fC=0x%x, srcAddr=0x%x, tgtAddr=0x%x, length=%d, sta=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostAsyncTxAck.sourceAddress, 
        pxlMost150Event->tagData.mostAsyncTxAck.targetAddress,
        pxlMost150Event->tagData.mostAsyncTxAck.length, 
        pxlMost150Event->tagData.mostAsyncTxAck.status);
      printEvent(cstr);
      break;

    case XL_MOST150_ASYNC_SPY:
      cstr.Format(_T("ASYNC SPY: chIdx=%d, fC=0x%x, srcAddr=0x%x, tgtAddr=0x%x, sta=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostAsyncSpy.sourceAddress, 
        pxlMost150Event->tagData.mostAsyncSpy.targetAddress,
        pxlMost150Event->tagData.mostAsyncSpy.status);
      printEvent(cstr);
      cstr.Format(_T(" asyncSpyData (telLen=%d):"), pxlMost150Event->tagData.mostAsyncSpy.asyncDataLen);
      for (int i = 0; i < pxlMost150Event->tagData.mostAsyncSpy.asyncDataLen; i++) {
        cstr.AppendFormat(_T(" %02x"), pxlMost150Event->tagData.mostAsyncSpy.asyncData[i]);
      }
      printEvent(cstr);
      break;
    
    case XL_MOST150_ASYNC_RX:
      cstr.Format(_T("ASYNC RX: chIdx=%d, fC=0x%x, srcAddr=0x%x, tgtAddr=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostAsyncRx.sourceAddress,
        pxlMost150Event->tagData.mostAsyncRx.targetAddress);
      printEvent(cstr);
      cstr.Format(_T(" asyncRxData (length=%d):"), pxlMost150Event->tagData.mostAsyncRx.length);
      for (int i = 0; i < pxlMost150Event->tagData.mostAsyncRx.length; i++) {
        cstr.AppendFormat(_T(" %02x"), pxlMost150Event->tagData.mostAsyncRx.asyncData[i]);
      }
      printEvent(cstr);
      break;

    case XL_MOST150_SYNC_VOLUME_STATUS:
      cstr.Format(_T("SYNC VOLUME STATUS: chIdx=%d, fC=0x%x, dev=0x%x, vol=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostSyncVolumeStatus.device,
        pxlMost150Event->tagData.mostSyncVolumeStatus.volume);
      printEvent(cstr);
      break;

    case XL_MOST150_TX_LIGHT:
      cstr.Format(_T("TX LIGHT: chIdx=%d, fC=0x%x, light=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostTxLight.light);
      if (pxlMost150Event->tagData.mostTxLight.light == XL_MOST150_LIGHT_OFF) {
        cstr.AppendFormat(_T(" (LIGHT OFF)"));
      }
      else if (pxlMost150Event->tagData.mostTxLight.light == XL_MOST150_LIGHT_FORCE_ON) {
        cstr.AppendFormat(_T(" (LIGHT FORCE ON)"));
      }
      else if (pxlMost150Event->tagData.mostTxLight.light == XL_MOST150_LIGHT_MODULATED) {
        cstr.AppendFormat(_T(" (LIGHT MODULATED)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_RXLIGHT_LOCKSTATUS:
      cstr.Format(_T("RXLIGHT LOCKSTATUS: chIdx=%d, fC=0x%x, status=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostRxLightLockStatus.status);
      if (pxlMost150Event->tagData.mostRxLightLockStatus.status == XL_MOST150_LIGHT_OFF) {
        cstr.AppendFormat(_T(" (LIGHT OFF)"));
      }
      else if (pxlMost150Event->tagData.mostRxLightLockStatus.status == XL_MOST150_LIGHT_ON_UNLOCK) {
        cstr.AppendFormat(_T(" (LIGHT ON UNLOCK)"));
      }
      else if (pxlMost150Event->tagData.mostRxLightLockStatus.status == XL_MOST150_LIGHT_ON_LOCK) {
        cstr.AppendFormat(_T(" (LIGHT ON LOCK)"));
      }
      else if (pxlMost150Event->tagData.mostRxLightLockStatus.status == XL_MOST150_LIGHT_ON_STABLE_LOCK) {
        cstr.AppendFormat(_T(" (LIGHT ON STABLE LOCK)"));
      }
      else if (pxlMost150Event->tagData.mostRxLightLockStatus.status == XL_MOST150_LIGHT_ON_CRITICAL_UNLOCK) {
        cstr.AppendFormat(_T(" (LIGHT ON CRITICAL UNLOCK)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_ERROR:
      cstr.Format(_T("ERROR: chIdx=%d, fC=0x%x, errorCode=0x%x"), 
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostError.errorCode);
      cstr.AppendFormat(_T(" ("));
      if (pxlMost150Event->tagData.mostError.errorCode == XL_MOST150_ERROR_ASYNC_TX_ACK_HANDLE) {
        cstr.AppendFormat(_T("INVALID ASYNC TX HANDLE, "));
      }
      if (pxlMost150Event->tagData.mostError.errorCode == XL_MOST150_ERROR_ETH_TX_ACK_HANDLE) {
        cstr.AppendFormat(_T("INVALID ETHERNET TX HANDLE, "));
      }
      cstr.AppendFormat(_T(")"));
      printEvent(cstr);
      break;

    case XL_MOST150_CONFIGURE_RX_BUFFER:
      cstr.Format(_T("CONFIGURE RX BUFFER: chIdx=%d, fC=0x%x, type=0x%x, mode=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostConfigureRxBuffer.bufferType,
        pxlMost150Event->tagData.mostConfigureRxBuffer.bufferMode);
      if (pxlMost150Event->tagData.mostConfigureRxBuffer.bufferMode == XL_MOST150_RX_BUFFER_NORMAL_MODE) {
        cstr.AppendFormat(_T(" (NORMAL MODE)"));
      }
      else if (pxlMost150Event->tagData.mostConfigureRxBuffer.bufferMode == XL_MOST150_RX_BUFFER_BLOCK_MODE) {
        cstr.AppendFormat(_T(" (BLOCK MODE)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_CTRL_SYNC_AUDIO:
      cstr.Format(_T("CTRL SYNC AUDIO: chIdx=%d, fC=0x%x, device=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostCtrlSyncAudio.device);
      if (pxlMost150Event->tagData.mostCtrlSyncAudio.device == XL_MOST150_DEVICE_LINE_IN) {
        cstr.AppendFormat(_T(" (LINE IN)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlSyncAudio.device == XL_MOST150_DEVICE_LINE_OUT) {
        cstr.AppendFormat(_T(" (LINE OUT)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlSyncAudio.device == XL_MOST150_DEVICE_SPDIF_IN) {
        cstr.AppendFormat(_T(" (SPDIF IN)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlSyncAudio.device == XL_MOST150_DEVICE_SPDIF_OUT) {
        cstr.AppendFormat(_T(" (SPDIF OUT)"));
      }
      cstr.AppendFormat(_T(" mode=0x%x"), 
        pxlMost150Event->tagData.mostCtrlSyncAudio.mode);
      if (pxlMost150Event->tagData.mostCtrlSyncAudio.mode == XL_MOST150_DEVICE_MODE_OFF) {
        cstr.AppendFormat(_T(" (MODE OFF)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlSyncAudio.mode == XL_MOST150_DEVICE_MODE_ON) {
        cstr.AppendFormat(_T(" (MODE ON)"));
      }
      printEvent(cstr);
    break;

    case XL_MOST150_SYNC_MUTE_STATUS:
      cstr.Format(_T("SYNC MUTE STATUS: chIdx=%d, fC=0x%x, dev=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostSyncMuteStatus.device);
      if (pxlMost150Event->tagData.mostSyncMuteStatus.device == XL_MOST150_DEVICE_LINE_IN) {
        cstr.AppendFormat(_T(" (LINE IN)"));
      }
      else if (pxlMost150Event->tagData.mostSyncMuteStatus.device == XL_MOST150_DEVICE_LINE_OUT) {
        cstr.AppendFormat(_T(" (LINE OUT)"));
      }
      else if (pxlMost150Event->tagData.mostSyncMuteStatus.device == XL_MOST150_DEVICE_SPDIF_IN) {
        cstr.AppendFormat(_T(" (SPDIF IN)"));
      }
      else if (pxlMost150Event->tagData.mostSyncMuteStatus.device == XL_MOST150_DEVICE_SPDIF_OUT) {
        cstr.AppendFormat(_T(" (SPDIF OUT)"));
      }
      cstr.AppendFormat(_T(" mute=0x%x"), 
        pxlMost150Event->tagData.mostSyncMuteStatus.mute);
      if (pxlMost150Event->tagData.mostSyncMuteStatus.mute == XL_MOST150_NO_MUTE) {
        cstr.AppendFormat(_T(" (NO MUTE)"));
      }
      else if (pxlMost150Event->tagData.mostSyncMuteStatus.mute == XL_MOST150_MUTE) {
        cstr.AppendFormat(_T(" (MUTE)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_LIGHT_POWER:
      cstr.Format(_T("LIGHT POWER: chIdx=%d, fC=0x%x, lightPower=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostLightPower.lightPower);
      if (pxlMost150Event->tagData.mostLightPower.lightPower == XL_MOST150_LIGHT_FULL) {
        cstr.AppendFormat(_T(" (LIGHT FULL)"));
      }
      else if (pxlMost150Event->tagData.mostLightPower.lightPower == XL_MOST150_LIGHT_3DB) {
        cstr.AppendFormat(_T(" (LIGHT 3DB)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_GEN_LIGHT_ERROR:
      cstr.Format(_T("GEN LIGHT ERROR: chIdx=%d, fC=0x%x, stressStarted=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostGenLightError.stressStarted);
      if (pxlMost150Event->tagData.mostGenLightError.stressStarted == XL_MOST150_MODE_DEACTIVATED) {
        cstr.AppendFormat(_T(" (DEACTIVATED)"));
      }
      else if (pxlMost150Event->tagData.mostGenLightError.stressStarted == XL_MOST150_MODE_ACTIVATED) {
        cstr.AppendFormat(_T(" (ACTIVATED)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_GEN_LOCK_ERROR:
      cstr.Format(_T("GEN LOCK ERROR: chIdx=%d, fC=0x%x, stressStarted=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostGenLockError.stressStarted);
      if (pxlMost150Event->tagData.mostGenLockError.stressStarted == XL_MOST150_MODE_DEACTIVATED) {
        cstr.AppendFormat(_T(" (DEACTIVATED)"));
      }
      else if (pxlMost150Event->tagData.mostGenLockError.stressStarted == XL_MOST150_MODE_ACTIVATED) {
        cstr.AppendFormat(_T(" (ACTIVATED)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_CTRL_BUSLOAD:
      cstr.Format(_T("CTRL BUSLOAD: chIdx=%d, fC=0x%x, stressStarted=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostCtrlBusload.busloadStarted);
      if (pxlMost150Event->tagData.mostCtrlBusload.busloadStarted == XL_MOST150_MODE_DEACTIVATED) {
        cstr.AppendFormat(_T(" (DEACTIVATED)"));
      }
      else if (pxlMost150Event->tagData.mostCtrlBusload.busloadStarted == XL_MOST150_MODE_ACTIVATED) {
        cstr.AppendFormat(_T(" (ACTIVATED)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_ASYNC_BUSLOAD:
      cstr.Format(_T("ASYNC BUSLOAD: chIdx=%d, fC=0x%x, stressStarted=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostAsyncBusload.busloadStarted);
      if (pxlMost150Event->tagData.mostAsyncBusload.busloadStarted == XL_MOST150_MODE_DEACTIVATED) {
        cstr.AppendFormat(_T(" (DEACTIVATED)"));
      }
      else if (pxlMost150Event->tagData.mostAsyncBusload.busloadStarted == XL_MOST150_MODE_ACTIVATED) {
        cstr.AppendFormat(_T(" (ACTIVATED)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_ETHERNET_RX:
      cstr.Format(_T("ETHERNET RX: chIdx=%d, fC=0x%x, srcAddr=[%02x %02x %02x %02x %02x %02x], tgtAddr=[%02x %02x %02x %02x %02x %02x]"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostEthernetRx.sourceAddress[0],
        pxlMost150Event->tagData.mostEthernetRx.sourceAddress[1],
        pxlMost150Event->tagData.mostEthernetRx.sourceAddress[2],
        pxlMost150Event->tagData.mostEthernetRx.sourceAddress[3],
        pxlMost150Event->tagData.mostEthernetRx.sourceAddress[4],
        pxlMost150Event->tagData.mostEthernetRx.sourceAddress[5],
        pxlMost150Event->tagData.mostEthernetRx.targetAddress[0],
        pxlMost150Event->tagData.mostEthernetRx.targetAddress[1],
        pxlMost150Event->tagData.mostEthernetRx.targetAddress[2],
        pxlMost150Event->tagData.mostEthernetRx.targetAddress[3],
        pxlMost150Event->tagData.mostEthernetRx.targetAddress[4],
        pxlMost150Event->tagData.mostEthernetRx.targetAddress[5]);
      printEvent(cstr);
      cstr.Format(_T(" ethRxData (length=%d):"), pxlMost150Event->tagData.mostEthernetRx.length);
      for (unsigned int i = 0; i < pxlMost150Event->tagData.mostEthernetRx.length; i++) {
        cstr.AppendFormat(_T(" %02x"), pxlMost150Event->tagData.mostEthernetRx.ethernetData[i]);
      }
      printEvent(cstr);
      break;

    case XL_MOST150_SYSTEMLOCK_FLAG:
      cstr.Format(_T("SYSTEMLOCK FLAG, chIdx=%d, fC=0x%x, state=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostSystemLockFlag.state);
      if (pxlMost150Event->tagData.mostSystemLockFlag.state == XL_MOST150_SYSTEMLOCK_FLAG_NOT_SET) {
        cstr.AppendFormat(_T(" (FLAG NOT SET)"));
      }
      else if (pxlMost150Event->tagData.mostSystemLockFlag.state == XL_MOST150_SYSTEMLOCK_FLAG_SET) {
        cstr.AppendFormat(_T(" (FLAG SET)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_SHUTDOWN_FLAG:
      cstr.Format(_T("SHUTDOWN FLAG: chIdx=%d, fC=0x%x, state=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostShutdownFlag.state);
      if (pxlMost150Event->tagData.mostShutdownFlag.state == XL_MOST150_SHUTDOWN_FLAG_NOT_SET) {
        cstr.AppendFormat(_T(" (FLAG NOT SET)"));
      }
      if (pxlMost150Event->tagData.mostShutdownFlag.state == XL_MOST150_SHUTDOWN_FLAG_SET) {
        cstr.AppendFormat(_T(" (FLAG SET)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_ETHERNET_SPY:
      cstr.Format(_T("ETHERNET SPY: chIdx=%d, fC=0x%x, srcAddr=[%02x %02x %02x %02x %02x %02x], tgtAddr=[%02x %02x %02x %02x %02x %02x]"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostEthernetSpy.sourceAddress[0],
        pxlMost150Event->tagData.mostEthernetSpy.sourceAddress[1],
        pxlMost150Event->tagData.mostEthernetSpy.sourceAddress[2],
        pxlMost150Event->tagData.mostEthernetSpy.sourceAddress[3],
        pxlMost150Event->tagData.mostEthernetSpy.sourceAddress[4],
        pxlMost150Event->tagData.mostEthernetSpy.sourceAddress[5],
        pxlMost150Event->tagData.mostEthernetSpy.targetAddress[0],
        pxlMost150Event->tagData.mostEthernetSpy.targetAddress[1],
        pxlMost150Event->tagData.mostEthernetSpy.targetAddress[2],
        pxlMost150Event->tagData.mostEthernetSpy.targetAddress[3],
        pxlMost150Event->tagData.mostEthernetSpy.targetAddress[4],
        pxlMost150Event->tagData.mostEthernetSpy.targetAddress[5]);
      printEvent(cstr);
      cstr.Format(_T(" ethSpyData (ethDataLen=%d):"), pxlMost150Event->tagData.mostEthernetSpy.ethernetDataLen);
      for (int i = 0; i < pxlMost150Event->tagData.mostEthernetSpy.ethernetDataLen; i++) {
        cstr.AppendFormat(_T(" %02x"), pxlMost150Event->tagData.mostEthernetSpy.ethernetData[i]);
      }
      printEvent(cstr);
      break;

    case XL_MOST150_ETHERNET_TX_ACK:
      cstr.Format(_T("ETHERNET TX ACK: chIdx=%d, fC=0x%x, srcAddr=[%02x %02x %02x %02x %02x %02x], tgtAddr=[%02x %02x %02x %02x %02x %02x], length=%d"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostEthernetTxAck.sourceAddress[0], 
        pxlMost150Event->tagData.mostEthernetTxAck.sourceAddress[1], 
        pxlMost150Event->tagData.mostEthernetTxAck.sourceAddress[2], 
        pxlMost150Event->tagData.mostEthernetTxAck.sourceAddress[3], 
        pxlMost150Event->tagData.mostEthernetTxAck.sourceAddress[4], 
        pxlMost150Event->tagData.mostEthernetTxAck.sourceAddress[5], 
        pxlMost150Event->tagData.mostEthernetTxAck.targetAddress[0],
        pxlMost150Event->tagData.mostEthernetTxAck.targetAddress[1],
        pxlMost150Event->tagData.mostEthernetTxAck.targetAddress[2],
        pxlMost150Event->tagData.mostEthernetTxAck.targetAddress[3],
        pxlMost150Event->tagData.mostEthernetTxAck.targetAddress[4],
        pxlMost150Event->tagData.mostEthernetTxAck.targetAddress[5],
        pxlMost150Event->tagData.mostEthernetTxAck.length);
      printEvent(cstr);
      break;

    case XL_MOST150_SPDIFMODE:
      cstr.Format(_T("SPDIF MODE: chIdx=%d, fC=0x%x, mode=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostSpdifMode.spdifMode);
      printEvent(cstr);
      break;

    case XL_MOST150_ECL_LINE_CHANGED:
      cstr.Format(_T("ECL LINE CHANGED, chIdx=%d, fC=0x%x, eclLineState=0x%x"), 
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostEclEvent.eclLineState);
      if (pxlMost150Event->tagData.mostEclEvent.eclLineState == XL_MOST150_ECL_LINE_LOW) {
        cstr.AppendFormat(_T(" (LINE LOW)"));
      }
      else if (pxlMost150Event->tagData.mostEclEvent.eclLineState == XL_MOST150_ECL_LINE_HIGH) {
        cstr.AppendFormat(_T(" (LINE HIGH)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_SYNC_ALLOC_INFO:
      {
        unsigned int size = (pxlMost150Event->size - XL_MOST150_RX_EVENT_HEADER_SIZE) / sizeof(XL_MOST150_CL_INFO);
        XL_MOST150_CL_INFO* pInfo = &pxlMost150Event->tagData.mostSyncAllocInfo.allocTable[0];
        cstr.Format(_T("SYNC ALLOC: chIdx=%d, fC=0x%x"), 
          pxlMost150Event->channelIndex,
          pxlMost150Event->flagsChip);
        if(size > 0)
        {
          for(unsigned int i=0; i<size; ++i)
          {
            cstr.AppendFormat(_T(", CL=0x%x, Width=%d"), 
              pInfo[i].label,
              pInfo[i].channelWidth);
          }
        }
        else
        {
          cstr.Append(_T(", No CL available!")); 
        }
        printEvent(cstr);
      }
      break;

    case XL_MOST150_ECL_TERMINATION_CHANGED:
      cstr.Format(_T("ECL TERMINATION CHANGED, chIdx=%d, fC=0x%x, resistorEnabled=0x%x"), 
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostEclTermination.resistorEnabled);
      if (pxlMost150Event->tagData.mostEclTermination.resistorEnabled == XL_MOST150_ECL_LINE_PULL_UP_NOT_ACTIVE) {
        cstr.AppendFormat(_T(" (PULL UP NOT ACTIVE)"));
      }
      else if (pxlMost150Event->tagData.mostEclTermination.resistorEnabled == XL_MOST150_ECL_LINE_PULL_UP_ACTIVE) {
        cstr.AppendFormat(_T(" (PULL UP ACTIVE)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_NW_STARTUP:
      cstr.Format(_T("NW STARTUP: chIdx=%d, fC=0x%x, error=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostStartup.error);
      if (pxlMost150Event->tagData.mostStartup.error == XL_MOST150_STARTUP_NO_ERROR) {
        cstr.AppendFormat(_T(" (NO ERROR)"));
      } else if(pxlMost150Event->tagData.mostStartup.error == 0x20) {
        cstr.AppendFormat(_T(" (errorInfo: 0x%x)"), pxlMost150Event->tagData.mostStartup.errorInfo);
      } else if(pxlMost150Event->tagData.mostStartup.error == 0x40) {
        cstr.AppendFormat(_T(" (BUSY)"));
      } else if(pxlMost150Event->tagData.mostStartup.error == 0x42) {
        cstr.AppendFormat(_T(" (NET_ON not reached)"));
      } else {
        cstr.AppendFormat(_T(" (UNKNOWN ERROR)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_NW_SHUTDOWN:
      cstr.Format(_T("NW SHUTDOWN: chIdx=%d, fC=0x%x, error=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostShutdown.error);
      if (pxlMost150Event->tagData.mostShutdown.error == XL_MOST150_SHUTDOWN_NO_ERROR) {
        cstr.AppendFormat(_T(" (NO ERROR)"));
      }
      else {
        cstr.AppendFormat(_T(" (UNKNOWN ERROR)"));
      }
      printEvent(cstr);
      break;

    case XL_MOST150_STREAM_STATE:
      cstr.Format(_T("STREAM STATE: chIdx=%d, fC=0x%x, state = "),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip);

      if(pxlMost150Event->tagData.mostStreamState.streamState == XL_MOST150_STREAM_STATE_OPENED)
      {
        cstr.AppendFormat(_T("OPENED"));
      }
      else if(pxlMost150Event->tagData.mostStreamState.streamState == XL_MOST150_STREAM_STATE_CLOSED)
      {
        cstr.AppendFormat(_T("CLOSED"));
      }
      else if(pxlMost150Event->tagData.mostStreamState.streamState == XL_MOST150_STREAM_STATE_STARTED)
      {
        cstr.AppendFormat(_T("STARTED"));
      }
      else if(pxlMost150Event->tagData.mostStreamState.streamState == XL_MOST150_STREAM_STATE_STOPPED)
      {
        cstr.AppendFormat(_T("STOPPED"));
      }
      cstr.AppendFormat(_T(", errorInfo=0x%x"), pxlMost150Event->tagData.mostStreamState.streamError);
      printEvent(cstr);      
      break;

    case XL_MOST150_STREAM_RX_BUFFER:
      cstr.Format(_T("RX STREAM BUFFER: chIdx=%d, fC=0x%x, numOfBytes=%d, status=0x%x"),
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip,
        pxlMost150Event->tagData.mostStreamRxBuffer.numberOfBytes,
        pxlMost150Event->tagData.mostStreamRxBuffer.status);
      printEvent(cstr);
      break;

    default: 
      cstr.Format(_T("UNKNOWN event, tag=0x%x, chIdx=%d, fC=0x%x"), 
        pxlMost150Event->tag,
        pxlMost150Event->channelIndex,
        pxlMost150Event->flagsChip);
      printEvent(cstr);
      break;
  }
}
