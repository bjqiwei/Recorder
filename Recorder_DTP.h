// DTP_Event_VC.h : main header file for the DTP_EVENT_VC application
//

#if !defined(AFX_DTP_EVENT_VC_H__BE7E1CFB_F1EB_45CA_B772_96C471ECAD25__INCLUDED_)
#define AFX_DTP_EVENT_VC_H__BE7E1CFB_F1EB_45CA_B772_96C471ECAD25__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp:
// See DTP_Event_VC.cpp for the implementation of this class
//

class CDTP_Event_VCApp : public CWinApp
{
public:
	CDTP_Event_VCApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDTP_Event_VCApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CDTP_Event_VCApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTP_EVENT_VC_H__BE7E1CFB_F1EB_45CA_B772_96C471ECAD25__INCLUDED_)
