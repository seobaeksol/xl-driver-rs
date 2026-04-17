// xlMOST150Streaming.cpp : implementation file
//

#include "stdafx.h"
#include "xlMOST150View.h"
#include "xlMOST150Streaming.h"
#include "debug.h"

const unsigned int XL_STREAM_BUFFER_SIZE = 1048576; // 1 MB

///////////////////////////////////////////////////////////////////////////////
// CMOST150StreamingDlg
///////////////////////////////////////////////////////////////////////////////
class CMOST150StreamingDlg : public CDialog
{
  DECLARE_DYNAMIC(CMOST150StreamingDlg)

public:
  CMOST150StreamingDlg(std::vector<XL_MOST150_CL_INFO>* pCLInfo, CWnd* pParent = NULL);
  virtual ~CMOST150StreamingDlg();

// Dialog Data
  enum { IDD = IDD_STREAM_CONFIG };

protected:
  virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
  BOOL OnInitDialog();
  void OnOK();

  DECLARE_MESSAGE_MAP()

public:
  unsigned int m_AllocTableIndex;

private:
  CListBox             m_CLInfo;
  std::vector<XL_MOST150_CL_INFO>* m_pCLInfo;
};

IMPLEMENT_DYNAMIC(CMOST150StreamingDlg, CDialog)
CMOST150StreamingDlg::CMOST150StreamingDlg(std::vector<XL_MOST150_CL_INFO>* pCLInfo, CWnd* pParent /*= NULL*/)
: CDialog(CMOST150StreamingDlg::IDD, pParent),
  m_AllocTableIndex(0),
  m_pCLInfo(pCLInfo)
{
}

CMOST150StreamingDlg::~CMOST150StreamingDlg()
{
}

void CMOST150StreamingDlg::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_CL_LIST, m_CLInfo);
}

