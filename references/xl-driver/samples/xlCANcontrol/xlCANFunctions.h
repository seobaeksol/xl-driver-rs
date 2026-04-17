/*----------------------------------------------------------------------------
| File        : xlCANFunctions.h
| Project     : Vector CAN Example 
|
| Description : shows the basic CAN functionality for the XL Driver Library
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2004 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#if !defined(AFX_XLCANFUNCTIONS_H__48DFA4A9_72B2_48FE_80D5_D318A80C4B3A__INCLUDED_)
#define AFX_XLCANFUNCTIONS_H__48DFA4A9_72B2_48FE_80D5_D318A80C4B3A__INCLUDED_

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

class CCANFunctions  
{
public:
  CCANFunctions();
  virtual ~CCANFunctions();

  XLstatus CANInit();
  XLstatus CANGoOnBus(unsigned long baudrate);
  XLstatus CANGoOffBus();
  XLstatus CANSend(XLevent xlEvent, int channel);
  XLstatus CANResetFilter();
  XLstatus CANSetFilter(unsigned long first_id, unsigned long last_id);
  const char *GetChannelName(int channel);
  void     AppendLicenseInfo(CString& cstr, int channel);
  
  CListBox *m_pOutput;

private:
  XLstatus         canGetChannelMask();
  XLstatus         canInit();
  XLstatus         canCreateRxThread();

  XLaccess         m_xlChannelMask[2];        //!< we support only two channels
  int              m_xlChannelIndex[2];       //!< we support only two channels
  XLportHandle     m_xlPortHandle;            //!< and one port
  XLaccess         m_xlChannelMask_both;
  XLdriverConfig   m_xlDrvConfig;

  HANDLE           m_hThread;
  XLhandle         m_hMsgEvent;
  int              m_bInitDone;
};

DWORD     WINAPI RxThread( PVOID par );


#endif // !defined(AFX_XLCANFUNCTIONS_H__48DFA4A9_72B2_48FE_80D5_D318A80C4B3A__INCLUDED_)
