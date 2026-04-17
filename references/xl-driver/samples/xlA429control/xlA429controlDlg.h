// xlA429controlDlg.h : header file
//

#include "xlA429Functions.h"

#if !defined(AFX_XLA429CONTROLDLG_H__6B0850D5_5DFB_4F90_9475_A6C598BFB702__INCLUDED_)
#define AFX_XLA429CONTROLDLG_H__6B0850D5_5DFB_4F90_9475_A6C598BFB702__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CXlA429controlDlg dialog

class CXlA429controlDlg : public CDialog
{
// Construction
public:
	CXlA429controlDlg(CWnd* pParent = NULL);	// standard constructor

  CA429Functions m_A429;

// Dialog Data
	//{{AFX_DATA(CXlA429controlDlg)
	enum { IDD = IDD_XLA429CONTROL_DIALOG };
	CListBox	m_Hardware;
	CButton	  m_btnSend;
	CButton	  m_btnOffBus;
	CButton	  m_btnOnBus;
	CComboBox	m_TxParity;
	CListBox	m_Output;
  CString   m_eTxMinGap;
  CString   m_eTxBitrate;
  CComboBox	m_RxParity;
  CString   m_eRxMinGap;
  BOOL	    m_autoBaudrate;
  CString   m_eRxBitrate;
  CString	  m_eLabel;
	CString   m_eData;
  CComboBox m_txMsgFlag;
  CString   m_eTxMsgCycleTime;
  CComboBox m_txMsgParity;
  CString	  m_eTxMsgGap;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXlA429controlDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CXlA429controlDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnOnbus();
	afx_msg void OnOffbus();
	afx_msg void OnSend();
	afx_msg void OnClear();
	afx_msg void OnAbout();
	afx_msg void OnReset();
	afx_msg void OnLicInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XLA429CONTROLDLG_H__6B0850D5_5DFB_4F90_9475_A6C598BFB702__INCLUDED_)
