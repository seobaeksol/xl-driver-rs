/*-------------------------------------------------------------------------------------------
| File        : xlFlexDemoDlg.h
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2013 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once
#include "xlFrFunctions.h"
#include "afxwin.h"
#include "afxcmn.h"

// CxlFlexDemoDlg dialog
class CxlFlexDemoDlg : public CDialog
{
// Construction
public:
  CxlFlexDemoDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
  enum { IDD = IDD_XLFLEXDEMO_DIALOG };

  protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

  // Timer ID for asynchronous output panel update
  static const UINT_PTR TIMER_ID_OUTPUT_PANEL_UPDATE = 100;
  // Timer delay for asynchronous output panel update [ms]
  static const UINT     TIMER_DELAY_OUTPUT_PANEL_UPDATE = 50;

// Implementation
protected:
	HICON           m_hIcon;
  CFrFunctions    m_Fr;
  BOOL            m_bBtnActivate;
  BOOL            m_bBtnShowTrace;
  BOOL            m_bBtnCopyTrace;

  // Generated message map functions
  virtual BOOL OnInitDialog();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  afx_msg void OnTimer(UINT_PTR nIDEvent);
  afx_msg void OnLvnEndScrollMsglist(NMHDR *pNMHDR, LRESULT *pResult);
  DECLARE_MESSAGE_MAP()
public:
  CListBox m_lDevice;
public:
  afx_msg void OnBnClickedActivate();
public:
  CButton m_btnActivate;
public:
  CListCtrl m_lMsg;
public:
  afx_msg void OnBnClickedClear();
public:
  afx_msg void OnCbnSelchangeCombo1();
public:
  CComboBox m_lRepetition;
public:
  afx_msg void OnBnClickedAddtxframe();
public:
  CString m_eOffset;
public:
  CString m_eSlotId;
public:
  CButton m_bShowTrace;
public:
  afx_msg void OnBnClickedShow();
  afx_msg void OnBnClickedCopy();
public:
  afx_msg void OnBnClickedOk();
public:
  afx_msg void OnBnClickedAbout();
public:
  CComboBox m_lChannel;
public:
  CString m_eSlotEray;
public:
  CString m_eSlotCold;
public:
  BOOL m_cColdCC;
public:
  BOOL m_cEray;
public:
  afx_msg void OnBnClickedEray();
public:
  CEdit m_ebSlotEray;
public:
  CEdit m_ebSlotCold;
public:
  afx_msg void OnBnClickedColdcc();
public:
  afx_msg void OnBnClickedSpy();
public:
  BOOL m_cSpy;
};
