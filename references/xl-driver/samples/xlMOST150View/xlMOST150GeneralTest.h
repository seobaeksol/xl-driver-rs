#pragma once

#include "xlMOST150Functions.h"

// CMOST150GeneralTest dialog

class CMOST150GeneralTest : public CDialog
{
  DECLARE_DYNAMIC(CMOST150GeneralTest)

public:
  CMOST150GeneralTest(CWnd* pParent = NULL);   // standard constructor
  virtual ~CMOST150GeneralTest();

  int             m_nChan;
  CMOST150Functions  *m_pMOST150;
// Dialog Data
  enum { IDD = IDD_GEN_TEST };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  BOOL OnInitDialog();
  DECLARE_MESSAGE_MAP()
public:
  afx_msg void OnBnClickedGenLight(void);
  afx_msg void OnBnClickedGenLock(void);
  CString m_esLightError;
  CString m_esLockError;
};
