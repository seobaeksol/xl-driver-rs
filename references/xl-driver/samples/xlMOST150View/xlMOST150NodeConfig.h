#pragma once

#include "xlMOST150Functions.h"
#include "xlGeneral.h"
#include "afxwin.h"
// MOST150NodeConfig dialog

class CMOST150NodeConfig : public CDialog
{
  DECLARE_DYNAMIC(CMOST150NodeConfig)

public:
  CMOST150NodeConfig(CWnd* pParent = NULL);   // standard constructor
  virtual ~CMOST150NodeConfig();

// Dialog Data
  enum { IDD = IDD_NODE_CONFIG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  BOOL OnInitDialog();

  DECLARE_MESSAGE_MAP()
public:

  int m_nChan;
  CMOST150Functions *m_pMOST150;
 
  CButton m_btnTimingMode;
  afx_msg void OnBnClickedSetParameter();
private:
  CComboBox m_cFrequency;
public:
//  afx_msg void OnInitMenu(CMenu* pMenu);
private:
  CButton  m_rDeviceModeSlave;
  CButton  m_rDeviceModeMaster;
  CString  m_strNodeAdr;
  CGeneral m_General;
public:
  CString m_esGroupAdr;
  CString m_esNodeAdr;
};
