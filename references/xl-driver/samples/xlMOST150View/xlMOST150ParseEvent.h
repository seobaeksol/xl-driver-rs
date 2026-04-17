#pragma once

#include "vxlapi.h"
#include "xlmost150param.h"

class CMOST150ParseEvent
{
public:
  CMOST150ParseEvent     (void);
  ~CMOST150ParseEvent    (void);
  CListBox *m_plRxBox;  
  virtual void parseEvent (XLmost150event *pxlMost150Event);

private:
  
  void printEvent(CString strEvent);
  unsigned int bitCount(unsigned int mask);
};
