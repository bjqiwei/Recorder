#pragma once
#ifndef _EVENTBUFFER_HEADER_
#define _EVENTBUFFER_HEADER_
#include <deque>
#include <Windows.h>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

template<class T>
class EventBuffer
{
public:
	EventBuffer(void){
		log = log4cplus::Logger::getInstance("EventBuffer.");
		InitializeCriticalSection(&csection);
		semaphore = CreateSemaphore(NULL, 0, LONG_MAX, NULL);
		if (NULL == semaphore){
			long err = GetLastError();
			LOG4CPLUS_ERROR(log,"Structure EventBuffer encountered an error,error=" << err);
		}
	}
	virtual ~EventBuffer(void){
		DeleteCriticalSection(&csection);
		CloseHandle(semaphore);
	}

	void addData(const T &data){
		EnterCriticalSection(&csection);
		if (m_dataBuffer.size() > MAXBUFFER)
		{
			LOG4CPLUS_WARN(log,"Event buffer size Exceed max buffer." );
			while(m_dataBuffer.size() >= MAXBUFFER){
				LOG4CPLUS_ERROR(log,"erase one event=" << m_dataBuffer.front());
				m_dataBuffer.pop_front();
			}
		}
		this->m_dataBuffer.push_back(data);
		ReleaseSemaphore(semaphore, 1, NULL);
		LeaveCriticalSection(&csection);
	}
	bool getData(T& data, DWORD dwMilliseconeds = INFINITE)
	{
		bool result =false;
		DWORD ret = WaitForSingleObject(semaphore,dwMilliseconeds);
		switch(ret)
		{
		 case WAIT_OBJECT_0:
			 {
				 EnterCriticalSection(&csection);
				 if (m_dataBuffer.size() > 0)
				 {
					 data = m_dataBuffer.front();
					 m_dataBuffer.pop_front();
					 result = true;
				 }
				 LeaveCriticalSection(&csection);
			 }
			 break;

		 case WAIT_TIMEOUT:
			 break;
		 default:
			 break;
		}
		
		return result;
	}
	unsigned long size() const
	{
		return m_dataBuffer.size();
	}
private:
	HANDLE semaphore;
	std::deque<T> m_dataBuffer; 
	CRITICAL_SECTION csection;
	log4cplus::Logger log;
	static const unsigned int MAXBUFFER = 1024*16;
};


#endif
