// xlMOSTViewDlg.h : Headerdatei
//

#pragma once
#include "afxwin.h"

#include "xlMOSTFunctions.h"
#include "xlMOSTNodeConfig.h"
#include "xlMOSTGeneralTest.h"
#include "xlGeneral.h"

// CxlMOSTViewDlg Dialogfeld
class CxlMOSTViewDlg : public CDialog
{
// Konstruktion
public:
	CxlMOSTViewDlg(CWnd* pParent = NULL);	// Standardkonstruktor
  ~CxlMOSTViewDlg();

// Dialogfelddaten
	enum { IDD = IDD_XLMOSTVIEW_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV-Unterstützung


// Implementierung
protected:
	HICON m_hIcon;
  CGeneral         m_General;
  CMOSTFunctions   m_MOST;
  CMOSTNodeConfig  m_MOSTNodeConfig;
  CMOSTGeneralTest m_MOSTGeneralTest;

  unsigned int     m_ToggleLight[XL_CONFIG_MAX_CHANNELS];
  BOOL             m_bStream;

	// Generierte Funktionen für die Meldungstabellen
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
  afx_msg void OnBnClickedLight();
  CButton m_btnToggleLight;
  afx_msg void OnLbnSelchangeList1();
  CListBox m_lDevice;
  afx_msg void OnLbnDblclkDevice();
  afx_msg void OnBnClickedClear();
  afx_msg void OnBnClickedTxCtrl();
  afx_msg void OnBnClickedTest();
  afx_msg void OnBnClickedGetInfo();
  CButton m_btnActivate;
  CButton m_btnDeactivate;
  afx_msg void OnBnClickedNodeConfig();
  afx_msg void OnBnClickedGenTest();
  afx_msg void OnBnClickedTwinkel();
  afx_msg void OnBnClickedAbout();
  afx_msg void OnEnChangeGroupAdr();
  CString m_esTargetAdr;
  CString m_esSourceAdr;
  CButton m_btnStream;
  afx_msg void OnBnClickedStreaming();
  CButton m_btnTwinkel;
  CButton n_btnNodeConfig;
  CButton m_btnGeneralTest;
  CButton m_btnGetInfo;
  CButton m_btnSendCtrlFrame;
};
