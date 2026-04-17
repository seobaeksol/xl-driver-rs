/*-------------------------------------------------------------------------------------------
| File        : xlMOSTFunctions.h
| Project     : Vector MOST Example 
|
| Description : Shows the basic MOST functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2005 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once

#include "vxlapi.h"
#include ".\xlmostparseevent.h"
#include ".\xlmoststreaming.h"
#include ".\xlmostparam.h"

#define CHAN01  0
#define CHAN02  1

class CMOSTFunctions : public virtual CMOSTParseEvent, public virtual CMOSTStreaming
{
public:
  CMOSTFunctions(void);
  ~CMOSTFunctions(void);

  CListBox *m_pListBox;    
  CListBox *m_plDevice;

  XLstatus MOSTInit();
  XLstatus MOSTClose();
  XLstatus MOSTActivate           (unsigned int nChan);
  XLstatus MOSTDeActivate         ();
  XLstatus MOSTCtrlTransmit       (unsigned short nTargetAdr);
  XLstatus MOSTToggleLight        (unsigned int nToggleLight);
  XLstatus MOSTSetupNode          (unsigned int nTimingMode, unsigned int nFrequency, 
                                   unsigned short nNodeAdr, unsigned short nGroupAdr, unsigned char nBypass);
  XLstatus MOSTGetInfo            ();
  XLstatus MOSTTwinklePowerLED    ();
  XLstatus MOSTGenerateLightError (unsigned short nRepeat);
  XLstatus MOSTGenerateLockError  (unsigned short nRepeat);
  XLstatus MOSTInitStreaming      ();

private:
  unsigned int      m_nChannelCount;
  int               m_channel;

  XLstatus mostClose();
  XLstatus mostActivate           ();
  XLstatus mostSetApplConfig      (XLdriverConfig xlDrvConfig, unsigned int appChannel);
  XLstatus mostGetChannelMask();
  XLstatus mostInit();
  XLstatus mostCreateRxThread();
  BOOL     mostCheckChannelMask   (XLaccess xlChannelMask);
};

typedef struct {
  XLportHandle    xlPortHandle; 
  HANDLE          hMsgEvent;
  CListBox        *pOutput;
  CMOSTFunctions  *pMostFunc;
} TStruct;

DWORD     WINAPI RxThread( PVOID par );