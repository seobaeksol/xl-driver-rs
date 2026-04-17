/*-------------------------------------------------------------------------------------------
| File        : xlMOST150NodeConfig.cpp
| Project     : Vector MOST150 Example 
|
| Description : Node Config Dialog box implementation
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlMOST150View.h"
#include "xlMOST150NodeConfig.h"
#include "xlMOST150Functions.h"
#include "vxlapi.h"

// CMOST150NodeConfig dialog

IMPLEMENT_DYNAMIC(CMOST150NodeConfig, CDialog)
CMOST150NodeConfig::CMOST150NodeConfig(CWnd* pParent /*=NULL*/)
  : CDialog(CMOST150NodeConfig::IDD, pParent)
  , m_strNodeAdr(_T(""))
  , m_esGroupAdr(_T(""))
  , m_esNodeAdr(_T(""))
{
}

CMOST150NodeConfig::~CMOST150NodeConfig()
{
}

void CMOST150NodeConfig::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_FREQUENCY, m_cFrequency);
  DDX_Control(pDX, IDC_DEVICE_MODE_SLAVE, m_rDeviceModeSlave);
  DDX_Control(pDX, IDC_DEVICE_MODE_MASTER, m_rDeviceModeMaster);
  DDX_Text(pDX, IDC_GROUP_ADR, m_esGroupAdr);
  DDX_Text(pDX, IDC_NODE_ADR, m_esNodeAdr);
}


BEGIN_MESSAGE_MAP(CMOST150NodeConfig, CDialog)
  ON_BN_CLICKED(IDC_SET_PARAMETER, OnBnClickedSetParameter)
//  ON_WM_INITMENU()
END_MESSAGE_MAP()

// MOSTNodeConfig message handlers

BOOL CMOST150NodeConfig::OnInitDialog() 
{
  CDialog::OnInitDialog();
  char str[100];
   
  // update the Dialogbox
  m_cFrequency.SetCurSel(m_pMOST150->m_NodeParam[m_nChan].nFrequency); // 0 -> 44,1kHz, 1 -> 48kHz, :-).
  
  if (m_pMOST150->m_NodeParam[m_nChan].nDeviceMode == XL_MOST150_DEVICEMODE_MASTER) {
    m_rDeviceModeMaster.SetCheck(BST_CHECKED);
  }
  else {
    m_rDeviceModeSlave.SetCheck(BST_CHECKED);
  }
 
  sprintf(str, "0x%04x", m_pMOST150->m_NodeParam[m_nChan].nNodeAdr);  
  m_esNodeAdr  = str;

  sprintf(str, "0x%04x", m_pMOST150->m_NodeParam[m_nChan].nGroupAdr);
  m_esGroupAdr = str;

  UpdateData(FALSE);

  return TRUE;   // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


void CMOST150NodeConfig::OnBnClickedSetParameter()
{
  unsigned int nDeviceMode = XL_MOST150_DEVICEMODE_SLAVE;
  unsigned int nFrequency  = XL_MOST150_FREQUENCY_44100;

  nDeviceMode = GetCheckedRadioButton(IDC_DEVICE_MODE_MASTER, IDC_DEVICE_MODE_SLAVE);

  if(nDeviceMode == IDC_DEVICE_MODE_SLAVE) {
    nDeviceMode = XL_MOST150_DEVICEMODE_SLAVE;
  }
  else if (nDeviceMode == IDC_DEVICE_MODE_MASTER) {
    nDeviceMode = XL_MOST150_DEVICEMODE_MASTER;
  }

  UpdateData(TRUE);

  if (m_cFrequency.GetCurSel() == 0) nFrequency = XL_MOST150_FREQUENCY_44100;
  else if (m_cFrequency.GetCurSel() == 1) nFrequency = XL_MOST150_FREQUENCY_48000;

  unsigned short nGroupAdr = m_General.StrToShort(m_esGroupAdr.GetBuffer(1));
  unsigned short nNodeAdr  = m_General.StrToShort(m_esNodeAdr.GetBuffer(1));

  // Check the parameter
  if ( (m_esGroupAdr.GetLength() > 6) || (m_esNodeAdr.GetLength() > 6) ) {
    AfxMessageBox("ERROR: Wrong parameter", MB_ICONSTOP);
  }
  else {
    m_pMOST150->MOST150SetupNode(nDeviceMode, nFrequency, nNodeAdr, nGroupAdr);
  }
}

