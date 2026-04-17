/*-------------------------------------------------------------------------------------------
| File        : xlFrTxFrameDlg.cpp
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xlFlexDemo.h"
#include "xlFrTxFrameDlg.h"
#include "debug.h"

// CTxFrameDlg dialog

IMPLEMENT_DYNAMIC(CTxFrameDlg, CDialog)

CTxFrameDlg::CTxFrameDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTxFrameDlg::IDD, pParent)
  , m_eSlotId(_T(""))
  , m_eOffset(_T(""))
  , m_cTxAkn(TRUE)
  , m_cPayloadInc(TRUE)
  , m_eData(_T(""))
{

}

CTxFrameDlg::~CTxFrameDlg()
{
}

void CTxFrameDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_CHANNEL, m_lChannel);
  DDX_Control(pDX, IDC_REPETITION, m_lRepetition);
  DDX_Text(pDX, IDC_ID, m_eSlotId);
  DDX_Text(pDX, IDC_OFFSET, m_eOffset);
  DDX_Check(pDX, IDC_TXAKN, m_cTxAkn);
  DDX_Check(pDX, IDC_PAYLOADINC, m_cPayloadInc);
  DDX_Text(pDX, IDC_DATA, m_eData);
  DDX_Control(pDX, IDC_PAYLOADINC, m_ccPayloadInc);
  DDX_Control(pDX, IDC_TXMODE, m_lTxMode);
}


BEGIN_MESSAGE_MAP(CTxFrameDlg, CDialog)
  ON_BN_CLICKED(IDC_ADDTXFRAME, &CTxFrameDlg::OnBnClickedAddtxframe)
  ON_BN_CLICKED(IDC_BTN_ADD_128FRAMES, &CTxFrameDlg::OnBnClickedBtnAdd128frames)
END_MESSAGE_MAP()


BOOL CTxFrameDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

  // setup a default TxFrame
  m_eOffset     = "0";
  m_eSlotId     = "42";
  m_eData       = "01020304";
  m_lChannel.SetCurSel(0);     // default: Channel A
  m_lRepetition.SetCurSel(2);  // default: Rep 4
  m_lTxMode.SetCurSel(0);

  if (m_pFr->m_lic) {
    m_ccPayloadInc.EnableWindow(TRUE);
    m_cPayloadInc = TRUE;
  }
  else {
    m_ccPayloadInc.EnableWindow(FALSE);
    m_cPayloadInc = FALSE;
  }
  
  UpdateData(FALSE);

  return TRUE;  // return TRUE  unless you set the focus to a control
}

// CTxFrameDlg message handlers

void CTxFrameDlg::OnBnClickedAddtxframe()
{

  XLstatus        xlStatus;
  unsigned char   payload[XL_FR_MAX_DATA_LENGTH]; // here we have bytes
  char            tmp[100];
  unsigned char   dataLength;
  unsigned short  flagsChip;
  unsigned short  txFlags = 0;
  unsigned char   txMode;
  unsigned char   payloadInc = 0;
  unsigned char   cRepetition[7] = {1, 2, 4, 8, 16, 32, 64};
  unsigned char   repetition;
  unsigned int    cFlagsChip;
  CString         sData;

  UpdateData(TRUE);
  
  // -------------------------------------------
  // map the GUI parameter to the XL API values
  // -------------------------------------------
  // get the payload data
  dataLength = (unsigned char)m_eData.GetLength();

  // build the payload data
  for (int i=0; i<(dataLength/2); i++) {
    sData = m_eData.Mid((i*2),2);
    payload[i] = (unsigned char) _tcstoul(sData, 0, 16);
    sprintf(tmp, "payload[%d]: %d",i, payload[i]);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  // generate the flagsChip parameter
  cFlagsChip = m_lChannel.GetCurSel();
  if (cFlagsChip == 0) {
    flagsChip = XL_FR_CHANNEL_A;
  }
  else if (cFlagsChip == 1) {
    flagsChip = XL_FR_CHANNEL_B;
  }
  else if (cFlagsChip == 2) {
    flagsChip = XL_FR_CHANNEL_AB;
  }
  else {
    return;
  }

  if (m_cTxAkn) txFlags |= XL_FR_FRAMEFLAG_REQ_TXACK;

  if (m_cPayloadInc) payloadInc = XL_FR_PAYLOAD_INCREMENT_8BIT;

  // build the txMode value
  txMode = (unsigned char)m_lTxMode.GetCurSel();
  if (txMode == 0) {
    txMode = XL_FR_TX_MODE_CYCLIC;
  }
  else if (txMode == 1) {
    txMode = XL_FR_TX_MODE_SINGLE_SHOT;
  }
  else {
    txMode = XL_FR_TX_MODE_NONE;
  }

  // read the repetition
  repetition = cRepetition[m_lRepetition.GetCurSel()];

  // -------------------------------------------
  // Add the TX frame
  // -------------------------------------------
  xlStatus = m_pFr->FrAddTxFrame(m_nChan, 
                                 flagsChip,
                                 txFlags,
                                 txMode,
                                 payloadInc,
                                 (unsigned short) atoi(m_eSlotId),
                                 (unsigned char) atoi(m_eOffset),
                                 repetition,
                                 dataLength/4, // two chars interpreted as hex and FR calculates in 16 Bit WORDS!! 
                                 payload);

  if (xlStatus != XL_SUCCESS) {
    AfxMessageBox("ERROR: Add TxFrame", MB_ICONERROR);
  }
  else {
    OnOK();
  }

}

void CTxFrameDlg::OnBnClickedBtnAdd128frames()
{
  XLstatus        xlStatus = XL_SUCCESS;
  unsigned char   payload[XL_FR_MAX_DATA_LENGTH]; // here we have bytes
  char            tmp[100];
  unsigned char   dataLength;
  unsigned short  flagsChip;
  unsigned short  txFlags = 0;
  unsigned char   txMode;
  unsigned char   payloadInc = 0;
  unsigned char   cRepetition[7] = {1, 2, 4, 8, 16, 32, 64};
  unsigned char   repetition;
  unsigned char   offset;
  unsigned int    cFlagsChip;
  unsigned short  slotid;
  CString         sData;

  UpdateData(TRUE);
  
  // -------------------------------------------
  // map the GUI parameter to the XL API values
  // -------------------------------------------
  // get the payload data
  dataLength = (unsigned char)m_eData.GetLength();

  // build the payload data
  for (int i=0; i<(dataLength/2); i++) {
    sData = m_eData.Mid((i*2),2);
    payload[i] = (unsigned char) _tcstoul(sData, 0, 16);
    sprintf(tmp, "payload[%d]: %d",i, payload[i]);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  // generate the flagsChip parameter
  cFlagsChip = m_lChannel.GetCurSel();
  if (cFlagsChip == 0) {
    flagsChip = XL_FR_CHANNEL_A;
  }
  else if (cFlagsChip == 1) {
    flagsChip = XL_FR_CHANNEL_B;
  }
  else if (cFlagsChip == 2) {
    flagsChip = XL_FR_CHANNEL_AB;
  }
  else {
    return;
  }

  if (m_cTxAkn) txFlags |= XL_FR_FRAMEFLAG_REQ_TXACK;

  if (m_cPayloadInc) payloadInc = XL_FR_PAYLOAD_INCREMENT_8BIT;

  // build the txMode value
  txMode = (unsigned char)m_lTxMode.GetCurSel();
  if (txMode == 0) {
    txMode = XL_FR_TX_MODE_CYCLIC;
  }
  else if (txMode == 1) {
    txMode = XL_FR_TX_MODE_SINGLE_SHOT;
  }
  else {
    txMode = XL_FR_TX_MODE_NONE;
  }

  // read the repetition
  repetition = cRepetition[m_lRepetition.GetCurSel()];

  offset = (unsigned char) atoi(m_eOffset);
  slotid = (unsigned short) atoi(m_eSlotId);

  // -------------------------------------------
  // Add 128 TX frames
  // -------------------------------------------
  for (int cnt=0; (cnt < 130) && (xlStatus == XL_SUCCESS); cnt++) {
    xlStatus = m_pFr->FrAddTxFrame(m_nChan, 
                                   flagsChip,
                                   txFlags,
                                   txMode,
                                   payloadInc,
                                   slotid,
                                   offset,
                                   repetition,
                                   dataLength/4, // two chars interpreted as hex and FR calculates in 16 Bit WORDS!! 
                                   payload);

    if (xlStatus != XL_SUCCESS) {
      sprintf(tmp, "ERROR: Add TxFrame with slotID %d failed!", slotid);
      AfxMessageBox(tmp, MB_ICONERROR);
    }
    else {
      slotid++;
    }
  }

  if (xlStatus == XL_SUCCESS) {
    sprintf(tmp, "Last TxFrame added has slotID %d.\n", slotid);
    XLDEBUG(DEBUG_ADV, tmp);

    OnOK();
  }

}
