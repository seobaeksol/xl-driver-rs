#pragma once

#include "stdafx.h"
#include "resource.h"		// Hauptsymbole

class CXlDAIOdemoApp : public CWinApp {
public:
	CXlDAIOdemoApp() = default;

	//{{AFX_VIRTUAL(CXlDAIOdemoApp)
	BOOL InitInstance() final;
	//}}AFX_VIRTUAL

	DECLARE_MESSAGE_MAP()
};