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
		std::string sql;
		try
		{

			if(!db->m_dataBase.IsOpen()){
				/*db->m_dataBase.SetConnectionTimeout(30);
				LOG4CPLUS_TRACE(log, "SetConnectionTimeout:30");*/
				LOG4CPLUS_TRACE(log, "Connecting database:" << db->m_strConnection);
				if(db->m_dataBase.Open(db->m_strConnection)){
					LOG4CPLUS_INFO(log, "Connected database:" << db->m_strConnection);
				}else{
					LOG4CPLUS_WARN(log, "Connecting Error database:" << db->m_strConnection);
				}
			}
			else{
				LOG4CPLUS_TRACE(log, "ge data from sql queue, timeout:60*1000ms");
				if(db->m_sqlQueue.getData(sql,60*1000)){
					LOG4CPLUS_TRACE(log, "get a sql:" << sql);
					db->m_dataBase.Execute(_bstr_t(sql.c_str()));
				}
				else{
					//db->addSql2Queue("select top 1 * from dbo.car;");
					LOG4CPLUS_TRACE(log, "sql queue is empty.");
					std::string trySQL="select count(id)as nid from sysobjects where xtype='u'";
					db->m_dataBase.Execute(_bstr_t(trySQL.c_str()));
				}
			}
			

		}
	
		catch(CADOException e){
			LOG4CPLUS_ERROR(log, "ErrorCode:" << std::hex << e.GetError() << ",ErrorMsg:" << e.GetErrorMessage());
			if (e.GetError() == 0x80004005) 
			{
				if(db->m_dataBase.IsOpen()){
					LOG4CPLUS_INFO(log," close connect.");
					db->m_dataBase.Close();
				}
				if(!sql.empty())
				{
					LOG4CPLUS_INFO(log," add the execute error sql:"<< sql << " to the sql Queue.");
					db->addSql2Queue(sql);
					sql.clear();
				}
				LOG4CPLUS_INFO(log, "sleep this thread 30*1000 ms.");
				Sleep(30*1000);
			}
		}
		catch (CException* e)
		{
			TCHAR   szError[1024];
			e->GetErrorMessage(szError,1024);
			LOG4CPLUS_ERROR(log, szError);
		}
		catch(...)
		{
			LOG4CPLUS_ERROR(log, "unknown exception.");
		}
	}
	db->td.thread_hnd = NULL;
	LOG4CPLUS_TRACE(log, __FUNCTION__ << " end.");
	return 0;
}

void DataBase::SetConnectionString(const CString &lpstrConnection)
{
	m_strConnection = lpstrConnection;
}

CString DataBase::GetConnectionString()const
{
	return m_strConnection;
}

void DataBase::addSql2Queue(const std::string & sql)
{
	m_sqlQueue.addData(sql);
}