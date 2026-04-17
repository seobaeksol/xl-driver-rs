// xlMOST150ViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "xlMOST150View.h"
#include "xlMOST150ViewDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialog
{
public:
  CAboutDlg();

  enum { IDD = IDD_ABOUTBOX };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);

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


// CxlMOST150ViewDlg Dialogfeld



CxlMOST150ViewDlg::CxlMOST150ViewDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CxlMOST150ViewDlg::IDD, pParent)
  , m_esCtrlTargetAdr(_T(""))
  , m_esAsyncTargetAdr(_T(""))
{
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  for (int i=0;i<XL_CONFIG_MAX_CHANNELS;i++) m_ToggleLight[i] = XL_MOST_LIGHT_OFF; // light off
}

CxlMOST150ViewDlg::~CxlMOST150ViewDlg()
{
  //m_MOST150.MOST150Close();
}

void CxlMOST150ViewDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LIST1, m_ListBox);
  DDX_Control(pDX, IDC_STARTUP, m_btnNwStartup);
  DDX_Control(pDX, IDC_SHUTDOWN, m_btnNwShutdown);
  DDX_Control(pDX, IDC_DEVICE, m_lDevice);
  DDX_Control(pDX, IDC_ACTIVATE, m_btnActivate);
  DDX_Control(pDX, IDC_DEACTIVATE, m_btnDeactivate);
  DDX_Text(pDX, IDC_CTRL_TARGET_ADDR, m_esCtrlTargetAdr);
  DDX_Text(pDX, IDC_ASYNC_TARGET_ADDR, m_esAsyncTargetAdr);
  DDX_Control(pDX, IDC_TWINKLE, m_btnTwinkel);
  DDX_Control(pDX, IDC_NODE_CONFIG, n_btnNodeConfig);
  DDX_Control(pDX, IDC_GEN_TEST, m_btnGeneralTest);
  DDX_Control(pDX, IDC_GET_INFO, m_btnGetInfo);
  DDX_Control(pDX, IDC_TX_CTRL, m_btnSendCtrlFrame);
  DDX_Control(pDX, IDC_SEND_ASYNC_TX, m_btnSendAsyncFrame);
  DDX_Control(pDX, IDC_STREAMING, m_btnStreaming);
}

BEGIN_MESSAGE_MAP(CxlMOST150ViewDlg, CDialog)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  //}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_ACTIVATE, OnBnClickedActivate)
  ON_BN_CLICKED(IDC_DEACTIVATE, OnBnClickedDeactivate)
  ON_BN_CLICKED(IDC_STARTUP, OnBnClickedStartup)
  ON_BN_CLICKED(IDC_SHUTDOWN, OnBnClickedShutdown)
  ON_LBN_DBLCLK(IDC_DEVICE, OnLbnDblclkDevice)
  ON_BN_CLICKED(IDC_CLEAR, OnBnClickedClear)
  ON_BN_CLICKED(IDC_TX_CTRL, OnBnClickedTxCtrl)
  ON_BN_CLICKED(IDC_SEND_ASYNC_TX, OnBnClickedSendAsyncTx)
  ON_BN_CLICKED(IDC_GET_INFO, OnBnClickedGetInfo)
  ON_BN_CLICKED(IDC_NODE_CONFIG, OnBnClickedNodeConfig)
  ON_BN_CLICKED(IDC_GEN_TEST, OnBnClickedGenTest)
  ON_BN_CLICKED(IDC_TWINKLE, OnBnClickedTwinkle)
  ON_BN_CLICKED(IDC_ABOUT, OnBnClickedAbout)
  ON_BN_CLICKED(IDC_STREAMING, OnBnClickedStreaming)
END_MESSAGE_MAP()


// CxlMOST150ViewDlg Meldungshandler

