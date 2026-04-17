/*-------------------------------------------------------------------------------------------
| File        : xlFlexDemo.h
| Project     : Vector FlexRay Example
|
| Description : Shows the basic FlexRay functionality for the XL Driver Library
|--------------------------------------------------------------------------------------------
|--------------------------------------------------------------------------------------------
| Copyright (c) 2012 by Vector Informatik GmbH.  All rights reserved.
|------------------------------------------------------------------------------------------*/

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CxlFlexDemoApp:
// See xlFlexDemo.cpp for the implementation of this class
//

class CxlFlexDemoApp : public CWinApp
{
public:
	CxlFlexDemoApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CxlFlexDemoApp theApp;