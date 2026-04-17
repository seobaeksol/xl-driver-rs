// xlMOSTView.h : Hauptheaderdatei für die xlMOSTView-Anwendung
//

#pragma once

#ifndef __AFXWIN_H__
	#error 'stdafx.h' muss vor dieser Datei in PCH eingeschlossen werden.
#endif

#include "resource.h"		// Hauptsymbole


// CxlMOSTViewApp:
// Siehe xlMOSTView.cpp für die Implementierung dieser Klasse
//

class CxlMOSTViewApp : public CWinApp
{
public:
	CxlMOSTViewApp();

// Überschreibungen
	public:
	virtual BOOL InitInstance();

// Implementierung

	DECLARE_MESSAGE_MAP()
};

extern CxlMOSTViewApp theApp;
