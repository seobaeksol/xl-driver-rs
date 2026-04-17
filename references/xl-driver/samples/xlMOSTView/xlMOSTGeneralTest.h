#pragma once

#include "xlMOSTFunctions.h"

// CMOSTGeneralTest dialog

class CMOSTGeneralTest : public CDialog
{
	DECLARE_DYNAMIC(CMOSTGeneralTest)

public:
	CMOSTGeneralTest(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMOSTGeneralTest();

  int             m_nChan;
  CMOSTFunctions  *m_pMOST;
// Dialog Data
	enum { IDD = IDD_GEN_TEST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  BOOL OnInitDialog();
	DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedGenLight();
  afx_msg void OnBnClickedGenLock();
  CString m_esLightError;
  CString m_esLockError;
  afx_msg void OnBnClickedAsync();
  afx_msg void OnBnClickedCtrl();
};
