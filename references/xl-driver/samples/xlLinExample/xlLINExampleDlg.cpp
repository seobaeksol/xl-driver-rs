// LINExampleDlg.cpp : implementation file
//

#include "stdafx.h"
#include "xlLINExample.h"
#include "xlLINExampleDlg.h"
#include "AboutDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CLINExampleDlg dialog

CLINExampleDlg::CLINExampleDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLINExampleDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLINExampleDlg)
	m_LINID = _T("");
	m_IDMaster = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLINExampleDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLINExampleDlg)
	DDX_Control(pDX, IDC_ID, m_editLINID);
	DDX_Control(pDX, IDC_ID_MASTER, m_editIDMaster);
	DDX_Control(pDX, IDC_CLOSE, m_btnClose);
	DDX_Control(pDX, IDC_INIT, m_btnInit);
	DDX_Control(pDX, IDC_SENDMASTERREQ, m_btnSendReq);
	DDX_Control(pDX, IDC_STATUS, m_StatusBox);
	DDX_Control(pDX, IDC_RX, m_RXBox);
	DDX_Text(pDX, IDC_ID, m_LINID);
	DDX_Text(pDX, IDC_ID_MASTER, m_IDMaster);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLINExampleDlg, CDialog)
	//{{AFX_MSG_MAP(CLINExampleDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_INIT, OnInit)
	ON_BN_CLICKED(IDC_SENDMASTERREQ, OnSendmasterreq)
	ON_BN_CLICKED(IDC_CLOSE, OnClose)
	ON_BN_CLICKED(IDC_CLEAR, OnClear)
	ON_BN_CLICKED(IDC_ABOUT, OnAbout)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLINExampleDlg message handlers

BOOL CLINExampleDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
  m_LIN.m_pRXBox = &m_RXBox;
  m_LIN.m_pStatusBox = &m_StatusBox;
	XLstatus xlStatus = m_LIN.LINGetDevice();

  m_data = 0x00;
  m_LINID = "04";
  m_IDMaster = "04";

  UpdateData(FALSE);
  
  if (xlStatus != XL_SUCCESS) { 
    AfxMessageBox("ERROR: You need LINcabs or LINpiggies!", MB_ICONSTOP);
    OnCancel();
  } 

	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLINExampleDlg::OnPaint() 
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
HCURSOR CLINExampleDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CLINExampleDlg::OnInit() 
{
	XLstatus         xlStatus = XL_ERROR;

  UpdateData(TRUE);
  int id = atoi(m_LINID);
 
  xlStatus = m_LIN.LINInit(id);

  if (xlStatus == XL_SUCCESS) {
    m_btnSendReq.EnableWindow(TRUE);
    m_btnInit.EnableWindow(FALSE);
    m_btnClose.EnableWindow(TRUE);
    m_editIDMaster.EnableWindow(TRUE);
    m_editLINID.EnableWindow(FALSE);
  }
  else if (xlStatus == XL_ERR_INVALID_ACCESS) {
    AfxMessageBox("ERROR: You need INIT access!", MB_ICONSTOP);
    xlStatus = m_LIN.LINClose();
  }
  else {
    AfxMessageBox("ERROR in INIT !", MB_ICONSTOP);
    xlStatus = m_LIN.LINClose();
    if (xlStatus) AfxMessageBox("ERROR in closing port !", MB_ICONSTOP);
  }

}

void CLINExampleDlg::OnSendmasterreq() 
{
	XLstatus         xlStatus = XL_ERROR;

  m_data++;

  UpdateData(TRUE);
  int id = atoi(m_IDMaster);
 
  xlStatus = m_LIN.LINSendMasterReq(m_data, id);
}

void CLINExampleDlg::OnClose() 
{
	XLstatus         xlStatus = XL_ERROR;
 
  xlStatus = m_LIN.LINClose();

  if (xlStatus) AfxMessageBox("ERROR in closing... !", MB_ICONSTOP);
  else {
    m_btnClose.EnableWindow(FALSE);
    m_btnInit.EnableWindow(TRUE);
    m_btnSendReq.EnableWindow(FALSE);
    m_editIDMaster.EnableWindow(FALSE);
    m_editLINID.EnableWindow(TRUE);
  }
 
}

void CLINExampleDlg::OnClear() 
{
  m_RXBox.ResetContent();	
  m_StatusBox.ResetContent();	
}

void CLINExampleDlg::OnAbout() 
{
  CAboutDlg a;
  a.DoModal();	
}

void CLINExampleDlg::OnOK() {

	if (m_LIN.LINClose()) AfxMessageBox("ERROR in closing... !", MB_ICONSTOP);

  CDialog::OnOK();
}

void CLINExampleDlg::OnCancel() {
	m_LIN.LINClose();	
	
	CDialog::OnCancel();
}