BOOL CxlMOST150ViewDlg::OnInitDialog()
{
  CDialog::OnInitDialog();

  // Hinzufügen des Menübefehls "Info..." zum Systemmenü.

  // IDM_ABOUTBOX muss sich im Bereich der Systembefehle befinden.
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

  // Symbol für dieses Dialogfeld festlegen. Wird automatisch erledigt
  //  wenn das Hauptfenster der Anwendung kein Dialogfeld ist
  SetIcon(m_hIcon, TRUE);			// Großes Symbol verwenden
  SetIcon(m_hIcon, FALSE);		// Kleines Symbol verwenden

  // here we init the MOST part
  XLstatus xlStatus;

  m_MOST150.m_pListBox = &m_ListBox;
  m_MOST150.m_plDevice = &m_lDevice;

  m_MOST150NodeConfig.m_pMOST150 = &m_MOST150;
  m_MOST150GeneralTest.m_pMOST150 = &m_MOST150;
 
  m_esCtrlTargetAdr  = "0xFFFF";
  m_esAsyncTargetAdr = "0xFFFF";

  m_bStream = FALSE;

  UpdateData(FALSE);

  xlStatus = m_MOST150.MOST150Init();
  if (!xlStatus) {
    m_lDevice.SetCurSel(0);
  }
  else if (xlStatus == XL_ERR_CANNOT_OPEN_DRIVER) {
    AfxMessageBox("Error: No VXLAPI.DLL found!");
    OnCancel(); // no vxlapi.dll found.
  }
  else if (xlStatus == XL_ERR_INVALID_ACCESS) {
    m_ListBox.InsertString(-1, "Error in init hardware... No init access ?");    
  }
  else m_ListBox.InsertString(-1, "Something went wrong in init the hardware!");

  return TRUE;  // Geben Sie TRUE zurück, außer ein Steuerelement soll den Fokus erhalten
}

void CxlMOST150ViewDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

// Wenn Sie dem Dialogfeld eine Schaltfläche "Minimieren" hinzufügen, benötigen Sie 
//  den nachstehenden Code, um das Symbol zu zeichnen. Für MFC-Anwendungen, die das 
//  Dokument/Ansicht-Modell verwenden, wird dies automatisch ausgeführt.

void CxlMOST150ViewDlg::OnPaint() 
{
  if (IsIconic())
  {
    CPaintDC dc(this); // Gerätekontext zum Zeichnen

    SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

    // Symbol in Clientrechteck zentrieren
    int cxIcon = GetSystemMetrics(SM_CXICON);
    int cyIcon = GetSystemMetrics(SM_CYICON);
    CRect rect;
    GetClientRect(&rect);
    int x = (rect.Width() - cxIcon + 1) / 2;
    int y = (rect.Height() - cyIcon + 1) / 2;

    // Symbol zeichnen
    dc.DrawIcon(x, y, m_hIcon);
  }
  else
  {
    CDialog::OnPaint();
  }
}

// Die System ruft diese Funktion auf, um den Cursor abzufragen, der angezeigt wird, während der Benutzer
//  das minimierte Fenster mit der Maus zieht.
HCURSOR CxlMOST150ViewDlg::OnQueryDragIcon()
{
  return static_cast<HCURSOR>(m_hIcon);
}

void CxlMOST150ViewDlg::OnBnClickedActivate()
{
  int nChan = m_lDevice.GetCurSel();
  XLstatus xlStatus;

  xlStatus = m_MOST150.MOST150Activate(nChan);
  if (xlStatus == XL_SUCCESS) {
    m_btnActivate.EnableWindow(FALSE);
    // enable the buttons
    m_btnDeactivate.EnableWindow(TRUE);
    m_btnTwinkel.EnableWindow(TRUE);
    n_btnNodeConfig.EnableWindow(TRUE);
    m_btnGeneralTest.EnableWindow(TRUE);
    m_btnGetInfo.EnableWindow(TRUE);
    m_btnSendCtrlFrame.EnableWindow(TRUE);
    m_btnSendAsyncFrame.EnableWindow(TRUE);
    m_btnNwStartup.EnableWindow(TRUE);
    m_btnNwShutdown.EnableWindow(TRUE);
    m_lDevice.EnableWindow(FALSE);
    m_btnStreaming.EnableWindow(TRUE);
  } else {
    m_ListBox.InsertString(-1, "Activate failed!");
  }
}

