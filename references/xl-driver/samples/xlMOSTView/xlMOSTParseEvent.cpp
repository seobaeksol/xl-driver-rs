/*-------------------------------------------------------------------------------------------
| File        : xlMOSTParseEvent.cpp
| Project     : Vector MOST Example 
|
| Description : Module to parse and print the received MOST events.
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2005 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include ".\xlmostparseevent.h"
#include "debug.h"


//CMOSTParseEvent::CMOSTParseEvent(CListBox *pOutput)
CMOSTParseEvent::CMOSTParseEvent()
{
  //m_pListBox = pOutput;
}

CMOSTParseEvent::~CMOSTParseEvent(void)
{
}

void CMOSTParseEvent::printEvent(CString strEvent)
{
  char tmp[200];
  sprintf_s(tmp, sizeof(tmp), "%s", strEvent.GetBuffer());
  XLDEBUG(DEBUG_ADV, tmp);
  m_plRxBox->InsertString(-1, tmp);
  m_plRxBox->SetCurSel(m_plRxBox->GetCount()-1);
}

void CMOSTParseEvent::printEvent(CString strEvent, int channelIndex)
{
  char tmp[100];
  sprintf_s(tmp, sizeof(tmp), "%s, ci: %d", strEvent.GetBuffer(), channelIndex);
  XLDEBUG(DEBUG_ADV, tmp);
  m_plRxBox->InsertString(-1, tmp);
  m_plRxBox->SetCurSel(m_plRxBox->GetCount()-1);
}

int CMOSTParseEvent::parseEvent(XLmostEvent *pxlMostEvent)
{
  char tmp[1000];
 
  switch (pxlMostEvent->tag) {

    case XL_MOST_START: { 
      printEvent("XL_MOST_START, measurement started",pxlMostEvent->channelIndex);
      break;
    }

    case XL_MOST_STOP: { 
      printEvent("XL_MOST_STOP, measurement stoped",pxlMostEvent->channelIndex);
      break;
    }
    
    case XL_TIMER: { 
      printEvent("XL_TIMER",pxlMostEvent->channelIndex);
      break;
    }
  
    case XL_MOST_ALLBYPASS: { 
      if (pxlMostEvent->tagData.mostAllBypass.bypassState==XL_MOST_MODE_ACTIVATE) {
        printEvent("XL_MOST_ALLBYPASS = albypass active (closed)",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostAllBypass.bypassState==XL_MOST_MODE_DEACTIVATE) {
        printEvent("XL_MOST_ALLBYPASS = albypass not active (open)",pxlMostEvent->channelIndex);
      } else {
        printEvent("XL_MOST_ALLBYPASS = unknown parameter",pxlMostEvent->channelIndex);
      }
      break;
    }
     
    case XL_MOST_TIMINGMODE: {
      if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_MASTER) {
        printEvent("XL_MOST_TIMINGMODE = Master",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_SLAVE) {
        printEvent("XL_MOST_TIMINGMODE = Slave",pxlMostEvent->channelIndex);
      } else {
        printEvent("XL_MOST_TIMINGMODE = unknown parameter",pxlMostEvent->channelIndex);
      }
      break;
    }

	case XL_MOST_TIMINGMODE_SPDIF: {
      if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_MASTER) {
        printEvent("XL_MOST_TIMINGMODE_SPIF = Master",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_SLAVE) {
		  printEvent("XL_MOST_TIMINGMODE_SPDIF = Slave",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_SLAVE_SPDIF_MASTER) {
        printEvent("XL_MOST_TIMINGMODE_SPIF = Slave and SPDIF Master",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_SLAVE_SPDIF_SLAVE) {
		  printEvent("XL_MOST_TIMINGMODE_SPDIF = Slave and SPDIF Slave",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_MASTER_SPDIF_MASTER) {
        printEvent("XL_MOST_TIMINGMODE_SPIF = Master and SPDIF Master",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTimingMode.timingmode==XL_MOST_TIMING_MASTER_SPDIF_SLAVE) {
		  printEvent("XL_MOST_TIMINGMODE_SPDIF = Master and SPDIF Slave",pxlMostEvent->channelIndex);
      } else { 
        printEvent("XL_MOST_TIMINGMODE_SPDIF = unknown parameter",pxlMostEvent->channelIndex);
      }
      break;
    }

    case XL_MOST_FREQUENCY: {
      if (pxlMostEvent->tagData.mostFrequency.frequency==XL_MOST_FREQUENCY_48000) {
        printEvent("XL_MOST_FREQUENCY = 48000Hz",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostFrequency.frequency==XL_MOST_FREQUENCY_44100) {
        printEvent("XL_MOST_FREQUENCY = 44100Hz",pxlMostEvent->channelIndex);
      } else {
        printEvent("XL_MOST_FREQUENCY = unknown parameter",pxlMostEvent->channelIndex);
      }
      break;
    }
  
    case XL_MOST_REGISTER_BYTES: {
      sprintf(tmp, "XL_MOST_REGISTER_BYTES: nr.: %d, Adr.: 0x%x, ci: %d", 
          pxlMostEvent->tagData.mostRegisterBytes.number, 
          pxlMostEvent->tagData.mostRegisterBytes.address,
          pxlMostEvent->channelIndex);
        printEvent(tmp);
      break;
    }

    case XL_MOST_REGISTER_BITS: {
      printEvent("XL_MOST_REGISTER_BITS",pxlMostEvent->channelIndex);
      break;
    }

    case XL_MOST_SPECIAL_REGISTER: {
      sprintf(tmp, "XL_MOST_SPECIAL_REGISTER: NA: 0x%02x%02x, GA: 0x03%02x, ci: %d", 
          pxlMostEvent->tagData.mostSpecialRegister.register_bNAH,
          pxlMostEvent->tagData.mostSpecialRegister.register_bNAL,
          pxlMostEvent->tagData.mostSpecialRegister.register_bGA,
          pxlMostEvent->channelIndex);
        printEvent(tmp);
      break;
    }

    // full support only in the MOST Analyses library                                    
    case XL_MOST_CTRL_RX_SPY: {
      sprintf(tmp, "XL_MOST_CTRL_RX_SPY: TA: 0x%04x, SA: 0x%04x, ci: %d", 
          pxlMostEvent->tagData.mostCtrlSpy.targetAddress,
          pxlMostEvent->tagData.mostCtrlSpy.sourceAddress,
          pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }

    case XL_MOST_CTRL_RX_OS8104: {
      printEvent("XL_MOST_CTRL_OS8104",pxlMostEvent->channelIndex);
      break;
    }

    case XL_MOST_CTRL_TX: {
      sprintf(tmp, "XL_MOST_CTRL_TX: chip: 0x%08X, status: 0x%08X, ci: %d", 
               pxlMostEvent->flagsChip,pxlMostEvent->tagData.mostCtrlMsg.status, pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }

    case XL_MOST_ASYNC_MSG: {
      printEvent("XL_MOST_ASYNC_MSG",pxlMostEvent->channelIndex);
      break;
    }
  
    case XL_MOST_ASYNC_TX: {
      printEvent("XL_MOST_ASYNC_TX",pxlMostEvent->channelIndex);
      break;
    }
    
    case XL_MOST_SYNC_ALLOCTABLE: {
      printEvent("XL_MOST_SYNC_ALLOCTABLE",pxlMostEvent->channelIndex);
      break;
    }
    
    case XL_MOST_SYNC_VOLUME_STATUS: {
      printEvent("XL_MOST_SYNC_VOLUME_STATUS",pxlMostEvent->channelIndex);
      break;
    }
    
    case XL_MOST_RXLIGHT: {
      if (pxlMostEvent->tagData.mostTxLight.light==XL_MOST_LIGHT_OFF) {
        printEvent("XL_MOST_RXLIGHT: light off",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTxLight.light==XL_MOST_LIGHT_MODULATED) {
        printEvent("XL_MOST_RXLIGHT: modulated light",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTxLight.light==XL_MOST_LIGHT_FORCE_ON) {
        printEvent("XL_MOST_RXLIGHT: light forced on",pxlMostEvent->channelIndex);
      } else {
        printEvent("XL_MOST_RXLIGHT: unknown",pxlMostEvent->channelIndex);
      }
      break;
    }
  
    case XL_MOST_TXLIGHT: {
      if (pxlMostEvent->tagData.mostTxLight.light==XL_MOST_LIGHT_OFF) {
        printEvent("XL_MOST_TXLIGHT: light off",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTxLight.light==XL_MOST_LIGHT_MODULATED) {
        printEvent("XL_MOST_TXLIGHT: modulated light",pxlMostEvent->channelIndex);
      } else if (pxlMostEvent->tagData.mostTxLight.light==XL_MOST_LIGHT_FORCE_ON) {
        printEvent("XL_MOST_TXLIGHT: light forced on",pxlMostEvent->channelIndex);
      } else {
        printEvent("XL_MOST_TXLIGHT: unknown",pxlMostEvent->channelIndex);
      }
      break;
    }

    case XL_MOST_LOCKSTATUS: {
      if (XL_MOST_UNLOCK == pxlMostEvent->tagData.mostLockStatus.lockStatus) {
        printEvent("XL_MOST_LOCKSTATUS: unlock", pxlMostEvent->channelIndex);
      }
      else if (XL_MOST_LOCK == pxlMostEvent->tagData.mostLockStatus.lockStatus) {
        printEvent("XL_MOST_LOCKSTATUS: lock", pxlMostEvent->channelIndex);
      }
      else {
        printEvent("XL_MOST_LOCKSTATUS: unknown state", pxlMostEvent->channelIndex);
      }
      break;
    }
    
    case XL_MOST_ERROR: {
      printEvent("XL_MOST_ERROR",pxlMostEvent->channelIndex);
      break;
    }
    
    // full support only in the MOST Analyses library      
    case XL_MOST_CTRL_RXBUFFER: {
      printEvent("XL_MOST_CTRL_RXBUFFER",pxlMostEvent->channelIndex);
      break;
    }  
    
    case XL_MOST_CTRL_SYNC_AUDIO: {
		sprintf(tmp, "XL_MOST_CTRL_SYNC_AUDIO, ci: %d, device: %d",pxlMostEvent->channelIndex, pxlMostEvent->tagData.mostCtrlSyncAudio.device);
		printEvent(tmp);
      break;
    }  
    
	case XL_MOST_CTRL_SYNC_AUDIO_EX: {
		sprintf(tmp, "XL_MOST_CTRL_SYNC_AUDIO_EX, ci: %d, device: %d",pxlMostEvent->channelIndex, pxlMostEvent->tagData.mostCtrlSyncAudio.device);
		printEvent(tmp);
      break;
    }  

    case XL_MOST_SYNC_MUTE_STATUS: {
      printEvent("XL_MOST_SYNC_MUTE_STATUS",pxlMostEvent->channelIndex);
      break;
    }
      
    // full support only in the MOST Analyses library     
    case XL_MOST_GENLIGHTERROR: {
      sprintf(tmp, "XL_MOST_GENLIGHTERROR: OnTime=%dms, OffTime=%dms, repeat=%d, ci: %d",
       pxlMostEvent->tagData.mostGenLightError.lightOnTime, pxlMostEvent->tagData.mostGenLightError.lightOffTime,
       pxlMostEvent->tagData.mostGenLightError.repeat,
       pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }  
    
    // full support only in the MOST Analyses library     
    case XL_MOST_GENLOCKERROR: {
       sprintf(tmp, "XL_MOST_GENLOCKERROR: OnTime=%dms, OffTime=%dms, repeat=%d, ci: %d",
       pxlMostEvent->tagData.mostGenLockError.lockOnTime, pxlMostEvent->tagData.mostGenLockError.lockOffTime,
       pxlMostEvent->tagData.mostGenLockError.repeat,
       pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }  

    case XL_MOST_TXLIGHT_POWER: {
      printEvent("XL_MOST_TXLIGHT_POWER",pxlMostEvent->channelIndex);
      break;
    }  

    // full support only in the MOST Analyses library     
    case XL_MOST_CTRL_BUSLOAD: {
      sprintf(tmp, "XL_MOST_CTRL_BUSLOAD: busloadCtrlStarted: %d, ci: %d",
        pxlMostEvent->tagData.mostCtrlBusload.busloadCtrlStarted,
        pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }  

    // full support only in the MOST Analyses library     
    case XL_MOST_ASYNC_BUSLOAD: {
      sprintf(tmp, "XL_MOST_ASYNC_BUSLOAD: busloadCtrlStarted: %d, ci: %d",
        pxlMostEvent->tagData.mostAsyncBusload.busloadAsyncStarted,
        pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }  

    // full support only in the MOST Analyses library     
    case XL_SYNC_PULSE: {
      printEvent("XL_SYNC_PULSE",pxlMostEvent->channelIndex);
      break;
    }

    // full support only in the MOST Analyses library      
    case XL_MOST_EVENTSOURCES: { 
      sprintf(tmp, "XL_MOST_EVENTSOURCES: mask=0x%08X, state=0x%08X, ci: %d",
       pxlMostEvent->tagData.mostEventSource.mask, pxlMostEvent->tagData.mostEventSource.state,
       pxlMostEvent->channelIndex);
      printEvent(tmp);
      break;
    }

    case XL_MOST_STREAM_BUFFER: {
      sprintf(tmp, "XL_MOST_STREAM_BUFFER: validBytes:%d, stat:0x%x",
       pxlMostEvent->tagData.mostStreamBuffer.validBytes,
       pxlMostEvent->tagData.mostStreamBuffer.status);
      printEvent(tmp);
      break;
    }

    case XL_MOST_STREAM_STATE: {
      sprintf(tmp, "XL_MOST_STREAM_STATE: state:0x%x",
        pxlMostEvent->tagData.mostStreamState.streamState);
      printEvent(tmp);
      break;
    }

    default: {
      sprintf(tmp, "unknown event, tag: 0x%x, size %d, %I64d", pxlMostEvent->tag, pxlMostEvent->size, pxlMostEvent->timeStamp);
      printEvent(tmp);
    }
  }

  return 0;
}
