// xlMOSTViewDlg.cpp : Implementierungsdatei
//

#include "stdafx.h"
#include "xlMOSTView.h"
#include "xlMOSTViewDlg.h"
#include ".\xlmostviewdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg-Dialogfeld für Anwendungsbefehl 'Info'

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialogfelddaten
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung

// Implementierung
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


// CxlMOSTViewDlg Dialogfeld



CxlMOSTViewDlg::CxlMOSTViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CxlMOSTViewDlg::IDD, pParent)
  , m_esTargetAdr(_T(""))
  , m_esSourceAdr(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
  for (int i=0;i<XL_CONFIG_MAX_CHANNELS;i++) m_ToggleLight[i] = XL_MOST_LIGHT_OFF; // light off
}

CxlMOSTViewDlg::~CxlMOSTViewDlg()
{
  //m_MOST.MOSTClose();
}

void CxlMOSTViewDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LIST1, m_ListBox);
  DDX_Control(pDX, IDC_LIGHT, m_btnToggleLight);
  DDX_Control(pDX, IDC_DEVICE, m_lDevice);
  DDX_Control(pDX, ID_ACTIVATE, m_btnActivate);
  DDX_Control(pDX, IDC_DEACTIVATE, m_btnDeactivate);
  DDX_Text(pDX, IDC_NODE_ADR, m_esTargetAdr);
  DDX_Control(pDX, IDC_STREAMING, m_btnStream);
  DDX_Control(pDX, IDC_TWINKEL, m_btnTwinkel);
  DDX_Control(pDX, IDC_NODE_CONFIG, n_btnNodeConfig);
  DDX_Control(pDX, IDC_GEN_TEST, m_btnGeneralTest);
  DDX_Control(pDX, IDC_GET_INFO, m_btnGetInfo);
  DDX_Control(pDX, IDC_TX_CTRL, m_btnSendCtrlFrame);
}

