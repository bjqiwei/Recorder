
// Recorder.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "Recorder.h"
#include "RecorderDlg.h"
#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include "CPUID.h"
#include <sstream>
#include <iomanip>
#include <log4cplus/loggingmacros.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRecorderApp

BEGIN_MESSAGE_MAP(CRecorderApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CRecorderApp 构造

CRecorderApp::CRecorderApp()
{
	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}


// 唯一的一个 CRecorderApp 对象

CRecorderApp theApp;
TCHAR szPath[MAX_PATH];

// CRecorderApp 初始化

BOOL CRecorderApp::InitInstance()
{
	AfxInitRichEdit2();
	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// 创建 shell 管理器，以防对话框包含
	// 任何 shell 树视图控件或 shell 列表视图控件。
	CShellManager *pShellManager = new CShellManager;

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

	GetModuleFileName(theApp.m_hInstance,szPath,MAX_PATH);

	// log4cplus initialize;
	log4cplus::tstring logFilePath = szPath;
	log4cplus::tstring logFileName = szPath;
	log4cplus::tstring logFile;
	logFilePath = logFilePath.substr(0,logFilePath.rfind(_T("\\"))+1);
	logFileName = logFileName.substr(logFileName.rfind(_T("\\"))+1,logFileName.rfind(_T(".")) - logFileName.rfind(_T("\\"))-1);
	//logFileName.append(_T(".log"));

	logFile = logFilePath + logFileName;
	logFile = "d:\\log\\LOG";
	log4cplus::initialize();
	log4cplus::SharedAppenderPtr _append(new log4cplus::DailyRollingFileAppender(logFile, 
		log4cplus::HOURLY,
		true,24));

	_append->setName(logFileName);
	std::string pattern = _T("%D{%y/%m/%d %H:%M:%S.%Q} [%t] %-5p %c{3} %x -  %m;line:%L %n");    
	std::auto_ptr<log4cplus::Layout> _layout(new log4cplus::PatternLayout(pattern));   

	_append->setLayout( _layout );   
	log4cplus::Logger root = log4cplus::Logger::getRoot();

	root.addAppender(_append);
	root.setLogLevel(log4cplus::ALL_LOG_LEVEL);

	CPUID cpu;
	SerialNumber serial;
	cpu.GetSerialNumber(serial);
	std::ostringstream oss ;
	for (int i=0; serial.nibble[i] >0 ;++i)
	{
		oss << std::uppercase << std::hex <<std::setw(8)<< std::setfill('0')<< serial.nibble[i];
	}
	std::string strSerial = oss.str();
	LOG4CPLUS_INFO(log4cplus::Logger::getRoot(),"CPU Serial:DDAB04" << strSerial);

	if ("BFEBFBFF000206A71FBAE3FF" == strSerial)
	{
	
		CRecorderDlg dlg;
		m_pMainWnd = &dlg;
		INT_PTR nResponse = dlg.DoModal();
		if (nResponse == IDOK)
		{
			// TODO: 在此放置处理何时用
			//  “确定”来关闭对话框的代码
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: 在此放置处理何时用
			//  “取消”来关闭对话框的代码
		}
	}else{
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(),"Error Info serial");
	}
	// 删除上面创建的 shell 管理器。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	return FALSE;
}

