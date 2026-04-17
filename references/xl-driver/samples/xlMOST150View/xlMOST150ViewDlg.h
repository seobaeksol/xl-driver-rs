// xlMOST150ViewDlg.h : Headerdatei
//

#pragma once
#include "afxwin.h"

#include "xlMOST150Functions.h"
#include "xlMOST150NodeConfig.h"
#include "xlMOST150GeneralTest.h"
#include "xlGeneral.h"

class CxlMOST150ViewDlg : public CDialog
{
public:
  CxlMOST150ViewDlg(CWnd* pParent = NULL);
  ~CxlMOST150ViewDlg();
  
  enum { IDD = IDD_XLMOST150VIEW_DIALOG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);

protected:
  HICON m_hIcon;
  CGeneral            m_General;
  CMOST150Functions   m_MOST150;
  CMOST150NodeConfig  m_MOST150NodeConfig;
  CMOST150GeneralTest m_MOST150GeneralTest;

  unsigned int        m_ToggleLight[XL_CONFIG_MAX_CHANNELS];
  BOOL                m_bStream;

  virtual BOOL OnInitDialog();
  afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
  afx_msg void OnPaint();
  afx_msg HCURSOR OnQueryDragIcon();
  DECLARE_MESSAGE_MAP()
public:
  CListBox m_ListBox;
  afx_msg void OnBnClickedWiznext();
  afx_msg void OnBnClickedActivate();
  afx_msg void OnBnClickedDeactivate();
  afx_msg void OnBnClickedStartup();
  CButton m_btnNwStartup;
  afx_msg void OnBnClickedShutdown();
  CButton m_btnNwShutdown;
  afx_msg void OnBnClickedStreaming();
  CButton m_btnStreaming;
  afx_msg void OnLbnSelchangeList1();
  CListBox m_lDevice;
  afx_msg void OnLbnDblclkDevice();
  afx_msg void OnBnClickedClear();
  afx_msg void OnBnClickedTxCtrl();
  afx_msg void OnBnClickedSendAsyncTx();
  afx_msg void OnBnClickedTest();
  afx_msg void OnBnClickedGetInfo();
  CButton m_btnActivate;
  CButton m_btnDeactivate;
  afx_msg void OnBnClickedNodeConfig();
  afx_msg void OnBnClickedGenTest();
  afx_msg void OnBnClickedTwinkle();
  afx_msg void OnBnClickedAbout();
  afx_msg void OnEnChangeGroupAdr();
  CString m_esCtrlTargetAdr;
  CString m_esAsyncTargetAdr;
  CButton m_btnTwinkel;
  CButton n_btnNodeConfig;
  CButton m_btnGeneralTest;
  CButton m_btnGetInfo;
  CButton m_btnSendCtrlFrame;
  CButton m_btnSendAsyncFrame;
};
