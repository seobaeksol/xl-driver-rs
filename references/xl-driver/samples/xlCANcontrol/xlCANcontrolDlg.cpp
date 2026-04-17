/*----------------------------------------------------------------------------
| File        : xlCANcontrolDlg.cpp
| Project     : Vector CAN Example 
|
| Description : handles the dialog box
|-----------------------------------------------------------------------------
|-----------------------------------------------------------------------------
| Copyright (c) 2004 by Vector Informatik GmbH.  All rights reserved.
|---------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlCANcontrol.h"
#include "xlCANcontrolDlg.h"


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
// CXlCANcontrolDlg dialog

CXlCANcontrolDlg::CXlCANcontrolDlg(CWnd* pParent /*=NULL*/)
  : CDialog(CXlCANcontrolDlg::IDD, pParent)
{
  //{{AFX_DATA_INIT(CXlCANcontrolDlg)
  m_eD00 = _T("");
  m_eD01 = _T("");
  m_eD02 = _T("");
  m_eD03 = _T("");
  m_eD04 = _T("");
  m_eD05 = _T("");
  m_eD06 = _T("");
  m_eD07 = _T("");
  m_eDLC = _T("");
  m_eID = _T("");
  m_ExtID = FALSE;
  m_eFilterFrom = _T("");
  m_eFilterTo = _T("");
  //}}AFX_DATA_INIT
  // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
  m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CXlCANcontrolDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  //{{AFX_DATA_MAP(CXlCANcontrolDlg)
  DDX_Control(pDX, IDC_HARDWARE, m_Hardware);
  DDX_Control(pDX, IDC_CHANNEL, m_Channel);
  DDX_Control(pDX, IDC_SEND, m_btnSend);
  DDX_Control(pDX, IDC_OFFBUS, m_btnOffBus);
  DDX_Control(pDX, IDC_ONBUS, m_btnOnBus);
  DDX_Control(pDX, IDC_BAUDRATE, m_Baudrate);
  DDX_Control(pDX, IDC_OUTPUT, m_Output);
  DDX_Control(pDX, IDC_LIC_INFO, m_btnLicInfo);
  DDX_Text(pDX, IDC_D00, m_eD00);
  DDX_Text(pDX, IDC_D01, m_eD01);
  DDX_Text(pDX, IDC_D02, m_eD02);
  DDX_Text(pDX, IDC_D03, m_eD03);
  DDX_Text(pDX, IDC_D04, m_eD04);
  DDX_Text(pDX, IDC_D05, m_eD05);
  DDX_Text(pDX, IDC_D06, m_eD06);
  DDX_Text(pDX, IDC_D07, m_eD07);
  DDX_Text(pDX, IDC_DLC, m_eDLC);
  DDX_Text(pDX, IDC_ID, m_eID);
  DDX_Check(pDX, IDC_EXID, m_ExtID);
  DDX_Text(pDX, IDC_FILTERFROM, m_eFilterFrom);
  DDX_Text(pDX, IDC_FILTERTO, m_eFilterTo);
  //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CXlCANcontrolDlg, CDialog)
  //{{AFX_MSG_MAP(CXlCANcontrolDlg)
  ON_WM_SYSCOMMAND()
  ON_WM_PAINT()
  ON_WM_QUERYDRAGICON()
  ON_BN_CLICKED(IDC_ONBUS, OnOnbus)
  ON_BN_CLICKED(IDC_OFFBUS, OnOffbus)
  ON_BN_CLICKED(IDC_SEND, OnSend)
  ON_BN_CLICKED(IDC_CLEAR, OnClear)
  ON_BN_CLICKED(IDC_ABOUT, OnAbout)
  ON_BN_CLICKED(IDC_RESET, OnReset)
  ON_BN_CLICKED(IDC_LIC_INFO, OnLicInfo)
  ON_BN_CLICKED(IDC_SETFILTER, OnSetfilter)
  //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CXlCANcontrolDlg message handlers

BOOL CXlCANcontrolDlg::OnInitDialog()
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
  SetIcon(m_hIcon, TRUE);     // Set big icon
  SetIcon(m_hIcon, FALSE);    // Set small icon
  
  // we need the listbox pointer within our CAN class
  m_CAN.m_pOutput = &m_Output;

  // init the CAN hardware
  if (m_CAN.CANInit()) {
    m_Hardware.ResetContent();
    m_Hardware.AddString("<ERROR> no HW!");
    m_Output.AddString("Please assign channels in Vector Hardware Config and restart application.");
    m_Output.AddString("You need two CANcabs/CANpiggy's.\n");
    m_btnOnBus.EnableWindow(FALSE);
    m_btnLicInfo.EnableWindow(FALSE);
  }
  else {
    // if successful, init the channel list and drop-down menu
    m_Hardware.AddString(m_CAN.GetChannelName(CHAN01));
    m_Hardware.AddString(m_CAN.GetChannelName(CHAN02));
    m_Channel.AddString(m_CAN.GetChannelName(CHAN01));
    m_Channel.AddString(m_CAN.GetChannelName(CHAN02));
  }

  // init the default window
  m_eD00 = "00";
  m_eD01 = "00";
  m_eD02 = "00";
  m_eD03 = "00";
  m_eD04 = "00";
  m_eD05 = "00";
  m_eD06 = "00";
  m_eD07 = "00";

  m_eDLC = "08";
  m_eID  = "04";

  m_Baudrate.SetCurSel(0);
  m_Channel.SetCurSel(0);
  
  m_eFilterFrom = "5";
  m_eFilterTo   = "10";

  UpdateData(FALSE);
  
  return TRUE;  // return TRUE  unless you set the focus to a control
}

void CXlCANcontrolDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CXlCANcontrolDlg::OnPaint() 
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
HCURSOR CXlCANcontrolDlg::OnQueryDragIcon()
{
  return (HCURSOR) m_hIcon;
}

void CXlCANcontrolDlg::OnOnbus() 
{
  int           nIndex=0;
  XLstatus      xlStatus;
  unsigned long bitrate[5] = {33333, 100000, 125000, 500000, 1000000};
    
  UpdateData(TRUE);

  nIndex = m_Baudrate.GetCurSel();
  xlStatus = m_CAN.CANGoOnBus(bitrate[nIndex]);

  if (!xlStatus) {
    m_btnOnBus.EnableWindow(FALSE);
    m_btnOffBus.EnableWindow(TRUE);
    m_Baudrate.EnableWindow(FALSE);
    m_btnSend.EnableWindow(TRUE);
    m_Channel.EnableWindow(TRUE);

    m_Output.InsertString(-1,"Successfully GoOnBus");
  }
  else m_Output.InsertString(-1,"Error: GoOnBus");
  
}

void CXlCANcontrolDlg::OnOffbus() 
{
  XLstatus xlStatus;

  m_btnOnBus.EnableWindow(TRUE);
  m_btnOffBus.EnableWindow(FALSE);
  m_Baudrate.EnableWindow(TRUE);
  m_btnSend.EnableWindow(FALSE);
  m_Channel.EnableWindow(FALSE);

  xlStatus = m_CAN.CANGoOffBus();
  if (!xlStatus) m_Output.InsertString(-1,"Successfully GoOffBus");
  else m_Output.InsertString(-1,"Error: GoOffBus");
}

void CXlCANcontrolDlg::OnSend() 
{
  UpdateData(TRUE);

  XLevent xlEvent;

  memset(&xlEvent, 0, sizeof(xlEvent));

  xlEvent.tag                 = XL_TRANSMIT_MSG;
  xlEvent.tagData.msg.id      = (unsigned int)  atoi(m_eID);
  xlEvent.tagData.msg.dlc     = (unsigned short)atoi(m_eDLC);
  xlEvent.tagData.msg.flags   = 0;
  xlEvent.tagData.msg.data[0] = (unsigned char) atoi(m_eD00);
  xlEvent.tagData.msg.data[1] = (unsigned char) atoi(m_eD01);
  xlEvent.tagData.msg.data[2] = (unsigned char) atoi(m_eD02);
  xlEvent.tagData.msg.data[3] = (unsigned char) atoi(m_eD03);
  xlEvent.tagData.msg.data[4] = (unsigned char) atoi(m_eD04);
  xlEvent.tagData.msg.data[5] = (unsigned char) atoi(m_eD05);
  xlEvent.tagData.msg.data[6] = (unsigned char) atoi(m_eD06);
  xlEvent.tagData.msg.data[7] = (unsigned char) atoi(m_eD07);

  if (m_ExtID) xlEvent.tagData.msg.id |= XL_CAN_EXT_MSG_ID;

  m_CAN.CANSend(xlEvent, m_Channel.GetCurSel() );
  
}

void CXlCANcontrolDlg::OnReset() 
{
  m_CAN.CANResetFilter();
}

void CXlCANcontrolDlg::OnSetfilter() 
{
  UpdateData(TRUE);
   
  m_CAN.CANSetFilter((unsigned long) atoi(m_eFilterFrom), (unsigned long) atoi(m_eFilterTo) );
  
}

void CXlCANcontrolDlg::OnClear() 
{
  m_Output.ResetContent();	
}

void CXlCANcontrolDlg::OnAbout() 
{
  CAboutDlg a;
  a.DoModal();	
}

void CXlCANcontrolDlg::OnLicInfo() 
{
  CString cstr;
  m_CAN.AppendLicenseInfo(cstr, CHAN01);
  cstr.Append("\n");
  m_CAN.AppendLicenseInfo(cstr, CHAN02);
  AfxMessageBox(cstr.GetBuffer());
}
