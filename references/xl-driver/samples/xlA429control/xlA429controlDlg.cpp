/*----------------------------------------------------------------------------
| File        : xlA429controlDlg.cpp
| Project     : Vector A429 Example 
|
| Description : handles the dialog box
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2004 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlA429control.h"
#include "xlA429controlDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXlA429controlDlg dialog

CXlA429controlDlg::CXlA429controlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CXlA429controlDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CXlA429controlDlg)
  m_eTxMinGap       = _T("");
  m_eTxBitrate      = _T("");
  m_eRxMinGap       = _T("");
  m_autoBaudrate    = TRUE;
  m_eRxMinGap       = _T("");
  m_eLabel          = _T("");
	m_eData           = _T("");
  m_eTxMsgCycleTime = _T("");
  m_eTxMsgGap       = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CXlA429controlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CXlA429controlDlg)
	DDX_Control(pDX, IDC_HARDWARE, m_Hardware);
	DDX_Control(pDX, IDC_SEND, m_btnSend);
	DDX_Control(pDX, IDC_OFFBUS, m_btnOffBus);
	DDX_Control(pDX, IDC_ONBUS, m_btnOnBus);
	DDX_Control(pDX, IDC_OUTPUT, m_Output);
  DDX_Control(pDX, IDC_TX_PARITY, m_TxParity);
  DDX_Control(pDX, IDC_RX_PARITY, m_RxParity);
  DDX_Text(pDX, IDC_TX_MINGAP, m_eTxMinGap);
  DDX_Text(pDX, IDC_TX_BITRATE, m_eTxBitrate);
  DDX_Text(pDX, IDC_RX_MINGAP, m_eRxMinGap);
  DDX_Check(pDX, IDC_RX_AUTOBAUDRATE, m_autoBaudrate);
  DDX_Text(pDX, IDC_RX_BITRATE, m_eRxBitrate);
  DDX_Text(pDX, IDC_LABEL, m_eLabel);
  DDX_Text(pDX, IDC_DATA, m_eData);
  DDX_Control(pDX, IDC_MSG_FLAGS, m_txMsgFlag);
  DDX_Text(pDX, IDC_MSG_CYCLE_TIME, m_eTxMsgCycleTime);
  DDX_Control(pDX, IDC_MSG_PARITY, m_txMsgParity);
  DDX_Text(pDX, IDC_MSG_GAP, m_eTxMsgGap);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CXlA429controlDlg, CDialog)
	//{{AFX_MSG_MAP(CXlA429controlDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_ONBUS, OnOnbus)
	ON_BN_CLICKED(IDC_OFFBUS, OnOffbus)
	ON_BN_CLICKED(IDC_SEND, OnSend)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_ABOUT, OnAbout)
	ON_BN_CLICKED(IDC_LIC_INFO, OnLicInfo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXlA429controlDlg message handlers

BOOL CXlA429controlDlg::OnInitDialog()
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
	
  // we need the listbox pointer within our A429 class
  m_A429.m_pOutput = &m_Output;
  m_A429.m_pHardware = &m_Hardware;

  // init the A429 hardware
  if (m_A429.A429Init()) {
    m_Hardware.ResetContent();
    m_Hardware.AddString("<ERROR> no HW!");
    m_Output.AddString("You need a VN0601 hardware interface");
	  m_btnOnBus.EnableWindow(FALSE);
  }

  // init the default window
  m_eTxMinGap  = "32";
  m_eTxBitrate = "100000";
  m_eRxMinGap  = "32";
  m_eRxBitrate = "100000";
  m_eLabel     = "1";
  m_eData      = "1234";

  m_TxParity.SetCurSel(1);
  m_RxParity.SetCurSel(1);
  
  m_txMsgFlag.SetCurSel(0);
  m_eTxMsgCycleTime = "0";
  m_txMsgParity.SetCurSel(0);
  m_eTxMsgGap = "32";

  UpdateData(FALSE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CXlA429controlDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CXlA429controlDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXlA429controlDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CXlA429controlDlg::OnOnbus() 
{
	int          nTxIndex = 0;
  int          nRxIndex = 0;
  XLstatus     xlStatus;
  unsigned int parity[4] = { XL_A429_MSG_PARITY_DEFAULT, 
                             XL_A429_MSG_PARITY_DISABLED, 
                             XL_A429_MSG_PARITY_ODD, 
                             XL_A429_MSG_PARITY_EVEN};

  unsigned int txMinGap = 0;
  unsigned int txBitrate = 0;
  unsigned int rxMinGap = 0;
  unsigned int rxAutoBaudrate = XL_A429_MSG_AUTO_BAUDRATE_DISABLED;
  unsigned int rxBitrate = 0;
    
  UpdateData(TRUE);

  nTxIndex = m_TxParity.GetCurSel();
  txMinGap = atoi(m_eTxMinGap);
  txBitrate = atoi(m_eTxBitrate);

  nRxIndex = m_RxParity.GetCurSel();
  rxMinGap = atoi(m_eRxMinGap);
  if (m_autoBaudrate) {
    rxAutoBaudrate = XL_A429_MSG_AUTO_BAUDRATE_ENABLED;
    rxBitrate = 0;
  }
  else {
    rxAutoBaudrate = XL_A429_MSG_AUTO_BAUDRATE_DISABLED;
    rxBitrate = atoi(m_eRxBitrate);
  }

  // activate channel(s) with parameter settings
  xlStatus = m_A429.A429GoOnBus(parity[nTxIndex], txMinGap, txBitrate,
                                parity[nRxIndex], rxMinGap, rxAutoBaudrate, rxBitrate);

  if (!xlStatus) {
    m_btnOnBus.EnableWindow(FALSE);
    m_btnOffBus.EnableWindow(TRUE);
    m_TxParity.EnableWindow(FALSE);
    m_RxParity.EnableWindow(FALSE);
    m_btnSend.EnableWindow(TRUE);

    m_Output.InsertString(-1,"Successfully GoOnBus");
  }
  else m_Output.InsertString(-1,"Error: GoOnBus");
	
}

void CXlA429controlDlg::OnOffbus() 
{
  XLstatus xlStatus;

	m_btnOnBus.EnableWindow(TRUE);
  m_btnOffBus.EnableWindow(FALSE);
	m_TxParity.EnableWindow(TRUE);
  m_RxParity.EnableWindow(TRUE);
  m_btnSend.EnableWindow(FALSE);

  xlStatus = m_A429.A429GoOffBus();
  if (!xlStatus) m_Output.InsertString(-1,"Successfully GoOffBus");
  else m_Output.InsertString(-1,"Error: GoOffBus");
}

void CXlA429controlDlg::OnSend() 
{
	XL_A429_MSG_TX xlA429MsgTx;
  unsigned int nFlagsIndex = 0;
  unsigned int nParityIndex = 0;
  unsigned int flags[3] = { XL_A429_MSG_FLAG_ON_REQUEST, 
                            XL_A429_MSG_FLAG_CYCLIC, 
                            XL_A429_MSG_FLAG_DELETE_CYCLIC};
  unsigned int parity[4] = { XL_A429_MSG_PARITY_DEFAULT, 
                             XL_A429_MSG_PARITY_DISABLED, 
                             XL_A429_MSG_PARITY_ODD, 
                             XL_A429_MSG_PARITY_EVEN};
  
  memset(&xlA429MsgTx, 0, sizeof(XL_A429_MSG_TX));

  UpdateData(TRUE);

  nFlagsIndex = m_txMsgFlag.GetCurSel();
  xlA429MsgTx.flags = flags[nFlagsIndex];
  if (xlA429MsgTx.flags == XL_A429_MSG_FLAG_ON_REQUEST) {
    xlA429MsgTx.cycleTime = 0;
  }
  else if (xlA429MsgTx.flags == XL_A429_MSG_FLAG_CYCLIC) {
    xlA429MsgTx.cycleTime  = (unsigned int)atoi(m_eTxMsgCycleTime);
  }
  else {
    xlA429MsgTx.cycleTime  = 0;
  }

  xlA429MsgTx.label      = (unsigned char)atoi(m_eLabel);
  xlA429MsgTx.gap        = atoi(m_eTxMsgGap);
  nParityIndex           = m_txMsgParity.GetCurSel();
  xlA429MsgTx.parity     = (unsigned char)parity[nParityIndex];
  xlA429MsgTx.userHandle = 0;
  
  xlA429MsgTx.data = (unsigned int)atoi(m_eData);

  m_A429.A429Send(xlA429MsgTx, CHAN01);
	
}

void CXlA429controlDlg::OnClear() 
{
  m_Output.ResetContent();	
}

void CXlA429controlDlg::OnAbout() 
{
	CAboutDlg a;
  a.DoModal();	
}

void CXlA429controlDlg::OnLicInfo() 
{
  m_A429.ShowLicenses();
}
