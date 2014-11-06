#include "StdAfx.h"
#include "DataBase.h"
#include <log4cplus/loggingmacros.h>


DataBase::DataBase(void):IsRunning(FALSE)
{
	log = log4cplus::Logger::getInstance("DataBase");
	td.thread_hnd = NULL;
	td.thread_id = 0;
}


DataBase::~DataBase(void)
{
	if (IsRunning)
	{
		IsRunning = false;
		stopDataBaseThread();
	}	
}

void DataBase::startDataBaseThread()
{
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " start.");
	if (td.thread_hnd) {
		LOG4CPLUS_WARN(log, "DataBaseThreadProc already exsits:" << td.thread_id);
		LOG4CPLUS_TRACE(log, __FUNCTION__ << " end.");
		return;
	}
	IsRunning = true;
	td.thread_hnd = (HANDLE)_beginthreadex(NULL,0,DataBaseThreadProc,this,0,&td.thread_id);
	if (!td.thread_hnd){
		int error = GetLastError();
		LOG4CPLUS_ERROR(log, __FUNCTION__ << " failed;error no.= " << error);
	}
	LOG4CPLUS_INFO(log, "Create DataBaseThreadProc:" << td.thread_id);
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " end.");
}

void DataBase::stopDataBaseThread()
{
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " start.");
	IsRunning = false;
	Sleep(40);
	if (td.thread_hnd){
		TerminateThread(td.thread_hnd,0);
		td.thread_hnd = NULL;
	}
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " end.");
}

unsigned int DataBase::DataBaseThreadProc( void *pParam )
{
	log4cplus::tostringstream logInstance_oss;
	logInstance_oss << "DataBase." << pParam;
	log4cplus::tstring logInstance = logInstance_oss.str();
	log4cplus::Logger log = log4cplus::Logger::getInstance(logInstance);
	coinit cominit;
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " start.");
	DataBase *db = reinterpret_cast<DataBase*>(pParam);
	while (db->IsRunning)
	{

		try
		{

			if(!db->m_dataBase.IsOpen()){
				LOG4CPLUS_TRACE(log, "Connecting database:" << db->m_strConnection);
				db->m_dataBase.Open(db->m_strConnection);
			}
			Sleep(1000);

		}
		catch (CMemoryException* e)
		{
			TCHAR   szError[1024];
			e->GetErrorMessage(szError,1024);
			LOG4CPLUS_ERROR(log, szError);
		}
		catch (CFileException* e)
		{
			TCHAR   szError[1024];
			e->GetErrorMessage(szError,1024);
			LOG4CPLUS_ERROR(log, e->m_strFileName << ":" << szError);
		}
		catch (CException* e)
		{
			TCHAR   szError[1024];
			e->GetErrorMessage(szError,1024);
			LOG4CPLUS_ERROR(log, szError);
		}
		catch(CADOException e){
			LOG4CPLUS_ERROR(log, e.GetErrorMessage());
		}
		catch(...)
		{
			LOG4CPLUS_ERROR(log, "unknown.");
		}
	}
	db->td.thread_hnd = NULL;
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " end.");
	return 0;
}

void DataBase::SetConnectionString(const CString lpstrConnection)
{
	m_strConnection = lpstrConnection;
}

CString DataBase::GetConnectionString()const
{
	return m_strConnection;
}