#pragma once
#include <vector>

#include "xlmost150param.h"

class CMOST150Streaming : public virtual CMOST150Param
{
public:
  CMOST150Streaming     (void);
  ~CMOST150Streaming    (void);
  
  XLstatus MOST150StreamStart(unsigned int nChan);
  XLstatus MOST150StreamStop();
  XLstatus MOST150StreamParseEvent(XLmost150event *pxlMost150Event);

private:
  
  XLstatus mostStreamStart(void);
  XLstatus mostStreamClose(void);

  unsigned char       *m_pStreamBuffer;
  FILE                *m_pFile;

  std::vector<XL_MOST150_CL_INFO>   m_AllocTable;
  XL_MOST150_CL_INFO*  m_pAllocInfo;
  XLmost150StreamOpen  m_xlMost150StreamOpen;
  unsigned int         m_streamHandle;
  int                  m_channel;
  bool                 m_InitRxFifo;
};
