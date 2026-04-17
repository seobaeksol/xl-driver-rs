// xlMOSTView.h : Hauptheaderdatei für die xlMOSTView-Anwendung
//

#pragma once

#ifndef __AFXWIN_H__
  #error 'stdafx.h' muss vor dieser Datei in PCH eingeschlossen werden.
#endif

#include "resource.h"		// Hauptsymbole


// CxlMOST150ViewApp:
// Siehe CxlMOST150ViewApp.cpp für die Implementierung dieser Klasse
//

class CxlMOST150ViewApp : public CWinApp
{
public:
  CxlMOST150ViewApp();

// Überschreibungen
  public:
  virtual BOOL InitInstance();

// Implementierung

  DECLARE_MESSAGE_MAP()
};

extern CxlMOST150ViewApp theApp;
