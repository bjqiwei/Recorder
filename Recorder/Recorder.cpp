
// Recorder.cpp : ����Ӧ�ó��������Ϊ��
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


// CRecorderApp ����

CRecorderApp::CRecorderApp()
{
	// TODO: �ڴ˴���ӹ�����룬
	// ��������Ҫ�ĳ�ʼ�������� InitInstance ��
}


// Ψһ��һ�� CRecorderApp ����

CRecorderApp theApp;
TCHAR szPath[MAX_PATH];

// CRecorderApp ��ʼ��

BOOL CRecorderApp::InitInstance()
{
	AfxInitRichEdit2();
	// ���һ�������� Windows XP �ϵ�Ӧ�ó����嵥ָ��Ҫ
	// ʹ�� ComCtl32.dll �汾 6 ����߰汾�����ÿ��ӻ���ʽ��
	//����Ҫ InitCommonControlsEx()�����򣬽��޷��������ڡ�
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// ��������Ϊ��������Ҫ��Ӧ�ó�����ʹ�õ�
	// �����ؼ��ࡣ
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	// ���� shell ���������Է��Ի������
	// �κ� shell ����ͼ�ؼ��� shell �б���ͼ�ؼ���
	CShellManager *pShellManager = new CShellManager;

	// ��׼��ʼ��
	// ���δʹ����Щ���ܲ�ϣ����С
	// ���տ�ִ���ļ��Ĵ�С����Ӧ�Ƴ�����
	// ����Ҫ���ض���ʼ������
	// �������ڴ洢���õ�ע�����
	// TODO: Ӧ�ʵ��޸ĸ��ַ�����
	// �����޸�Ϊ��˾����֯��
	SetRegistryKey(_T("Ӧ�ó��������ɵı���Ӧ�ó���"));

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
			// TODO: �ڴ˷��ô����ʱ��
			//  ��ȷ�������رնԻ���Ĵ���
		}
		else if (nResponse == IDCANCEL)
		{
			// TODO: �ڴ˷��ô����ʱ��
			//  ��ȡ�������رնԻ���Ĵ���
		}
	}else{
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(),"Error Info serial");
	}
	// ɾ�����洴���� shell ��������
	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	// ���ڶԻ����ѹرգ����Խ����� FALSE �Ա��˳�Ӧ�ó���
	//  ����������Ӧ�ó������Ϣ�á�
	return FALSE;
}

