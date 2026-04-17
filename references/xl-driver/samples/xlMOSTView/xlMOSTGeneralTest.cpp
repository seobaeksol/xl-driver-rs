// xlMOSTGeneralTest.cpp : implementation file
//

#include "stdafx.h"
#include "xlMOSTView.h"
#include "xlMOSTGeneralTest.h"
#include ".\xlmostgeneraltest.h"


// CMOSTGeneralTest dialog

IMPLEMENT_DYNAMIC(CMOSTGeneralTest, CDialog)
CMOSTGeneralTest::CMOSTGeneralTest(CWnd* pParent /*=NULL*/)
	: CDialog(CMOSTGeneralTest::IDD, pParent)
  , m_esLightError(_T(""))
  , m_esLockError(_T(""))
{
}

CMOSTGeneralTest::~CMOSTGeneralTest()
{
}

void CMOSTGeneralTest::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_EDIT1, m_esLightError);
  DDX_Text(pDX, IDC_EDIT2, m_esLockError);
}


BEGIN_MESSAGE_MAP(CMOSTGeneralTest, CDialog)
  ON_BN_CLICKED(IDC_GEN_LIGHT, OnBnClickedGenLight)
  ON_BN_CLICKED(IDC_GEN_LOCK, OnBnClickedGenLock)
END_MESSAGE_MAP()


BOOL CMOSTGeneralTest::OnInitDialog() 
{
  CDialog::OnInitDialog();
   
  m_esLightError = "5";
  m_esLockError = "5";
  UpdateData(FALSE);

  return TRUE;   // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
void CMOSTGeneralTest::OnBnClickedGenLight()
{
  XLstatus xlStatus;

  UpdateData(TRUE);
  
  unsigned short nRepeat = (unsigned short) atoi(m_esLightError);

  if (m_esLightError.GetLength() > 2) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else {
    xlStatus = m_pMOST->MOSTGenerateLightError(nRepeat);
    if (xlStatus == XL_ERR_NO_LICENSE) {
      AfxMessageBox("ERROR: No Licence", MB_ICONSTOP);
    }
  }
}

void CMOSTGeneralTest::OnBnClickedGenLock()
{
  XLstatus xlStatus;

  UpdateData(TRUE);

  unsigned short nRepeat = (unsigned short) atoi(m_esLockError);

  if (m_esLockError.GetLength() > 2) 
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  else {
     xlStatus = m_pMOST->MOSTGenerateLockError(nRepeat);
     if (xlStatus == XL_ERR_NO_LICENSE) {
      AfxMessageBox("ERROR: No Licence", MB_ICONSTOP);
    }
  }
}

