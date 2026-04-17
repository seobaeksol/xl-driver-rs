/*-------------------------------------------------------------------------------------------
| File        : xlFrTxFrameDlg.h
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2013 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once
#include "afxwin.h"

#include "xlFrFunctions.h"

// CTxFrameDlg dialog

class CTxFrameDlg : public CDialog
{
  DECLARE_DYNAMIC(CTxFrameDlg)

public:
  CTxFrameDlg(CWnd* pParent = NULL);   // standard constructor
  virtual ~CTxFrameDlg();

// Dialog Data
  enum { IDD = IDD_TXFRAME };

protected:
  	// Generated message map functions
  virtual BOOL OnInitDialog();
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  afx_msg void OnBnClickedBtnAdd128frames();

  DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedAddtxframe();
  CFrFunctions    *m_pFr;
  unsigned int    m_nChan;
  CComboBox       m_lChannel;
  CComboBox       m_lRepetition;
  CString         m_eSlotId;
  CString         m_eOffset;
public:
  BOOL m_cTxAkn;
public:
  BOOL m_cPayloadInc;
public:
  CString m_eData;
public:
  CButton m_ccPayloadInc;
public:
  CComboBox m_lTxMode;
};
