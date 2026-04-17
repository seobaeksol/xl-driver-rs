#pragma once

#include "vxlapi.h"
#include ".\xlmostparam.h"

class CMOSTParseEvent
{
public:
  //CMOSTParseEvent(CListBox *pOutput);
  CMOSTParseEvent();
  ~CMOSTParseEvent(void);
  CListBox *m_plRxBox;  
  int parseEvent  (XLmostEvent *pxlMostEvent);

private:
  
  void printEvent(CString strEvent, int channelIndex);
  void printEvent(CString strEvent);
};
