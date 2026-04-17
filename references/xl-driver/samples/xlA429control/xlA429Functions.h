/*----------------------------------------------------------------------------
| File        : xlA429Functions.h
| Project     : Vector A429 Example 
|
| Description : shows the basic A429 functionality for the XL Driver Library
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2004 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#if !defined(AFX_XLA429FUNCTIONS_H__48DFA4A9_72B2_48FE_80D5_D318A80C4B3A__INCLUDED_)
#define AFX_XLA429FUNCTIONS_H__48DFA4A9_72B2_48FE_80D5_D318A80C4B3A__INCLUDED_

#include "vxlapi.h"

#define CHAN01 0
#define CHAN02 1

typedef struct {
    XLportHandle xlPortHandle; 
    HANDLE       hMsgEvent;
    CListBox    *pOutput;
} TStruct;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CA429Functions  
{
public:
	CA429Functions();
	virtual ~CA429Functions();

  XLstatus A429Init();
  XLstatus A429GoOnBus(unsigned int txParity, unsigned int txMinGap, unsigned int txBitrate,
                       unsigned int rxParity, unsigned int rxMinGap, unsigned int rxAutoBaudrate, unsigned int rxBitrate);
  XLstatus A429GoOffBus();
  XLstatus A429Send(XL_A429_MSG_TX xlA429MsgTx, int channel);
  XLstatus ShowLicenses();
  
  CListBox *m_pOutput;
  CListBox *m_pHardware;

private:
  XLstatus         a429GetChannelMask();
  XLstatus         a429Init();
  XLstatus         a429CreateRxThread();

  XLaccess         m_xlChannelMask[2];        //!< we support only two channels
  XLportHandle     m_xlPortHandle;            //!< and one port
  XLaccess         m_xlChannelMask_both;

  HANDLE           m_hThread;
  XLhandle         m_hMsgEvent;
  int              m_bInitDone;
};

DWORD     WINAPI RxThread( PVOID par );


#endif // !defined(AFX_XLA429FUNCTIONS_H__48DFA4A9_72B2_48FE_80D5_D318A80C4B3A__INCLUDED_)
