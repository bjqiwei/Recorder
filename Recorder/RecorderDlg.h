
// RecorderDlg.h : 头文件
//

#pragma once

#include "C:\ShCti\api\Vc6.0\Inc\shpa3api.h"
#include <string>
#include <log4cplus/logger.h>
#include "afxcmn.h"
#include "DataBase.h"
#include "CSystemTray.h"
/////////////////////////////////////////////////////////////////////////////

// CRecorderDlg 对话框
#define MAX_CH 1000		//Maximum number of the monitored circuits
//User-defined circuit status
enum CIRCUIT_STATE
{
	CIRCUIT_IDLE,			//Idle state
	CIRCUIT_RCV_PHONUM,		//State of receiving phone number
	CIRCUIT_RINGING,		//State of ringing
	CIRCUIT_TALKING,		//State of talking
	STATE_RECORDING,		//录音
	STATE_PICKUP,			//摘机
	STATE_UNUSEABLE,			//不可用
};


enum RECORD_DIRECTION
{
	CALL_IN_RECORD,		//Incoming call recording
	CALL_OUT_RECORD,	//Outgoing call recording
	MIX_RECORD			//Mix-record of incoming/outgoing call 
};


typedef struct tagCIC_STRUCT
{
	CIRCUIT_STATE  nState;				//State of monitored circuits
	CString szState;
	CString szCallerId;		//Calling party number
	CString szCalleeId;		//Called party number
	CString szDtmf;		//DTMF received on the incoming channel
	//CString szCallOutDtmf;	//DTMF received on the outgoing channel
	WORD wRecDirection;			//Recording direction
	int  nCallInCh;				//Incoming channel
	int  nCallOutCh;			//Outgoing channel
	unsigned int nRecordTimes;  //Record Times 
	CTime tStartTime;// record start time
	CTime tEndTime;// record end time
	CString szFileName; // record file name
	CString sql;
}CIC_STRUCT;

class CRecorderDlg : public CDialogEx
{
// 构造
public:
	CRecorderDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_RECORDER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	HICON m_hIconOffhook;
	HICON m_hIconOnhook;
	log4cplus::Logger log;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ChList;
	CImageList m_ImageList;
	//int		m_nRecFormat;
	//int		m_nCallFnMode;
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	CIC_STRUCT ChMap[MAX_CH];	//Monitored circuits
	int nMaxCh;					//Maximum number of the monitored circuits
	ULONGLONG m_freeCapacity;
	ULONGLONG m_totalCapacity;
	BOOL InitCtiBoard();			//Initialize board
	void InitCircuitListCtrl();		//Initialize list
	void UpdateCircuitListCtrl(unsigned int nIndex);	//Update list
	void SetRegKey(CString name, CString value);
	void SetRegKey(CString name, DWORD value);
	CString ReadRegKeyString(CString name);
	DWORD ReadRegKeyDWORD(CString name);
public:
	afx_msg void OnDestroy();
	CRichEditCtrl m_ctrCapacityView;

	void DrawCapacityView();
	afx_msg void OnBnClickedButton1();
private:
	CString m_strFileDir;
public:
	CString m_strDataBase;
	afx_msg void OnBnClickedButton2();
private:
	UINT m_KeepDays;
public:
	afx_msg void OnBnClickedButton3();
private:
	UINT m_DBKeepDays;
public:
	afx_msg void OnBnClickedButton4();
private:
	UINT m_RecordingSum;
	BOOL m_DetailLog;
public:
	afx_msg void OnBnClickedCheck1();
private:
	int m_AutoBackup;
public:
	afx_msg void OnBnClickedCheck3();
private:
	DataBase m_sqlServerDB;
public:
	void checkDiskSize(void);
private:
	CString m_strTotalSize;
public:
	CString m_strFreeSize;
	CString m_strApplySize;
	bool StopRecording(unsigned long nIndex);
	bool StartRecording(unsigned long nIndex);
	void SetChannelState(unsigned long nIndex, CIRCUIT_STATE newState);
	void GetCaller(unsigned long nIndex);
	void GetCallee(unsigned long nIndex);
	void GetCallerAndCallee(unsigned long nIndex);
	bool CreateMultipleDirectory(const CString& szPath);
	int MySpyChToCic(int nCh);
	static std::string GetShEventName(unsigned int nEvent);
	static std::string GetShStateName(unsigned int nState);
	static const std::string & GetVersion();
#define WM_ICON_NOTIFY (WM_USER + MAX_EVENT_SIZE + 1)
	CSystemTray m_TrayIcon; 
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShow();
	afx_msg void OnExit();
};
