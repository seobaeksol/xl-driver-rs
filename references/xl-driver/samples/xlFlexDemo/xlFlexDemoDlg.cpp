/*-------------------------------------------------------------------------------------------
| File        : xlFlexDemoDlg.cpp
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2013 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlFlexDemo.h"
#include "xlFlexDemoDlg.h"
#include "xlFrTxFrameDlg.h"
#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CxlFlexDemoDlg dialog




CxlFlexDemoDlg::CxlFlexDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CxlFlexDemoDlg::IDD, pParent)
  , m_bBtnActivate(FALSE)
  , m_bBtnShowTrace(TRUE)
  , m_eSlotEray(_T(""))
  , m_eSlotCold(_T(""))
  , m_cColdCC(TRUE)
  , m_cEray(TRUE)
  , m_cSpy(FALSE)
{
  
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CxlFlexDemoDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_DEVICE, m_lDevice);
  DDX_Control(pDX, IDC_ACTIVATE, m_btnActivate);
  DDX_Control(pDX, IDC_MSGLIST, m_lMsg);
  DDX_Control(pDX, IDC_SHOW, m_bShowTrace);
  DDX_Text(pDX, IDC_ERAYID, m_eSlotEray);
  DDX_Text(pDX, IDC_COLDID, m_eSlotCold);
  DDX_Check(pDX, IDC_COLDCC, m_cColdCC);
  DDX_Check(pDX, IDC_ERAY, m_cEray);
  DDX_Control(pDX, IDC_ERAYID, m_ebSlotEray);
  DDX_Control(pDX, IDC_COLDID, m_ebSlotCold);
  DDX_Check(pDX, IDC_SPY, m_cSpy);
}

BEGIN_MESSAGE_MAP(CxlFlexDemoDlg, CDialog)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_TIMER()
  ON_WM_QUERYDRAGICON()
  //}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_ACTIVATE, &CxlFlexDemoDlg::OnBnClickedActivate)
  ON_BN_CLICKED(IDC_CLEAR, &CxlFlexDemoDlg::OnBnClickedClear)
  ON_BN_CLICKED(IDC_ADDTXFRAME, &CxlFlexDemoDlg::OnBnClickedAddtxframe)
  ON_BN_CLICKED(IDC_SHOW, &CxlFlexDemoDlg::OnBnClickedShow)
  ON_BN_CLICKED(IDC_BTN_COPY, &CxlFlexDemoDlg::OnBnClickedCopy)
  ON_BN_CLICKED(IDOK, &CxlFlexDemoDlg::OnBnClickedOk)
  ON_BN_CLICKED(ID_ABOUT, &CxlFlexDemoDlg::OnBnClickedAbout)
  ON_BN_CLICKED(IDC_ERAY, &CxlFlexDemoDlg::OnBnClickedEray)
  ON_BN_CLICKED(IDC_COLDCC, &CxlFlexDemoDlg::OnBnClickedColdcc)
  ON_BN_CLICKED(IDC_SPY, &CxlFlexDemoDlg::OnBnClickedSpy)
  ON_NOTIFY(LVN_ENDSCROLL, IDC_MSGLIST, &CxlFlexDemoDlg::OnLvnEndScrollMsglist)
END_MESSAGE_MAP()


// CxlFlexDemoDlg message handlers

BOOL CxlFlexDemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

  // -----------------------------
	// init the GUI
  // -----------------------------
  int   nColInterval;
  CRect rect;
  //GetClientRect(&rect);

  m_lMsg.GetClientRect(&rect);
  nColInterval = rect.Width()/20;
  m_lMsg.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_REPORT);  // Allow selecting in all columns of a row - not only in the first column.
  m_lMsg.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_DOUBLEBUFFER, LVS_EX_DOUBLEBUFFER);

  m_lMsg.InsertColumn(0, _T("flag"), LVCFMT_RIGHT, nColInterval*3);
  m_lMsg.InsertColumn(0, _T("dlc"), LVCFMT_RIGHT, nColInterval);
  m_lMsg.InsertColumn(0, _T("data"), LVCFMT_RIGHT, nColInterval*4);
  m_lMsg.InsertColumn(0, _T("cycle"), LVCFMT_RIGHT, nColInterval*2);
  m_lMsg.InsertColumn(0, _T("type"), LVCFMT_RIGHT, nColInterval*3);
  m_lMsg.InsertColumn(0, _T("dir"), LVCFMT_RIGHT, nColInterval);
  m_lMsg.InsertColumn(0, _T("slot"), LVCFMT_RIGHT, nColInterval);
  m_lMsg.InsertColumn(0, _T("ch"), LVCFMT_RIGHT, nColInterval);
  m_lMsg.InsertColumn(0, _T("time"), LVCFMT_RIGHT, nColInterval*4);

  // init our TxFrame
  m_eSlotEray = "1";
  m_eSlotCold = "2";

  // Set window title dependent on OS version
#ifdef _WIN64
  this->SetWindowText("xlFlexDemo - 64bit");
#else
  this->SetWindowText("xlFlexDemo - 32bit");
#endif

  UINT_PTR id;
  // Start cyclic status panel update
  id = SetTimer(TIMER_ID_OUTPUT_PANEL_UPDATE, TIMER_DELAY_OUTPUT_PANEL_UPDATE, NULL);
  if (id != TIMER_ID_OUTPUT_PANEL_UPDATE) {
    // Error when starting timer
    XLDEBUG(DEBUG_ADV, "## Error: Timer(TIMER_ID_UPDATE_OUTPUT_PANEL) couldn't be started!! ##\n");
  }

  // -----------------------------
	// here we init the FlexRay part
  // -----------------------------
  XLstatus xlStatus;

  m_Fr.m_plDevice = &m_lDevice;
  m_Fr.m_plMsg    = &m_lMsg;
  
  xlStatus = m_Fr.FrInit();
  if (xlStatus) return FALSE;
   
  // we found some FlexRay devices... and select the first one
  m_lDevice.SetCurSel(0);
  m_btnActivate.EnableWindow(TRUE);

  UpdateData(FALSE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CxlFlexDemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CxlFlexDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CxlFlexDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CxlFlexDemoDlg::OnBnClickedAbout()
{ 
  CAboutDlg a;
  a.DoModal();	
}


void CxlFlexDemoDlg::OnBnClickedActivate()
{
  XLstatus xlStatus;
  unsigned short slotEray;
  unsigned short slotCold;
  unsigned char flags = 0;
  
  UpdateData(TRUE);

  if (!m_bBtnActivate) {
    
    slotEray = (unsigned short) atoi(m_eSlotEray);
    slotCold = (unsigned short) atoi(m_eSlotCold);

    if (m_cColdCC) flags |= USE_STARTUP_COLD;
    if (m_cEray) flags |= USE_STARTUP_ERAY;
    if (m_cSpy) flags |= USE_SPY_MODE;

    xlStatus = m_Fr.FrActivate(m_lDevice.GetCurSel(),
                               slotEray, slotCold, flags);
    if (!xlStatus) {
      m_lDevice.EnableWindow(FALSE);
      m_btnActivate.SetWindowText("&Deactivate");
      m_bBtnActivate = TRUE;
    }
  }
  else {
    xlStatus = m_Fr.FrDeactivate(m_lDevice.GetCurSel());
    if (!xlStatus) {
      m_lDevice.EnableWindow(TRUE);
      m_bShowTrace.SetWindowText("&Break Trace");
      m_btnActivate.SetWindowText("&Activate");
      m_bBtnActivate = FALSE;
      m_bBtnShowTrace = TRUE;
    }
  }

  if (xlStatus) {

  }
}

void CxlFlexDemoDlg::OnBnClickedClear()
{
  m_lMsg.DeleteAllItems();
}


void CxlFlexDemoDlg::OnBnClickedAddtxframe()
{
  if (m_bBtnActivate) {
    CTxFrameDlg txFrame;

    txFrame.m_pFr   = &m_Fr;
    txFrame.m_nChan = m_lDevice.GetCurSel();
    txFrame.DoModal();
  }
  else {
    
  }
}

void CxlFlexDemoDlg::OnBnClickedShow()
{
  if (m_bBtnShowTrace) {
    m_bShowTrace.SetWindowText("&Show Trace");
    m_bBtnShowTrace = FALSE;
  }
  else{
    m_bShowTrace.SetWindowText("&Break Trace");
    m_bBtnShowTrace = TRUE;
  }

  m_Fr.FrToggleTrace(m_lDevice.GetCurSel(), m_bBtnShowTrace);
}

void CxlFlexDemoDlg::OnBnClickedCopy()
{
  CString      cstrList;
  CString      cstrItem;
  int          count;
  CHeaderCtrl* pHeader = NULL;
  HDITEM       hdi;
  TCHAR        lpBuffer[256];

  pHeader = m_lMsg.GetHeaderCtrl();

  if (pHeader) {
    memset(lpBuffer, 0, sizeof(lpBuffer));
    count = pHeader->GetItemCount();
    hdi.mask = HDI_TEXT;
    hdi.pszText = lpBuffer;
    hdi.cchTextMax = 256;
    for (int i = 0; i < count - 1; ++i) {
      pHeader->GetItem(i, &hdi);
      cstrList.AppendFormat("%s;\t", hdi.pszText);
    }
    pHeader->GetItem(count - 1, &hdi);
    cstrList.AppendFormat("%s;", hdi.pszText);
    cstrList += "\r\n\r\n";
    // Format the content of the message list
    for (int i = 0; i < m_lMsg.GetItemCount(); i++) {
      //m_lMsg.GetText(i, cstrItem);
      for (int j = 0; j < count - 1; ++j) {
        cstrList.Append(m_lMsg.GetItemText(i, j));
        cstrList += "\t";
      }
      cstrList.Append(m_lMsg.GetItemText(i, count - 1));
      cstrList += "\r\n";
    }

    if (cstrList.GetLength()) {
      //Copy the content of the status box to the clipboard
      if (OpenClipboard()) {
        EmptyClipboard();

        // Convert CString to character array
        const char* szText = cstrList.GetString();

        HANDLE hData = GlobalAlloc(GMEM_MOVEABLE, lstrlen(szText) + 1);
        LPSTR pData = (LPSTR) ::GlobalLock(hData);
        lstrcpy(pData, szText);
        GlobalUnlock(hData);

        SetClipboardData(CF_TEXT, hData);
        CloseClipboard();
      }
    }
  }
}

void CxlFlexDemoDlg::OnBnClickedOk()
{ 
  if (m_bBtnActivate) {
    // close all stuff... (endup the thread...)
    m_Fr.FrDeactivate(m_lDevice.GetCurSel());
  }

  // Disable cyclic output panel updates
  KillTimer(TIMER_ID_OUTPUT_PANEL_UPDATE);

  OnOK();
}

void CxlFlexDemoDlg::OnBnClickedEray()
{
  UpdateData(TRUE);

  if (m_cEray) {
    m_ebSlotEray.EnableWindow(TRUE);
    m_cSpy = FALSE;
  }
  else {
    m_ebSlotEray.EnableWindow(FALSE);
    m_ebSlotCold.EnableWindow(FALSE); 
    m_cColdCC = FALSE;
    m_cSpy = TRUE;
  }
   UpdateData(FALSE);
}

void CxlFlexDemoDlg::OnBnClickedColdcc()
{
  UpdateData(TRUE);
  if (m_cEray) {
    if (m_cColdCC) {
      m_ebSlotCold.EnableWindow(TRUE);  
    }
    else {
      m_ebSlotCold.EnableWindow(FALSE);  
    }
  }
  else {
    m_cColdCC = FALSE;
  }

  UpdateData(FALSE);
}

void CxlFlexDemoDlg::OnBnClickedSpy()
{
  UpdateData(TRUE);
  if (m_cSpy) {
    m_ebSlotCold.EnableWindow(FALSE);  
    m_ebSlotEray.EnableWindow(FALSE); 
    m_cColdCC = FALSE;
    m_cEray = FALSE;
  }
  else {
    m_ebSlotCold.EnableWindow(TRUE);  
    m_ebSlotEray.EnableWindow(TRUE);
    m_cEray = TRUE;
    m_cColdCC = TRUE;
  }

  UpdateData(FALSE);
}


// Update listbox control cyclically. The RX-thread only updates the data structure of this item
// but does no update/repaint of the GUI due to performance reasons.
void CxlFlexDemoDlg::OnTimer(UINT_PTR nIDEvent) {
  // Timer for message output panel update
  if (nIDEvent == TIMER_ID_OUTPUT_PANEL_UPDATE) {
    // Don't stop timer to be repeatedly called

    // Update listbox content
    static int nLastCount = 0;
    int nCount = m_lMsg.GetItemCount();
    if (nCount > nLastCount) {
      m_lMsg.EnsureVisible(nCount-1, FALSE);
      nLastCount = nCount;
    }

    //m_lMsg.SendMessage(WM_SETREDRAW, TRUE, 0);
    m_lMsg.SetRedraw(TRUE);
    m_lMsg.UpdateWindow();
    //m_lMsg.SendMessage(WM_SETREDRAW, FALSE, 0);
    m_lMsg.SetRedraw(FALSE);
  }
} //OnTimer()


// Redraw listbox items in case the content is scrolled
void CxlFlexDemoDlg::OnLvnEndScrollMsglist(NMHDR *pNMHDR, LRESULT *pResult)
{
  UNREFERENCED_PARAMETER(pNMHDR);
  //LPNMLVSCROLL pStateChanged = reinterpret_cast<LPNMLVSCROLL>(pNMHDR);

  m_lMsg.SetRedraw(TRUE);
  m_lMsg.RedrawWindow();
  m_lMsg.SetRedraw(FALSE);

  *pResult = 0;
}
