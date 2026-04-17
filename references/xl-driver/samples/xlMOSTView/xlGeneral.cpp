/*-------------------------------------------------------------------------------------------
| File        : xlGeneral.cpp
| Project     : Vector MOST Example 
|
| Description : 
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2005 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#include "StdAfx.h"
#include ".\xlgeneral.h"

CGeneral::CGeneral(void)
{
}

CGeneral::~CGeneral(void)
{
}

////////////////////////////////////////////////////////////////////////////

//! StrToShort

//! converts safely string to short value
//!
////////////////////////////////////////////////////////////////////////////

unsigned short CGeneral::StrToShort(char *str) 
{
  int valret = 0,tmpVal;
  unsigned int bpos;
  int bMode = 10,i;
  char *stopstring,dummyVal[2]={0,0};

  if(!str)return 0;
  if(strlen(str)==0)return 0;

  if((str[0]=='0')&&(str[1]=='x'))bMode = 16;//we have hex mode

  if(bMode == 16){//for future improvement
    bpos = 1;
    for (i=(int)strlen(str)-1;i>1;i--){//like "0xfae81a"
      dummyVal[0] = str[i];
      tmpVal = strtol(dummyVal,&stopstring,16);
      valret+=tmpVal*bpos;
      bpos*=bMode;
    }
  }else {//10
    bpos = 1;
    for (i=(int)strlen(str)-1;i>-1;i--){//like "125534"
      dummyVal[0] = str[i];
      tmpVal = atoi(dummyVal);
      valret+=tmpVal*bpos;
      bpos*=bMode;
    }
  }
  return (unsigned short)valret;
}

