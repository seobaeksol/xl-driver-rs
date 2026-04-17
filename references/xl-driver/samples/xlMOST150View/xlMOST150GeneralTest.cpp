// xlMOST150GeneralTest.cpp : implementation file
//

#include "stdafx.h"
#include "xlMOST150View.h"
#include "xlMOST150GeneralTest.h"


// CMOST150GeneralTest dialog

IMPLEMENT_DYNAMIC(CMOST150GeneralTest, CDialog)
CMOST150GeneralTest::CMOST150GeneralTest(CWnd* pParent /*=NULL*/)
  : CDialog(CMOST150GeneralTest::IDD, pParent)
  , m_esLightError(_T(""))
  , m_esLockError(_T(""))
{
}

CMOST150GeneralTest::~CMOST150GeneralTest()
{
}

void CMOST150GeneralTest::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_EDIT1, m_esLightError);
  DDX_Text(pDX, IDC_EDIT2, m_esLockError);
}


BEGIN_MESSAGE_MAP(CMOST150GeneralTest, CDialog)
  ON_BN_CLICKED(IDC_GEN_LIGHT, OnBnClickedGenLight)
  ON_BN_CLICKED(IDC_GEN_LOCK, OnBnClickedGenLock)
END_MESSAGE_MAP()


BOOL CMOST150GeneralTest::OnInitDialog() 
{
  CDialog::OnInitDialog();
   
  m_esLightError = "5";
  m_esLockError = "5";
  UpdateData(FALSE);

  return TRUE;   // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
void CMOST150GeneralTest::OnBnClickedGenLight(void)
{
  XLstatus xlStatus;

  UpdateData(TRUE);
  
  unsigned short nRepeat = (unsigned short) atoi(m_esLightError);

  if (m_esLightError.GetLength() > 2) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else {
    xlStatus = m_pMOST150->MOST150GenerateLightError(nRepeat);
    if (xlStatus == XL_ERR_NO_LICENSE) {
      AfxMessageBox("ERROR: No Licence", MB_ICONSTOP);
    }
  }
}

void CMOST150GeneralTest::OnBnClickedGenLock(void)
{
  XLstatus xlStatus;

  UpdateData(TRUE);

  unsigned short nRepeat = (unsigned short) atoi(m_esLockError);

  if (m_esLockError.GetLength() > 2) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else {
     xlStatus = m_pMOST150->MOST150GenerateLockError(nRepeat);
     if (xlStatus == XL_ERR_NO_LICENSE) {
      AfxMessageBox("ERROR: No Licence", MB_ICONSTOP);
    }
  }
}

