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
  unsigned int    nDeviceMode;
  unsigned int    nFrequency;
  unsigned int    *pStreamHandle;
};

class CMOST150Param 
{   
public:
  CMOST150Param(void){}
  ~CMOST150Param(void) {}

  CNodeParam m_NodeParam[XL_CONFIG_MAX_CHANNELS];
};

