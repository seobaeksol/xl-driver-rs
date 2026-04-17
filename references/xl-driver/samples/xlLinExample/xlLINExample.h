// LINExample.h : main header file for the LINEXAMPLE application
//

#if !defined(AFX_LINEXAMPLE_H__3B18A11C_C327_4194_9387_F825F250BADB__INCLUDED_)
#define AFX_LINEXAMPLE_H__3B18A11C_C327_4194_9387_F825F250BADB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CLINExampleApp:
// See LINExample.cpp for the implementation of this class
//

class CLINExampleApp : public CWinApp
{
public:
	CLINExampleApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLINExampleApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLINExampleApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINEXAMPLE_H__3B18A11C_C327_4194_9387_F825F250BADB__INCLUDED_)
