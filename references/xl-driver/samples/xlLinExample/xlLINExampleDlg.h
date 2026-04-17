// LINExampleDlg.h : header file
//

#if !defined(AFX_LINEXAMPLEDLG_H__B878A7CA_7BC7_4DC7_9411_67353BAA3CC4__INCLUDED_)
#define AFX_LINEXAMPLEDLG_H__B878A7CA_7BC7_4DC7_9411_67353BAA3CC4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "xlLINFunctions.h"

/////////////////////////////////////////////////////////////////////////////
// CLINExampleDlg dialog

class CLINExampleDlg : public CDialog
{
// Construction
public:
	CLINExampleDlg(CWnd* pParent = NULL);	// standard constructor

  CLINFunctions m_LIN;

// Dialog Data
	//{{AFX_DATA(CLINExampleDlg)
	enum { IDD = IDD_LINEXAMPLE_DIALOG };
	CEdit	m_editLINID;
	CEdit	m_editIDMaster;
	CButton	m_btnClose;
	CButton	m_btnInit;
	CButton	m_btnSendReq;
	CListBox	m_StatusBox;
	CListBox	m_RXBox;
	CString	m_LINID;
	CString	m_IDMaster;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLINExampleDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

  BYTE    m_data;

	// Generated message map functions
	//{{AFX_MSG(CLINExampleDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnInit();
	afx_msg void OnSendmasterreq();
	afx_msg void OnClose();
	afx_msg void OnClear();
	afx_msg void OnAbout();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINEXAMPLEDLG_H__B878A7CA_7BC7_4DC7_9411_67353BAA3CC4__INCLUDED_)
