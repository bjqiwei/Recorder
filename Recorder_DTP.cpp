// DTP_Event_VC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Recorder_DTP.h"
#include "Recorder_DTPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp

BEGIN_MESSAGE_MAP(CDTP_Event_VCApp, CWinApp)
	//{{AFX_MSG_MAP(CDTP_Event_VCApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp construction

CDTP_Event_VCApp::CDTP_Event_VCApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDTP_Event_VCApp object

CDTP_Event_VCApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp initialization

BOOL CDTP_Event_VCApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.


	CDTP_Event_VCDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

