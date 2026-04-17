#include "xlDAIOdemo.h"
#include "xlDAIOdemoDlg.h"

BEGIN_MESSAGE_MAP(CXlDAIOdemoApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

CXlDAIOdemoApp theApp{};

BOOL CXlDAIOdemoApp::InitInstance()
{
	CXlDAIOdemoDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	// Da das Dialogfeld geschlossen wurde, FALSE zurückliefern, so dass wir die
	//  Anwendung verlassen, anstatt das Nachrichtensystem der Anwendung zu starten.
	return false;
}