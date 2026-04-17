/*-------------------------------------------------------------------------------------------
| File        : xlFrFunctions.h
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once

#include "vxlapi.h"

#define CHAN01                  0
#define CHAN02                  1
#define RX_FIFO_SIZE        32768 
#define USE_STARTUP_ERAY     0x01
#define USE_STARTUP_COLD     0x02
#define USE_SPY_MODE         0x04
#define STATIC_PAYLOAD_LENGTH   2 // gPayloadLengthStatic

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
};

typedef struct {
  XLportHandle xlPortHandle; 
  HANDLE       hMsgEvent;
  CListCtrl    *plMsg;
  BOOL         bShowTrace;
} TStruct;

class CFrFunctions
{
public:
  CFrFunctions(void);
  ~CFrFunctions(void);

  CListBox  *m_plDevice;
  CListCtrl *m_plMsg;
  BOOL      m_lic;

  XLstatus FrInit();
  void     FrToggleTrace        (unsigned int   nChan, 
                                 BOOL           bShowTrace);
  XLstatus FrAddTxFrame         (unsigned int   nChan,
                                 unsigned short flagsChip,
                                 unsigned short txFlags,
                                 unsigned char  txMode,
                                 unsigned char  payloadInc,
                                 unsigned short slotId,
                                 unsigned char  offset,
                                 unsigned char  repetition,
                                 unsigned char  payloadLength,
                                 unsigned char  *data);
  XLstatus FrActivate           (unsigned int   nChan, 
                                 unsigned short erayId, 
                                 unsigned short coldId,
                                 unsigned char  flags);
  XLstatus FrDeactivate         (unsigned int   nChan);
  
  CNodeParam m_NodeParam[XL_CONFIG_MAX_CHANNELS];
  
private:
  unsigned int                    m_nChannelCount;

  XLstatus frGetChannelMask();
  XLstatus frInit               (unsigned int   nChan);
  XLstatus frSetConfig          (unsigned int   nChan);
  XLstatus frStartUpSync        (unsigned int   nChan, 
                                 unsigned short erayId, 
                                 unsigned short coldId,
                                 unsigned char  flags);
  XLstatus frCreateRxThread     (unsigned int   nChan);
};

// global thread
DWORD     WINAPI RxThread( PVOID par );