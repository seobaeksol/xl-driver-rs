#pragma once

#include "vxlapi.h"

class CNodeParam 
{   
public:
  CNodeParam(void){
    xlPortHandle = XL_INVALID_PORTHANDLE;
  }
  ~CNodeParam(void) {
  }

  XLaccess        xlChannelMask;   
  XLportHandle    xlPortHandle;       
  HANDLE          hThread;
  XLhandle        hMsgEvent; 
  unsigned short  nNodeAdr;
  unsigned short  nGroupAdr; 
  unsigned char   nBypass;
  unsigned int    nTimingMode;
  unsigned int    nFrequency;
  unsigned int    *pStreamHandle;
};

class CMOSTParam 
{   
public:
  CMOSTParam(void){}
  ~CMOSTParam(void) {}

  CNodeParam m_NodeParam[XL_CONFIG_MAX_CHANNELS];
};