BEGIN_MESSAGE_MAP(CxlMOSTViewDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
  ON_BN_CLICKED(ID_ACTIVATE, OnBnClickedActivate)
  ON_BN_CLICKED(IDC_DEACTIVATE, OnBnClickedDeactivate)
  ON_BN_CLICKED(IDC_LIGHT, OnBnClickedLight)
  ON_LBN_DBLCLK(IDC_DEVICE, OnLbnDblclkDevice)
  ON_BN_CLICKED(IDC_CLEAR, OnBnClickedClear)
  ON_BN_CLICKED(IDC_TX_CTRL, OnBnClickedTxCtrl)
  ON_BN_CLICKED(IDC_GET_INFO, OnBnClickedGetInfo)
  ON_BN_CLICKED(IDC_NODE_CONFIG, OnBnClickedNodeConfig)
  ON_BN_CLICKED(IDC_GEN_TEST, OnBnClickedGenTest)
  ON_BN_CLICKED(IDC_TWINKEL, OnBnClickedTwinkel)
  ON_BN_CLICKED(IDC_ABOUT, OnBnClickedAbout)
  ON_BN_CLICKED(IDC_STREAMING, &CxlMOSTViewDlg::OnBnClickedStreaming)
END_MESSAGE_MAP()


// CxlMOSTViewDlg Meldungshandler

BOOL CxlMOSTViewDlg::OnInitDialog()
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

  m_MOST.m_pListBox = &m_ListBox;
  m_MOST.m_plDevice = &m_lDevice;

  m_MOSTNodeConfig.m_pMOST = &m_MOST;
  m_MOSTGeneralTest.m_pMOST = &m_MOST;
 
  m_esTargetAdr  = "0x0fff";
  UpdateData(FALSE);

  m_bStream = FALSE;

  xlStatus = m_MOST.MOSTInit();
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

void CxlMOSTViewDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CxlMOSTViewDlg::OnPaint() 
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
HCURSOR CxlMOSTViewDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CxlMOSTViewDlg::OnBnClickedActivate()
{
  int nChan = m_lDevice.GetCurSel();
  XLstatus xlStatus;

  xlStatus = m_MOST.MOSTActivate(nChan);
  if (xlStatus == XL_SUCCESS) {
    m_btnActivate.EnableWindow(FALSE);
    // enable the buttons
    m_btnDeactivate.EnableWindow(TRUE);
    m_btnStream.EnableWindow(TRUE);
    m_btnTwinkel.EnableWindow(TRUE);
    n_btnNodeConfig.EnableWindow(TRUE);
    m_btnGeneralTest.EnableWindow(TRUE);
    m_btnGetInfo.EnableWindow(TRUE);
    m_btnSendCtrlFrame.EnableWindow(TRUE);
    m_btnToggleLight.EnableWindow(TRUE);
    m_lDevice.EnableWindow(FALSE);
  } else {
    m_ListBox.InsertString(-1, "Activate failed!");
  }
}

void CxlMOSTViewDlg::OnBnClickedDeactivate()
{
//  int nChan = m_lDevice.GetCurSel();
  XLstatus xlStatus;

  xlStatus = m_MOST.MOSTDeActivate();
  if (xlStatus!= XL_SUCCESS) {
    CString txt;
    txt.Format("0x%x Deactivate failed!", xlStatus);
    m_ListBox.InsertString(-1, txt);
  }
  m_btnActivate.EnableWindow(TRUE);
  // disable the buttons
  m_btnDeactivate.EnableWindow(FALSE);
  m_btnStream.EnableWindow(FALSE);
  m_btnTwinkel.EnableWindow(FALSE);
  n_btnNodeConfig.EnableWindow(FALSE);
  m_btnGeneralTest.EnableWindow(FALSE);
  m_btnGetInfo.EnableWindow(FALSE);
  m_btnSendCtrlFrame.EnableWindow(FALSE);
  m_btnToggleLight.EnableWindow(FALSE);
  m_lDevice.EnableWindow(TRUE);
}

void CxlMOSTViewDlg::OnBnClickedStreaming()
{
  int nChan = m_lDevice.GetCurSel();

  if (m_bStream) {
    m_btnStream.SetWindowText("Start Stream");
    m_btnStream.EnableWindow(FALSE);
    m_MOST.MOSTStreamClose();
    m_bStream = FALSE;
  }
  else {
    m_MOST.MOSTStreamInit(nChan);
    m_btnStream.SetWindowText("Stop Stream");
    m_bStream = TRUE;
  }
}

void CxlMOSTViewDlg::OnBnClickedLight()
{
  int nChan = m_lDevice.GetCurSel();

  switch (m_ToggleLight[nChan]) {

    case XL_MOST_LIGHT_OFF:
      //SetDlgItemText(IDC_LIGHT,"Set Light modulated");
      m_ToggleLight[nChan] = XL_MOST_LIGHT_FORCE_ON;
      break;
    case XL_MOST_LIGHT_FORCE_ON:
      //SetDlgItemText(IDC_LIGHT,"Set Light off");
      m_ToggleLight[nChan] = XL_MOST_LIGHT_MODULATED;
      break;
    case XL_MOST_LIGHT_MODULATED:
      //m_ToggleLight = 0;
      //SetDlgItemText(IDC_LIGHT,"Set Light on");
      m_ToggleLight[nChan] = XL_MOST_LIGHT_OFF;
      break;
  }

  m_MOST.MOSTToggleLight(m_ToggleLight[nChan]);
}

void CxlMOSTViewDlg::OnLbnDblclkDevice()
{

  m_MOSTNodeConfig.m_nChan = m_lDevice.GetCurSel();

  m_MOSTNodeConfig.DoModal();

}

void CxlMOSTViewDlg::OnBnClickedClear()
{
  m_ListBox.ResetContent();	
}

void CxlMOSTViewDlg::OnBnClickedTxCtrl()
{
  UpdateData(TRUE);

  unsigned short nTargetAdr  = m_General.StrToShort(m_esTargetAdr.GetBuffer(1));

  // Check the parameter
  if (m_esTargetAdr.GetLength() > 6) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else
    m_MOST.MOSTCtrlTransmit(nTargetAdr);  
}

void CxlMOSTViewDlg::OnBnClickedGetInfo()
{
  m_MOST.MOSTGetInfo();
}

void CxlMOSTViewDlg::OnBnClickedNodeConfig()
{
  m_MOSTNodeConfig.m_nChan = m_lDevice.GetCurSel();
  m_MOSTNodeConfig.DoModal();
}

void CxlMOSTViewDlg::OnBnClickedGenTest()
{
  m_MOSTGeneralTest.m_nChan = m_lDevice.GetCurSel();
  m_MOSTGeneralTest.DoModal();
}

void CxlMOSTViewDlg::OnBnClickedTwinkel()
{
  m_MOST.MOSTTwinklePowerLED();
}

void CxlMOSTViewDlg::OnBnClickedAbout()
{
  CAboutDlg dlgAbout;
  dlgAbout.DoModal();
}

