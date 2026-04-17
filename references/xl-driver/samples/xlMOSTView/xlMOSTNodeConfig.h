#pragma once

#include "xlMOSTFunctions.h"
#include "xlGeneral.h"
#include "afxwin.h"
// MOSTNodeConfig dialog

class CMOSTNodeConfig : public CDialog
{
	DECLARE_DYNAMIC(CMOSTNodeConfig)

public:
	CMOSTNodeConfig(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMOSTNodeConfig();

// Dialog Data
	enum { IDD = IDD_NODE_CONFIG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:

  int m_nChan;
  CMOSTFunctions *m_pMOST;
 
  CButton m_btnTimingMode;
  afx_msg void OnBnClickedSetParameter();
private:
  CComboBox m_cFrequency;
public:
//  afx_msg void OnInitMenu(CMenu* pMenu);
private:
  CButton  m_rSlaveTiming;
  CButton  m_rMasterTiming;
  CString  m_strNodeAdr;
  CGeneral m_General;
public:
  CString m_esGroupAdr;
  CString m_esNodeAdr;
  CComboBox m_cBypass;
};
