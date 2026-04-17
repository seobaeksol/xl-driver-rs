/*-------------------------------------------------------------------------------------------
| File        : xlFrParseEvent.h
| Project     : Vector FlexRay Example
|
| Description : Parse the incoming FlexRay events and print it to the trace window
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once

#include "vxlapi.h"

class CFrParseEvent
{
public:
  CFrParseEvent(CListCtrl *plMsg);
  ~CFrParseEvent(void);

  int parseEvent(XLfrEvent *pxlFrEvent);

private:
  CListCtrl *m_plMsg;  
  void printRxEvent(XLfrEvent *pxlFrEvent, LPTSTR sDir);
  void printSpyEvent(XLfrEvent *pxlFrEvent);
  void printStartOfCycleEvent(XLfrEvent *pxlFrEvent);
  void printValue(LVITEM *pLvi, unsigned int item, LPTSTR sValue);
  void printEvent(XLfrEvent *pxlFrEvent, LPTSTR sType);
};