void CxlMOST150ViewDlg::OnBnClickedDeactivate()
{
//  int nChan = m_lDevice.GetCurSel();
  XLstatus xlStatus;

  xlStatus = m_MOST150.MOST150DeActivate();
  if (xlStatus!= XL_SUCCESS) {
    CString txt;
    txt.Format("0x%x Deactivate failed!", xlStatus);
    m_ListBox.InsertString(-1, txt);
  }
  m_btnActivate.EnableWindow(TRUE);
  // disable the buttons
  m_btnDeactivate.EnableWindow(FALSE);
  m_btnTwinkel.EnableWindow(FALSE);
  n_btnNodeConfig.EnableWindow(FALSE);
  m_btnGeneralTest.EnableWindow(FALSE);
  m_btnGetInfo.EnableWindow(FALSE);
  m_btnSendCtrlFrame.EnableWindow(FALSE);
  m_btnSendAsyncFrame.EnableWindow(FALSE);
  m_btnNwStartup.EnableWindow(FALSE);
  m_btnNwShutdown.EnableWindow(FALSE);
  m_lDevice.EnableWindow(TRUE);
  m_btnStreaming.EnableWindow(FALSE);
}

void CxlMOST150ViewDlg::OnBnClickedStartup()
{
  m_MOST150.MOST150NwStartup();
}

void CxlMOST150ViewDlg::OnBnClickedShutdown()
{
  m_MOST150.MOST150NwShutdown();
}

void CxlMOST150ViewDlg::OnBnClickedStreaming()
{
  int nChan = m_lDevice.GetCurSel();

  if (m_bStream) {
    m_btnStreaming.SetWindowText("Start Stream");
    m_MOST150.MOST150StreamStop();
    m_bStream = FALSE;
  }
  else {
    if(m_MOST150.MOST150StreamStart(nChan) == XL_SUCCESS)
    {
      m_btnStreaming.SetWindowText("Stop Stream");
      m_bStream = TRUE;
    }
  }
}

void CxlMOST150ViewDlg::OnLbnDblclkDevice()
{

  m_MOST150NodeConfig.m_nChan = m_lDevice.GetCurSel();

  m_MOST150NodeConfig.DoModal();

}

void CxlMOST150ViewDlg::OnBnClickedClear()
{
  m_ListBox.ResetContent();	
}

void CxlMOST150ViewDlg::OnBnClickedTxCtrl()
{
  UpdateData(TRUE);

  unsigned short nTargetAdr  = m_General.StrToShort(m_esCtrlTargetAdr.GetBuffer(1));

  // Check the parameter
  if (m_esCtrlTargetAdr.GetLength() > 6) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else
    m_MOST150.MOST150CtrlTransmit(nTargetAdr);  
}

void CxlMOST150ViewDlg::OnBnClickedSendAsyncTx()
{
  UpdateData(TRUE);

  unsigned short nTargetAdr  = m_General.StrToShort(m_esAsyncTargetAdr.GetBuffer(1));

  if (m_esAsyncTargetAdr.GetLength() > 6) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else
  m_MOST150.MOST150AsyncTransmit(nTargetAdr);
}

void CxlMOST150ViewDlg::OnBnClickedGetInfo()
{
  m_MOST150.MOST150GetInfo();
}

void CxlMOST150ViewDlg::OnBnClickedNodeConfig()
{
  m_MOST150NodeConfig.m_nChan = m_lDevice.GetCurSel();
  m_MOST150NodeConfig.DoModal();
}

void CxlMOST150ViewDlg::OnBnClickedGenTest()
{
  m_MOST150GeneralTest.m_nChan = m_lDevice.GetCurSel();
  m_MOST150GeneralTest.DoModal();
}

void CxlMOST150ViewDlg::OnBnClickedTwinkle()
{
  m_MOST150.MOST150TwinklePowerLED();
}

void CxlMOST150ViewDlg::OnBnClickedAbout()
{
  CAboutDlg dlgAbout;
  dlgAbout.DoModal();
}

