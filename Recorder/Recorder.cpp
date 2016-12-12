
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
#include <DbgHelp.h>
#include <log4cplus/loggingmacros.h>

#pragma comment( lib, "DbgHelp" )

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

#define CALL_FIRST 1  
#define CALL_LAST 0

BOOL CALLBACK MiniDumpCallback(PVOID pParam, const PMINIDUMP_CALLBACK_INPUT   pInput,
	PMINIDUMP_CALLBACK_OUTPUT pOutput)

{
	// Callback implementation

	if (pInput == NULL)
		return FALSE;

	if (pOutput == NULL)
		return FALSE;

	if (pInput->CallbackType == ModuleCallback)
	{
		return TRUE;
	}

	return TRUE;

}


LONG WINAPI
VectoredHandler(
struct _EXCEPTION_POINTERS *ExceptionInfo
	)
{

	if (ExceptionInfo->ExceptionRecord->ExceptionCode == 0x406D1388
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0xE06D7363
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0x000006BA
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0x8001010D
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0x80010108
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0x40010006
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0x80000003
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0x80000004
		|| ExceptionInfo->ExceptionRecord->ExceptionCode == 0xC0000095
		)
	{
	}
	else{
		DWORD pId = GetCurrentProcessId();
		std::stringstream oss;
		oss << "d:\\log" << L"\\" << pId <<L"." << CRecorderDlg::GetVersion() <<  L".dmp";
		std::string dmpFile = oss.str();
		HANDLE lhDumpFile = CreateFile(dmpFile.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		MINIDUMP_EXCEPTION_INFORMATION loExceptionInfo;
		loExceptionInfo.ExceptionPointers = ExceptionInfo;
		loExceptionInfo.ThreadId = GetCurrentThreadId();
		loExceptionInfo.ClientPointers = TRUE;
		if (lhDumpFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_CALLBACK_INFORMATION mci;

			mci.CallbackRoutine = MiniDumpCallback;
			mci.CallbackParam = NULL;     // this example does not use the context

			MiniDumpWriteDump(GetCurrentProcess(), pId, lhDumpFile, MINIDUMP_TYPE(
				MiniDumpWithDataSegs |
				MiniDumpWithUnloadedModules |
				MiniDumpWithProcessThreadData),
				&loExceptionInfo,
				NULL,
				NULL);
			CloseHandle(lhDumpFile);
			//MessageBox(NULL, "Stop", "" ,MB_OK);
		}
	}
	/*std::string info = "你所使用的坐席程序发生了一个意外错误，请将此窗口截图和文件\r\n";
	info += dmpFile;
	info += "提交给开发人员，并重启程序。";

	std::string caption = "坐席插件崩溃:";
	caption += CloopenAgentBase::GetVersion();*/
	//MessageBox(NULL, info.c_str() , caption.c_str() , MB_OK);
	return EXCEPTION_CONTINUE_SEARCH;
}

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

	PVOID h1 = NULL;
	if (h1 == NULL){
		h1 = AddVectoredExceptionHandler(CALL_FIRST, VectoredHandler);
	}

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

	HANDLE m_hMutex = ::CreateMutex(NULL, TRUE, "Sanhui.Recorder");
	if (GetLastError() == ERROR_ALREADY_EXISTS){
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(),"程序已经在运行。");
		::MessageBox(NULL,"此程序只允许启动一个实例。","运行错误",MB_OK);//弹出对话框确认不能运行第二个实例。
		return FALSE;
	}

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
	bool flagTime=true;
	CTime t(2018,10,31,0,0,0);
	
	if(CTime::GetCurrentTime() > t) 
	{
		flagTime = false;
	}

	if ("BFEBFBFF000206A71FBAE3FF" == strSerial&&flagTime)
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
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(),"Error Info serial or restricted by time. version:" << CRecorderDlg::GetVersion());
	}
	// 删除上面创建的 shell 管理器。
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// 由于对话框已关闭，所以将返回 FALSE 以便退出应用程序，
	//  而不是启动应用程序的消息泵。
	if (h1){
		RemoveVectoredExceptionHandler(h1);
		h1 = NULL;
	}
	return FALSE;
}

