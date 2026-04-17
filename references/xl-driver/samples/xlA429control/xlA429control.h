// xlA429control.h : main header file for the XLA429CONTROL application
//

#if !defined(AFX_XLA429CONTROL_H__FAC45B58_F7CE_45F2_8BC7_CD3BC6A8D5C2__INCLUDED_)
#define AFX_XLA429CONTROL_H__FAC45B58_F7CE_45F2_8BC7_CD3BC6A8D5C2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CXlA429controlApp:
// See xlA429control.cpp for the implementation of this class
//

class CXlA429controlApp : public CWinApp
{
public:
	CXlA429controlApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CXlA429controlApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CXlA429controlApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_XLA429CONTROL_H__FAC45B58_F7CE_45F2_8BC7_CD3BC6A8D5C2__INCLUDED_)
