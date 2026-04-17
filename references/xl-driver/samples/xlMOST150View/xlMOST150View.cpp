// xlMOST150View.cpp : implementation file
//

#include "stdafx.h"
#include "xlMOST150View.h"
#include "xlMOST150ViewDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CxlMOST150ViewApp

BEGIN_MESSAGE_MAP(CxlMOST150ViewApp, CWinApp)
  ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CxlMOST150ViewApp-Erstellung

CxlMOST150ViewApp::CxlMOST150ViewApp()
{
}


CxlMOST150ViewApp theApp;


BOOL CxlMOST150ViewApp::InitInstance()
{
  InitCommonControls();

  CWinApp::InitInstance();

  AfxEnableControlContainer();

  // Standardinitialisierung
  // Wenn Sie diese Features nicht verwenden und die Größe
  // der ausführbaren Datei verringern möchten, entfernen Sie
  // die nicht erforderlichen Initialisierungsroutinen.
  // Ändern Sie den Registrierungsschlüssel unter dem Ihre Einstellungen gespeichert sind.
  // TODO: Ändern Sie diese Zeichenfolge entsprechend,
  // z.B. zum Namen Ihrer Firma oder Organisation.
  SetRegistryKey(_T("Vom lokalen Anwendungs-Assistenten generierte Anwendungen"));

  CxlMOST150ViewDlg dlg;
  m_pMainWnd = &dlg;
  INT_PTR nResponse = dlg.DoModal();
  if (nResponse == IDOK)
  {
  }
  else if (nResponse == IDCANCEL)
  {
  }

  return FALSE;
}
