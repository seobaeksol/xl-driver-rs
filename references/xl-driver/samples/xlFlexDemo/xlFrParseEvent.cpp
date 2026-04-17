/*-------------------------------------------------------------------------------------------
| File        : xlFrParseEvent.cpp
| Project     : Vector FlexRay Example
|
| Description : Module to parse and print the received FlexRay events.
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2013 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include ".\xlFrParseEvent.h"
#include "debug.h"


////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
////////////////////////////////////////////////////////////////////////////

CFrParseEvent::CFrParseEvent(CListCtrl *plMsg)
{
  m_plMsg = plMsg;
}

CFrParseEvent::~CFrParseEvent(void)
{
}

////////////////////////////////////////////////////////////////////////////

//! printEvent

//! 
//!
//!
////////////////////////////////////////////////////////////////////////////

void CFrParseEvent::printEvent(XLfrEvent *pxlFrEvent, LPTSTR sType)
{
 // write into listbox
  LVITEM  lvi;
  char sValue[100];  

  // create the line
  lvi.mask =  LVIF_IMAGE | LVIF_TEXT;

  sprintf(sValue, "%I64d", pxlFrEvent->timeStampSync);
 
  lvi.iItem = m_plMsg->GetItemCount();
  lvi.iSubItem = 0;
  lvi.pszText = sValue;
  lvi.iImage = 0;
  m_plMsg->InsertItem(&lvi);
  
  printValue(&lvi, 4, sType);
}

////////////////////////////////////////////////////////////////////////////

//! printValue

//! 
//!
//!
////////////////////////////////////////////////////////////////////////////

void CFrParseEvent::printValue(LVITEM *pLvi, unsigned int item, LPTSTR sValue)
{
  // write the type
  pLvi->iSubItem = item;
  pLvi->pszText = sValue;
  m_plMsg->SetItem(pLvi);
}

////////////////////////////////////////////////////////////////////////////

//! printEvent

//!
//!
//!
////////////////////////////////////////////////////////////////////////////

void CFrParseEvent::printRxEvent(XLfrEvent *pxlFrEvent, LPTSTR sDir)
{
  // write into listbox
  LVITEM  lvi;
  char sValue[100]; 
  CString sTemp="";
  CString sData;

  // create the line
  lvi.mask =  LVIF_IMAGE | LVIF_TEXT;

  sprintf(sValue, "%I64d", pxlFrEvent->timeStampSync);
 
  lvi.iItem = m_plMsg->GetItemCount();
  lvi.iSubItem = 0;
  lvi.pszText = sValue;
  lvi.iImage = 0;
  m_plMsg->InsertItem(&lvi);
  
  sprintf(sValue, "%d", (pxlFrEvent->flagsChip & 0x3));
  printValue(&lvi, 1, sValue);

  sprintf(sValue, "%d", pxlFrEvent->tagData.frRxFrame.slotID);
  printValue(&lvi, 2, sValue);

  printValue(&lvi, 3, sDir);
  
  if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_NULLFRAME) {
    printValue(&lvi, 4, "NULLFRAME");
  }
  else {
    printValue(&lvi, 4, "DATA");
  }
  
  sprintf(sValue, "%d", pxlFrEvent->tagData.frRxFrame.cycleCount);
  printValue(&lvi, 5, sValue);

  // build the data string
  for (int i=0; i<(pxlFrEvent->tagData.frRxFrame.payloadLength*2); i++) {
    sTemp.Format(_T("%02x"),pxlFrEvent->tagData.frRxFrame.data[i]);
    sData = sData + sTemp;
  }
  printValue(&lvi, 6, sData.GetBuffer(sData.GetLength()));
  
  // print the DLC
  sprintf(sValue, "%d", pxlFrEvent->tagData.frRxFrame.payloadLength);
  printValue(&lvi, 7, sValue);

  // print the flags
  sData = "";
  if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_SYNC) {
    sData = sData + "SYNC ";
  }
  if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_STARTUP) {
    sData = sData + "STARTUP ";
  }
  if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_PAYLOAD_PREAMBLE) {
    sData = sData + "PAYLOAD_PREAMBLE ";
  }
  if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_NEW_DATA_TX) {
    sData = sData +  "NEW_DATA ";
  }
  if (pxlFrEvent->tagData.frRxFrame.flags & XL_FR_FRAMEFLAG_DATA_UPDATE_LOST) {
    sData = sData + "DATA_LOST ";
  }
  printValue(&lvi, 8, sData.GetBuffer(sData.GetLength()));

}

////////////////////////////////////////////////////////////////////////////

//! printEvent

//!
//!
//!
////////////////////////////////////////////////////////////////////////////

void CFrParseEvent::printSpyEvent(XLfrEvent *pxlFrEvent)
{
  
  // write into listbox
  LVITEM  lvi;
  char sValue[100]; 
  CString sTemp="";
  CString sData;

  // create the line
  lvi.mask =  LVIF_IMAGE | LVIF_TEXT;

  sprintf(sValue, "%I64d", pxlFrEvent->timeStampSync);
 
  lvi.iItem = m_plMsg->GetItemCount();
  lvi.iSubItem = 0;
  lvi.pszText = sValue;
  lvi.iImage = 0;
  m_plMsg->InsertItem(&lvi);
  
  // print the XL_FR_SPY_CHANNEL_X flags...
  sprintf(sValue, "%d", ((pxlFrEvent->flagsChip & 0x30)>>4));
  printValue(&lvi, 1, sValue);

  sprintf(sValue, "%d", pxlFrEvent->tagData.frSpyFrame.slotID);
  printValue(&lvi, 2, sValue);
  
  printValue(&lvi, 4, "SPY FRAME");

  sprintf(sValue, "%d", pxlFrEvent->tagData.frSpyFrame.cycleCount);
  printValue(&lvi, 5, sValue);

  // build the data string
  for (int i=0; i<(pxlFrEvent->tagData.frSpyFrame.payloadLength*2); i++) {
    sTemp.Format(_T("%02x"),pxlFrEvent->tagData.frSpyFrame.data[i]);
    sData = sData + sTemp;
  }
  printValue(&lvi, 6, sData.GetBuffer(sData.GetLength()));
  
  // print the DLC
  sprintf(sValue, "%d", pxlFrEvent->tagData.frSpyFrame.payloadLength);
  printValue(&lvi, 7, sValue);

  // print the headerflags
  sData = "";
  if (pxlFrEvent->tagData.frSpyFrame.headerFlags & XL_FR_FRAMEFLAG_SYNC) {
    sData = sData + "SYNC ";
  }
  if (pxlFrEvent->tagData.frSpyFrame.headerFlags & XL_FR_FRAMEFLAG_STARTUP) {
    sData = sData + "STARTUP ";
  }
  if (pxlFrEvent->tagData.frSpyFrame.headerFlags & XL_FR_FRAMEFLAG_PAYLOAD_PREAMBLE) {
    sData = sData + "PAYLOAD_PREAMBLE ";
  }
  if (pxlFrEvent->tagData.frSpyFrame.headerFlags & XL_FR_FRAMEFLAG_NULLFRAME) {
    sData = sData +  "NF ";
  }
  printValue(&lvi, 8, sData.GetBuffer(sData.GetLength()));

}


////////////////////////////////////////////////////////////////////////////

//! printStartOfCycleEvent

//!
//!
//!
////////////////////////////////////////////////////////////////////////////

void CFrParseEvent::printStartOfCycleEvent(XLfrEvent *pxlFrEvent)
{
  // write into listbox
  LVITEM  lvi;
  char sValue[100];  

  // create the line
  lvi.mask =  LVIF_IMAGE | LVIF_TEXT;

  sprintf(sValue, "%I64d", pxlFrEvent->timeStampSync);
 
  lvi.iItem = m_plMsg->GetItemCount();
  lvi.iSubItem = 0;
  lvi.pszText = sValue;
  lvi.iImage = 0;
  m_plMsg->InsertItem(&lvi);
  
  printValue(&lvi, 4, "StartCycle");

  sprintf(sValue, "%d", pxlFrEvent->tagData.frStartCycle.cycleCount);
  printValue(&lvi, 5, sValue);


}

////////////////////////////////////////////////////////////////////////////

//! parseEvent

//! 
//!
//!
////////////////////////////////////////////////////////////////////////////

int CFrParseEvent::parseEvent(XLfrEvent *pxlFrEvent)
{
  char tmp[200];
 
  switch (pxlFrEvent->tag) {

    case XL_TIMER: { 
      break;
    }

    case XL_FR_RX_FRAME: {
      printRxEvent(pxlFrEvent, "RX");
      break;
    }
                         
    case XL_FR_TXACK_FRAME: {
      printRxEvent(pxlFrEvent, "TX");
      break;
    }

    case XL_FR_INVALID_FRAME:{
      printEvent(pxlFrEvent, "INVALID-FRAME");
      break;
    }

    case XL_FR_START_CYCLE: {
      printStartOfCycleEvent(pxlFrEvent);
      break;
    }

    case XL_FR_STATUS: {
      printEvent(pxlFrEvent, "STATUS");
      break;
    }
    
    case XL_FR_ERROR: {
      printEvent(pxlFrEvent, "ERROR");
      break;
    }

    case XL_FR_SPY_FRAME: {
      printSpyEvent(pxlFrEvent);
      break;
    }
   
    default: {
      sprintf(tmp, "unknown event, tag: 0x%x", pxlFrEvent->tag);
      XLDEBUG(DEBUG_ADV, tmp);
    }
  }

  if (pxlFrEvent->flagsChip & XL_FR_QUEUE_OVERFLOW) {
    printEvent(pxlFrEvent, "++ QUEUE OVERFLOW ++");
  }
 
  return 0;
}
