/*-------------------------------------------------------------------------------------------
| File        : xlMOSTStreaming.cpp
| Project     : Vector MOST Example
|
| Description : Module to handle the MOST streaming
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2008 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include ".\xlMOSTStreaming.h"
#include "debug.h"

CMOSTStreaming::CMOSTStreaming()
{
}

CMOSTStreaming::~CMOSTStreaming(void)
{
}

////////////////////////////////////////////////////////////////////////////

//! mostStreamStop

//! 
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTStreaming::mostStreamStop()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  for (unsigned int i=0; i<m_nBuffer;i++) {
    xlStatus = xlMostStreamBufferSetNext( m_NodeParam[m_channel].xlPortHandle,
                                          m_NodeParam[m_channel].xlChannelMask,
                                          0xab,
                                          m_streamHandle,
                                          m_pStreamBuffer[i],
                                          m_bufferSize[i]);

    sprintf(tmp, "xlMostStreamBufferSetNext, bufferS: %d, p:%p, stat: %d",
                  m_bufferSize[i], 
                  m_pStreamBuffer[i], 
                  xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);
  }

  xlStatus = xlMostStreamBufferDeallocateAll( m_NodeParam[m_channel].xlPortHandle, 
                                              m_NodeParam[m_channel].xlChannelMask, 
                                              0xab,
                                              m_streamHandle);

  sprintf(tmp, "xlMostStreamBufferDeallocateAll, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  xlStatus = xlMostStreamClose (  m_NodeParam[m_channel].xlPortHandle, 
                                  m_NodeParam[m_channel].xlChannelMask, 
                                  0xab,
                                  m_streamHandle);
  sprintf(tmp, "xlMostStreamClose, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  
  return xlStatus;

}

////////////////////////////////////////////////////////////////////////////

//! mostStreamClose

//! 
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTStreaming::mostStreamClose()
{
            
  // close our most file
  fclose(m_pFile);

  return XL_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////

//! mostStreamAlloc

//! allocate the streaming memory and start streaming
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTStreaming::mostStreamAlloc()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  unsigned char   syncChannels[60];

  // defines the audio channels for streaming
  syncChannels[0] = 0;
  syncChannels[1] = 1;
  syncChannels[2] = 2;
  syncChannels[3] = 3;

  // -------------------------------------------------
  // at first we allocate the buffer within the driver
  // -------------------------------------------------
  m_nBuffer = 0;

  xlStatus = xlMostStreamBufferAllocate(m_NodeParam[m_channel].xlPortHandle,
                                        m_NodeParam[m_channel].xlChannelMask,
                                        0xab,
                                        m_streamHandle, 
                                        &m_pStreamBuffer[m_nBuffer], 
                                        &m_bufferSize[m_nBuffer]);
  sprintf(tmp, "xlMostStreamBufferAllocate, bufferSize: %d, stat: %d", m_bufferSize[m_nBuffer], xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  while(xlStatus == XL_SUCCESS) {
    xlStatus = xlMostStreamBufferAllocate(m_NodeParam[m_channel].xlPortHandle,
                                          m_NodeParam[m_channel].xlChannelMask,
                                          0xab,
                                          m_streamHandle, 
                                          &m_pStreamBuffer[m_nBuffer], 
                                          &m_bufferSize[m_nBuffer]);
    sprintf(tmp, "xlMostStreamBufferAllocate, bufferS: %d, stat: %d", 
                  m_bufferSize[m_nBuffer], 
                  xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);

    m_nBuffer++;
  }

  sprintf(tmp, "xlMostStreamBufferAllocate, nBuffer: %d", m_nBuffer);
  XLDEBUG(DEBUG_ADV, tmp);

  // ------------------------------------------------- 

  for (unsigned int i=0; i<m_nBuffer;i++) {
    xlStatus = xlMostStreamBufferSetNext( m_NodeParam[m_channel].xlPortHandle,
                                          m_NodeParam[m_channel].xlChannelMask,
                                          0xab,
                                          m_streamHandle,
                                          m_pStreamBuffer[i],
                                          m_bufferSize[i]);

    sprintf(tmp, "xlMostStreamBufferSetNext, bufferS: %d, p:%p, stat: %d", 
                  m_bufferSize[i], 
                  m_pStreamBuffer[i],
                  xlStatus);
    XLDEBUG(DEBUG_ADV, tmp);

  }

  // -------------------------------------------------
  // tell the driver to stream
  // -------------------------------------------------

  xlStatus = xlMostStreamStart( m_NodeParam[m_channel].xlPortHandle,
                                m_NodeParam[m_channel].xlChannelMask,
                                0xab,
                                m_streamHandle,
                                syncChannels);

  sprintf(tmp, "xlMostStreamStart, PH: %d, stat: %d", m_NodeParam[m_channel].xlPortHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTStreamInit

//! 
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTStreaming::MOSTStreamInit(unsigned int nChan)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  // open the binary file
  if ((m_pFile = fopen("most.bin", "wb")) == NULL){
    XLDEBUG(DEBUG_ADV, "Error opening file");
    return XL_ERROR;
  }

  m_channel = nChan;
  m_streamHandle = XL_MOST_STREAM_INVALID_HANDLE;

  m_xlMostStreamOpen.numSyncChannels  = 4;
  m_xlMostStreamOpen.direction        = XL_MOST_STREAM_RX_DATA; 
  m_xlMostStreamOpen.options          = 0;
  m_xlMostStreamOpen.latency          = 2;
  m_xlMostStreamOpen.pStreamHandle    = &m_streamHandle;

  xlStatus = xlMostStreamOpen(m_NodeParam[m_channel].xlPortHandle, 
                              m_NodeParam[m_channel].xlChannelMask, 
                              0xab, 
                              &m_xlMostStreamOpen);

  m_streamHandle = *m_xlMostStreamOpen.pStreamHandle;

  sprintf(tmp, "xlMostStreamOpen, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTStreamClose

//! 
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTStreaming::MOSTStreamClose()
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];

  xlStatus = xlMostStreamStop( m_NodeParam[m_channel].xlPortHandle, 
                               m_NodeParam[m_channel].xlChannelMask, 
                               0xab,
                               m_streamHandle);
  sprintf(tmp, "xlMostStreamStop, PH: %d, streamH: 0x%x, stat: %d", 
                m_NodeParam[m_channel].xlPortHandle, m_streamHandle, xlStatus);
  XLDEBUG(DEBUG_ADV, tmp);

  return xlStatus;
}

////////////////////////////////////////////////////////////////////////////

//! MOSTStreamParse

//! 
//! 
//!
//////////////////////////////////////////////////////////////////////////// 

XLstatus CMOSTStreaming::MOSTStreamParse(XLmostEvent *pxlMostEvent)
{
  XLstatus        xlStatus = XL_ERROR;
  char            tmp[100];
 
  switch (pxlMostEvent->tag) {

    case XL_MOST_STREAM_BUFFER: {
      unsigned int wBytes;
      unsigned char   *pBuffer = pxlMostEvent->tagData.mostStreamBuffer.pBuffer;

#ifdef _WIN64
  pBuffer = (unsigned char*)((((DWORD64)pxlMostEvent->tagData.mostStreamBuffer.pBuffer_highpart)<<32)|
    (DWORD64)pxlMostEvent->tagData.mostStreamBuffer.pBuffer);
#endif                                
      sprintf(tmp, "XL_MOST_STREAM_BUFFER: validBytes:%d, stat:0x%x",
       pxlMostEvent->tagData.mostStreamBuffer.validBytes,
       pxlMostEvent->tagData.mostStreamBuffer.status);
      XLDEBUG(DEBUG_ADV, tmp);

      wBytes = (unsigned int)fwrite(pxlMostEvent->tagData.mostStreamBuffer.pBuffer, 
                      sizeof(unsigned char), 
                      pxlMostEvent->tagData.mostStreamBuffer.validBytes, m_pFile );

      memset(pxlMostEvent->tagData.mostStreamBuffer.pBuffer,
             0,
             pxlMostEvent->tagData.mostStreamBuffer.validBytes);
       
      xlStatus = xlMostStreamBufferSetNext( m_NodeParam[m_channel].xlPortHandle,
                                            m_NodeParam[m_channel].xlChannelMask,
                                            0xab,
                                            m_streamHandle,
                                            pBuffer,
                                            0); // only for RX streaming

      sprintf(tmp, "xlMostStreamBufferSetNext, stat: %d", xlStatus);
      XLDEBUG(DEBUG_ADV, tmp);

     
      break;
    }

    case XL_MOST_STREAM_STATE: {
      
      switch (pxlMostEvent->tagData.mostStreamState.streamState) {

        case XL_MOST_STREAM_STATE_OPENED: {
          sprintf(tmp, "XL_MOST_STREAM_STATE: XL_MOST_STREAM_STATE_OPENED");
          XLDEBUG(DEBUG_ADV, tmp);
          mostStreamAlloc();
          break;
        }

        case XL_MOST_STREAM_STATE_STARTED: {
          sprintf(tmp, "XL_MOST_STREAM_STATE: XL_MOST_STREAM_STATE_STARTED");
          XLDEBUG(DEBUG_ADV, tmp);
          break;
        }
        
        case XL_MOST_STREAM_STATE_STOPPED: {
          sprintf(tmp, "XL_MOST_STREAM_STATE: XL_MOST_STREAM_STATE_STOPPED");
          XLDEBUG(DEBUG_ADV, tmp);
          mostStreamStop();
          break;
        }

        case XL_MOST_STREAM_STATE_CLOSED: {
          sprintf(tmp, "XL_MOST_STREAM_STATE: XL_MOST_STREAM_STATE_CLOSED");
          XLDEBUG(DEBUG_ADV, tmp);
          mostStreamClose();
          break;
        }

        break;
      }

    }
  }

  return xlStatus;
}