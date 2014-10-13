// DTP_Event_VC.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Recorder_DTP.h"
#include "Recorder_DTPDlg.h"
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp

BEGIN_MESSAGE_MAP(CRecorder_DTPApp, CWinApp)
	//{{AFX_MSG_MAP(CDTP_Event_VCApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp construction

CRecorder_DTPApp::CRecorder_DTPApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDTP_Event_VCApp object

CRecorder_DTPApp theApp;
TCHAR szPath[MAX_PATH];
/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCApp initialization

BOOL CRecorder_DTPApp::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.
	GetModuleFileName(theApp.m_hInstance,szPath,MAX_PATH);

	// log4cplus initialize;
	log4cplus::tstring logFilePath = szPath;
	log4cplus::tstring logFileName = szPath;
	log4cplus::tstring logFile;
	logFilePath = logFilePath.substr(0,logFilePath.rfind(_T("\\"))+1);
	logFileName = logFileName.substr(logFileName.rfind(_T("\\"))+1,logFileName.rfind(_T(".")) - logFileName.rfind(_T("\\"))-1);
	logFileName.append(_T(".log"));

	logFile = logFilePath + logFileName;
	log4cplus::initialize();
	log4cplus::SharedAppenderPtr _append(new log4cplus::DailyRollingFileAppender(logFile, 
		log4cplus::DailyRollingFileSchedule::HOURLY,
		true,24*60));

	_append->setName(logFileName);
	std::string pattern = _T("%D{%y/%m/%d %H:%M:%S.%Q} [%t] %-5p %c{3} %x -  %m;line:%L %n");    
	std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout(pattern));   

	_append->setLayout( _layout );   
	log4cplus::Logger root = log4cplus::Logger::getRoot();

	root.addAppender(_append);
	root.setLogLevel(log4cplus::ALL_LOG_LEVEL);

	CRecorder_DTPDlg dlg;
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

