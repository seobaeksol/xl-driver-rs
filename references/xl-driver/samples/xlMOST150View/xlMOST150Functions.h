/*-------------------------------------------------------------------------------------------
| File        : xlMOSTFunctions.h
| Project     : Vector MOST Example 
|
| Description : Shows the basic MOST150 functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2011 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once

#include "vxlapi.h"
#include "xlmost150parseevent.h"
#include "xlmost150streaming.h"

#define CHAN01  0
#define CHAN02  1

class CMOST150Functions : public virtual CMOST150ParseEvent, public virtual CMOST150Streaming
{
public:
  CMOST150Functions(void);
  ~CMOST150Functions(void);

  CListBox *m_pListBox;    
  CListBox *m_plDevice;

  XLstatus MOST150Init               (void);
  XLstatus MOST150Close              (void);
  XLstatus MOST150Activate           (unsigned int nChan);
  XLstatus MOST150DeActivate         (void);
  XLstatus MOST150CtrlTransmit       (unsigned short nTargetAdr);
  XLstatus MOST150AsyncTransmit      (unsigned short nTargetAdr);
  XLstatus MOST150SetupNode          (unsigned int nDeviceMode, 
                                      unsigned int nFrequency, 
                                      unsigned short nNodeAdr, 
                                      unsigned short nGroupAdr);
  XLstatus MOST150NwStartup          (void);
  XLstatus MOST150NwShutdown         (void);
  XLstatus MOST150GetInfo            (void);
  XLstatus MOST150TwinklePowerLED    (void);
  XLstatus MOST150GenerateLightError (unsigned short nRepeat);
  XLstatus MOST150GenerateLockError  (unsigned short nRepeat);

  virtual void parseEvent (XLmost150event *pxlMost150Event);

private:
  unsigned int      m_nChannelCount;
  int               m_channel;
  bool              m_DoInitialStartup;

  void     most150BuildCtrlMsgHeader (unsigned char fblockId, 
                                      unsigned char instId,
                                      unsigned char fktID, 
                                      unsigned char opType,
                                      unsigned char telId, 
                                      unsigned short telLen,
                                      XLmost150CtrlTxMsg* pCtrlMsg);
  XLstatus most150ClosePort          (void);
  XLstatus most150Startup            (void);
  XLstatus most150SetApplConfig      (XLdriverConfig xlDrvConfig, unsigned int appChannel);
  XLstatus most150GetChannelMask     (void);
  XLstatus most150OpenPort           (void);
  XLstatus most150CreateRxThread     (void);
  BOOL     most150CheckChannelMask   (XLaccess xlChannelMask);
};

typedef struct {
  XLportHandle       xlPortHandle; 
  HANDLE             hMsgEvent;
  CListBox           *pOutput;
  CMOST150Functions  *pMost150Func;
} TStruct;

DWORD     WINAPI RxThread( PVOID par );