#pragma once

#include "xlDAIOdemo.h"
#include "stdafx.h"

#include "vxlapi.h"

#include <array>

class CXlDAIOdemoDlg : public CDialog
{
public:
	explicit CXlDAIOdemoDlg(CWnd* pParent = NULL);

  void AddMessage   (CString msg);

// Dialogfelddaten
	//{{AFX_DATA(CXlDAIOdemoDlg)
	enum { IDD = IDD_XLDAIODEMO_DIALOG };

  std::array<CEdit,4> mAnalogCtrls;

	CListBox	m_Screen;

	CSliderCtrl	m_PwmValueCtrl;

	std::array<CButton, 4> mDigitalOuts;

	CListBox	m_DigitalStateList;

	CButton	m_StopCtrl;

	CButton	m_StartCtrl;

	std::array<UINT, 4> mAnalogs = {0u, 0u, 0u, 0u};

	int     m_MeasurementFrequency{0};
  int     m_PwmValue{0};
  int     m_PwmFrequency{0};

  CString m_SelectedChannelName{_T("No channel activated")};
	//}}AFX_DATA


private:
  HICON m_hIcon;

	XLaccess gChannelMask{0};  // mask of selected channel

	bool OnFirstStart();

	//{{AFX_VIRTUAL(CXlDAIOdemoDlg)
	void DoDataExchange(CDataExchange* pDX) final;	// DDX/DDV-Unterstützung
	//}}AFX_VIRTUAL

	// Generierte Message-Map-Funktionen
	//{{AFX_MSG(CXlDAIOdemoDlg)
	BOOL OnInitDialog() final;
  void OnCancel() final;

	void OnSetDigitalGeneric(unsigned int portNumber);

	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnStart();
	afx_msg void OnStop();
	afx_msg void OnSetAnalog();
	afx_msg void OnSetDigital1();
	afx_msg void OnSetDigital2();
	afx_msg void OnSetDigital3();
	afx_msg void OnSetDigital4();
	afx_msg void OnMeasureNow();
	afx_msg void OnSetConfiguration();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnReleasedcaptureSlider1(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.