#pragma once
#include "ado2.h"
#include <log4cplus/logger.h>
#include "EventBuffer.h"

struct thread_data {
	HANDLE thread_hnd;
	unsigned thread_id;
};
class coinit{
public:
	coinit():hr(NULL){
		hr = CoInitialize(NULL);

	}
	~coinit()
	{
		if(SUCCEEDED(hr))
		{
			CoUninitialize();
		}
	}
private:
	HRESULT  hr;
};
class DataBase
{
public:
	DataBase(void);
	virtual ~DataBase(void);
	volatile bool IsRunning;
	void SetConnectionString(const CString &lpstrConnection);
	CString GetConnectionString() const;
	void startDataBaseThread();
	void stopDataBaseThread();
	void addSql2Queue(const std::string & sql);

private:
	CADODatabase m_dataBase;
	static unsigned int __stdcall DataBaseThreadProc( void *pParam );
	struct thread_data td;
	log4cplus::Logger log;
	CString m_strConnection;
	EventBuffer<std::string> m_sqlQueue;

};

