/*-------------------------------------------------------------------------------------------
| File        : xlMOSTParseEvent.cpp
| Project     : Vector MOST Example 
|
| Description : Node Config Dialog box implementation
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2005 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlMOSTView.h"
#include "xlMOSTNodeConfig.h"
#include "xlMOSTFunctions.h"
#include ".\xlmostnodeconfig.h"
#include "vxlapi.h"

// CMOSTNodeConfig dialog

IMPLEMENT_DYNAMIC(CMOSTNodeConfig, CDialog)
CMOSTNodeConfig::CMOSTNodeConfig(CWnd* pParent /*=NULL*/)
	: CDialog(CMOSTNodeConfig::IDD, pParent)
  , m_strNodeAdr(_T(""))
  , m_esGroupAdr(_T(""))
  , m_esNodeAdr(_T(""))
{
}

CMOSTNodeConfig::~CMOSTNodeConfig()
{
}

void CMOSTNodeConfig::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_FREQUENCY, m_cFrequency);
  DDX_Control(pDX, IDC_TIMING_SLAVE, m_rSlaveTiming);
  DDX_Control(pDX, IDC_TIMING_MASTER, m_rMasterTiming);
  DDX_Text(pDX, IDC_GROUP_ADR, m_esGroupAdr);
  DDX_Text(pDX, IDC_NODE_ADR, m_esNodeAdr);
  DDX_Control(pDX, IDC_BYPASS, m_cBypass);
}


BEGIN_MESSAGE_MAP(CMOSTNodeConfig, CDialog)
  ON_BN_CLICKED(IDC_SET_PARAMETER, OnBnClickedSetParameter)
//  ON_WM_INITMENU()
END_MESSAGE_MAP()

// MOSTNodeConfig message handlers

BOOL CMOSTNodeConfig::OnInitDialog() 
{
  CDialog::OnInitDialog();
  char str[100];
   
  // update the Dialogbox
  m_cFrequency.SetCurSel(m_pMOST->m_NodeParam[m_nChan].nFrequency); // 0 -> 44,1kHz, 1 -> 48kHz, :-).
  m_cBypass.SetCurSel(m_pMOST->m_NodeParam[m_nChan].nBypass);
  if (m_pMOST->m_NodeParam[m_nChan].nTimingMode == XL_MOST_TIMING_MASTER) {
    m_rMasterTiming.SetCheck(BST_CHECKED);
  }
  else {
    m_rSlaveTiming.SetCheck(BST_CHECKED);
  }
 
  sprintf(str, "0x%04x", m_pMOST->m_NodeParam[m_nChan].nNodeAdr);  
  m_esNodeAdr  = str;

  sprintf(str, "0x%04x", m_pMOST->m_NodeParam[m_nChan].nGroupAdr);
  m_esGroupAdr = str;

  UpdateData(FALSE);

  return TRUE;   // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CMOSTNodeConfig::OnBnClickedSetParameter()
{
  unsigned int nTimingMode = XL_MOST_TIMING_SLAVE;
  unsigned int nFrequency  = XL_MOST_FREQUENCY_44100;

  nTimingMode = GetCheckedRadioButton(IDC_TIMING_MASTER, IDC_TIMING_SLAVE);

  if (nTimingMode == IDC_TIMING_SLAVE) {
    nTimingMode = XL_MOST_TIMING_SLAVE;
  }
  else if (nTimingMode == IDC_TIMING_MASTER) {
    nTimingMode = XL_MOST_TIMING_MASTER;
  }

  UpdateData(TRUE);

  if (m_cFrequency.GetCurSel() == 0) {
    nFrequency = XL_MOST_FREQUENCY_44100;
  }
  else if (m_cFrequency.GetCurSel() == 1) {
    nFrequency = XL_MOST_FREQUENCY_48000;
  }

  unsigned short nGroupAdr = m_General.StrToShort(m_esGroupAdr.GetBuffer(1));
  unsigned short nNodeAdr  = m_General.StrToShort(m_esNodeAdr.GetBuffer(1));
  unsigned char nBypass = (unsigned char) m_cBypass.GetCurSel();

  // Check the parameter
  if ( (m_esGroupAdr.GetLength() > 6) || (m_esNodeAdr.GetLength() > 6) ) {
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  }
  else {
    m_pMOST->MOSTSetupNode(nTimingMode, nFrequency, nNodeAdr, nGroupAdr, nBypass);
  }
}