BOOL CMOST150StreamingDlg::OnInitDialog()
{
  CString cstr;
  CDialog::OnInitDialog();

  m_AllocTableIndex = 0;
  std::vector<XL_MOST150_CL_INFO>::iterator it = m_pCLInfo->begin();
  while(it != m_pCLInfo->end())
  {
    cstr.Format("CL: 0x%X, Width: %d", (*it).label, (*it).channelWidth);
    m_CLInfo.AddString(cstr.GetBuffer());
    ++it;
  }

  UpdateData(FALSE);

  return TRUE;   // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CMOST150StreamingDlg::OnOK()
{
  m_AllocTableIndex = m_CLInfo.GetCurSel();
  CDialog::OnOK();
}

BEGIN_MESSAGE_MAP(CMOST150StreamingDlg, CDialog)
END_MESSAGE_MAP()



///////////////////////////////////////////////////////////////////////////////
// CMOST150Streaming
///////////////////////////////////////////////////////////////////////////////

CMOST150Streaming::CMOST150Streaming() 
{
  m_pStreamBuffer = new unsigned char[XL_STREAM_BUFFER_SIZE];
  memset(m_pStreamBuffer, 0x00, XL_STREAM_BUFFER_SIZE);
  m_AllocTable.clear();
  m_streamHandle = XL_MOST150_STREAM_INVALID_HANDLE;
  m_pFile = 0;
  m_pAllocInfo = 0;
  m_InitRxFifo = true;

}

CMOST150Streaming::~CMOST150Streaming() 
{
  if(m_pStreamBuffer)
  {
    delete [] m_pStreamBuffer;
    m_pStreamBuffer = NULL;
  }
    
}

XLstatus CMOST150Streaming::MOST150StreamStart(unsigned int nChan) 
{
  XLstatus xlStatus = XL_SUCCESS;
  char            tmp[100];

  m_channel = nChan;
  m_streamHandle = XL_MOST_STREAM_INVALID_HANDLE;

  // check for available CL
  CMOST150StreamingDlg dlg(&m_AllocTable);
  INT_PTR nResponse = dlg.DoModal();
  if (nResponse == IDOK && dlg.m_AllocTableIndex != 0xFFFFFFFF)
  {
    // open the binary file
    if ((m_pFile = fopen("most150.bin", "wb")) == NULL){
      XLDEBUG(DEBUG_ADV, "Error opening file");
      return XL_ERROR;
    }

    if(m_InitRxFifo)
    {
      // init Rx FIFO (has only to be done once!)
      xlStatus = xlMost150StreamInitRxFifo(m_NodeParam[m_channel].xlPortHandle, 
                                           m_NodeParam[m_channel].xlChannelMask);
      if(xlStatus == XL_SUCCESS)
      {
        m_InitRxFifo = false;
      }

      sprintf(tmp, "xlMost150StreamInitRxFifo, PH: %d, stat: %d", 
                    m_NodeParam[m_channel].xlPortHandle, xlStatus);
      XLDEBUG(DEBUG_ADV, tmp);
    }

    // open stream
    m_pAllocInfo = &m_AllocTable[dlg.m_AllocTableIndex];

    m_xlMost150StreamOpen.numBytesPerFrame = m_pAllocInfo->channelWidth;
    m_xlMost150StreamOpen.direction        = XL_MOST150_STREAM_RX_DATA; 
    m_xlMost150StreamOpen.latency          = XL_MOST150_STREAM_LATENCY_MEDIUM;
    m_xlMost150StreamOpen.pStreamHandle    = &m_streamHandle;

    xlStatus = xlMost150StreamOpen(m_NodeParam[m_channel].xlPortHandle, 
                                   m_NodeParam[m_channel].xlChannelMask, 
                                   0xab, 
                                   &m_xlMost150StreamOpen);

    m_streamHandle = *m_xlMost150StreamOpen.pStreamHandle;

    sprintf(tmp, "xlMostStreamOpen, PH: %d, streamH: 0x%x, stat: %d", 
                  m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
  }
  else
  {
    xlStatus = XL_ERROR;
  }

  return xlStatus;
}

XLstatus CMOST150Streaming::MOST150StreamStop()
{
  XLstatus xlStatus = XL_SUCCESS;
  char     tmp[100];

  xlStatus = xlMost150StreamStop(m_NodeParam[m_channel].xlPortHandle, 
                                 m_NodeParam[m_channel].xlChannelMask, 
                                 0xab,
                                 m_streamHandle);
  sprintf(tmp, "xlMostStreamStop, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

XLstatus CMOST150Streaming::MOST150StreamParseEvent(XLmost150event *pxlMost150Event)
{
  XLstatus xlStatus = XL_SUCCESS;
  char     tmp[100];

  switch(pxlMost150Event->tag)
  {
    case XL_MOST150_SPECIAL_NODE_INFO:
      {
        if (pxlMost150Event->tagData.mostSpecialNodeInfo.changeMask & XL_MOST150_INIC_NISTATE_CHANGED) {
          if(pxlMost150Event->tagData.mostSpecialNodeInfo.inicNIState == XL_MOST150_INIC_NISTATE_NET_OFF) {
            m_AllocTable.clear(); // any CL automatically removed
          }
        }
      }
      break;

    case XL_MOST150_SYNC_ALLOC_INFO:
      {
        XL_MOST150_CL_INFO* pInfo = &pxlMost150Event->tagData.mostSyncAllocInfo.allocTable[0];
        unsigned int size = (pxlMost150Event->size - XL_MOST150_RX_EVENT_HEADER_SIZE) / sizeof(XL_MOST150_CL_INFO);
        bool bFound = false;
        
        // application requested ALLOC INFO explicitly
        if(pxlMost150Event->userHandle != 0)
          m_AllocTable.clear();


        for(unsigned int i=0; i<size; ++i)
        {
          std::vector<XL_MOST150_CL_INFO>::iterator it = m_AllocTable.begin();
          while(it != m_AllocTable.end())
          {
            if( (*it).label == pInfo[i].label)
            {
              bFound = true;
              break;
            }

            ++it;
          }

          if(!bFound)
          {
            if(pInfo[i].channelWidth > 0)
              m_AllocTable.push_back(pInfo[i]);
          }
          else
          {
            if(pInfo[i].channelWidth > 0)
            {
              (*it).channelWidth += pInfo[i].channelWidth;
            }
            else
            {
              m_AllocTable.erase(it);
            }
          }

        }
      }
      break;

    case XL_MOST150_STREAM_STATE:
      {
        switch (pxlMost150Event->tagData.mostStreamState.streamState)
        {
          case XL_MOST150_STREAM_STATE_OPENED:
            {
              sprintf(tmp, "XL_MOST150_STREAM_STATE: XL_MOST150_STREAM_STATE_OPENED");
              XLDEBUG(DEBUG_ADV, tmp);
              mostStreamStart();
            }
            break;

          case XL_MOST150_STREAM_STATE_STARTED:
            {
              sprintf(tmp, "XL_MOST150_STREAM_STATE: XL_MOST150_STREAM_STATE_STARTED");
              XLDEBUG(DEBUG_ADV, tmp);
              break;
            }
          
          case XL_MOST150_STREAM_STATE_STOPPED:
            {
              sprintf(tmp, "XL_MOST150_STREAM_STATE: XL_MOST150_STREAM_STATE_STOPPED");
              XLDEBUG(DEBUG_ADV, tmp);
              mostStreamClose();
              break;
            }

          case XL_MOST150_STREAM_STATE_CLOSED:
            {
              sprintf(tmp, "XL_MOST150_STREAM_STATE: XL_MOST150_STREAM_STATE_CLOSED");
              XLDEBUG(DEBUG_ADV, tmp);
              fclose(m_pFile);
              break;
            }
        }
      }
      break;

    case XL_MOST150_STREAM_RX_BUFFER:
      {
        unsigned int numOfBytes, numBytesToRead;

        numOfBytes = pxlMost150Event->tagData.mostStreamRxBuffer.numberOfBytes;
        numBytesToRead = numOfBytes;
        xlStatus = XL_SUCCESS;        

        while(numBytesToRead > 0 && xlStatus == XL_SUCCESS)
        {
          if(numOfBytes > XL_STREAM_BUFFER_SIZE)
          {
            numOfBytes = XL_STREAM_BUFFER_SIZE;
          }

          xlStatus = xlMost150StreamReceiveData(m_NodeParam[m_channel].xlPortHandle, 
                                                m_NodeParam[m_channel].xlChannelMask,
                                                m_pStreamBuffer,
                                                &numOfBytes);
          if(xlStatus == XL_SUCCESS)
          {
            if(m_pFile)
            {
              fwrite(m_pStreamBuffer,
                     sizeof(unsigned char), 
                     numOfBytes,
                     m_pFile);
            }

            numBytesToRead -= numOfBytes;
            numOfBytes = numBytesToRead;
          }
        }
      }
      break;
  }

  return xlStatus;
}

XLstatus CMOST150Streaming::mostStreamStart()
{
  XLstatus xlStatus = XL_SUCCESS;
  char     tmp[100];
  unsigned int connLabels[XL_MOST150_STREAM_RX_NUM_CL_MAX];

  memset(connLabels, 0xFF, sizeof(connLabels));
  connLabels[0] = m_pAllocInfo->label;

  xlStatus = xlMost150StreamStart(m_NodeParam[m_channel].xlPortHandle, 
                                  m_NodeParam[m_channel].xlChannelMask, 
                                  0xab,
                                  m_streamHandle,
                                  1,
                                  connLabels);
  sprintf(tmp, "xlMost150StreamStart, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

XLstatus CMOST150Streaming::mostStreamClose()
{
  XLstatus xlStatus = XL_SUCCESS;
  char     tmp[100];

  xlStatus = xlMost150StreamClose(m_NodeParam[m_channel].xlPortHandle, 
                                  m_NodeParam[m_channel].xlChannelMask, 
                                  0xab,
                                  m_streamHandle);
  sprintf(tmp, "xlMost150StreamClose, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}




