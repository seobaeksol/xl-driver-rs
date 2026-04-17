// xlCANcontrolDlg.h : header file
//

#include "xlCANFunctions.h"

#if !defined(AFX_XLCANCONTROLDLG_H__6B0850D5_5DFB_4F90_9475_A6C598BFB702__INCLUDED_)
#define AFX_XLCANCONTROLDLG_H__6B0850D5_5DFB_4F90_9475_A6C598BFB702__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CXlCANcontrolDlg dialog

class CXlCANcontrolDlg : public CDialog
{
// Construction
public:
  CXlCANcontrolDlg(CWnd* pParent = NULL);	// standard constructor

  CCANFunctions m_CAN;

// Dialog Data
  //{{AFX_DATA(CXlCANcontrolDlg)
  enum { IDD = IDD_XLCANCONTROL_DIALOG };
  CListBox m_Hardware;
  CComboBox m_Channel;
  CButton m_btnSend;
  CButton m_btnOffBus;
  CButton m_btnOnBus;
  CButton m_btnLicInfo;
  CComboBox m_Baudrate;
  CListBox m_Output;
  CString m_eD00;
  CString m_eD01;
  CString m_eD02;
  CString m_eD03;
  CString m_eD04;
  CString m_eD05;
  CString m_eD06;
  CString m_eD07;
  CString m_eDLC;
  CString m_eID;
  BOOL m_ExtID;
  CString m_eFilterFrom;
  CString m_eFilterTo;
  //}}AFX_DATA

  // ClassWizard generated virtual function overrides
  //{{AFX_VIRTUAL(CXlCANcontrolDlg)
  protected:
  virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
  //}}AFX_VIRTUAL

// Implementation
protected:
  HICON m_hIcon;

  // Generated message map functions
  //{{AFX_MSG(CXlCANcontrolDlg)
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
  afx_msg void OnSetfilter();
  //}}AFX_MSG
  DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XLCANCONTROLDLG_H__6B0850D5_5DFB_4F90_9475_A6C598BFB702__INCLUDED_)
