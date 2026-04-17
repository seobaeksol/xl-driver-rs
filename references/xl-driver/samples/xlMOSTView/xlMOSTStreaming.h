#pragma once

#include "vxlapi.h"
#include ".\xlmostparam.h"

#define XL_MAX_STREAM_BUFFER              10
#define XL_MAX_BUFFER_SIZE        4294967296

class CMOSTStreaming : public virtual CMOSTParam
{
public:
  CMOSTStreaming();
  ~CMOSTStreaming(void);

 XLstatus MOSTStreamInit  (unsigned int nChan);
 XLstatus MOSTStreamClose();
 XLstatus MOSTStreamParse (XLmostEvent *pxlMostEvent);

private:
 
  XLstatus mostStreamAlloc();
  XLstatus mostStreamStop();
  XLstatus mostStreamClose();

  unsigned char     m_streamBuffer[XL_MAX_STREAM_BUFFER];
  unsigned char     *m_pStreamBuffer[XL_MAX_STREAM_BUFFER];
  unsigned int      m_bufferSize[XL_MAX_STREAM_BUFFER];
  unsigned int      m_nBuffer;
  FILE              *m_pFile;

  XLmostStreamOpen  m_xlMostStreamOpen;
  unsigned int      m_streamHandle;
  int               m_channel;
};
