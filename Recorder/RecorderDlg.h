
// RecorderDlg.h : 头文件
//

#pragma once

#include "C:\ShCti\api\Vc6.0\Inc\shpa3api.h"
#include <string>
#include <log4cplus/logger.h>
#include "afxcmn.h"
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
	STATE_PICKUP,			//摘机
};


enum RECORD_DIRECTION
{
	CALL_IN_RECORD,		//Incoming call recording
	CALL_OUT_RECORD,	//Outgoing call recording
	MIX_RECORD			//Mix-record of incoming/outgoing call 
};


typedef struct tagCIC_STRUCT
{
	int  nState;				//State of monitored circuits
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
	CString szFileName; // record file name
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
	log4cplus::Logger log;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ChList;
	//int		m_nRecFormat;
	int		m_nCallFnMode;
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	CIC_STRUCT ChMap[MAX_CH];	//Monitored circuits
	int nMaxCh;					//Maximum number of the monitored circuits
	BOOL InitCtiBoard();			//Initialize board
	void InitCircuitListCtrl();		//Initialize list
	void UpdateCircuitListCtrl(unsigned int nIndex);	//Update list
public:
	afx_msg void OnDestroy();
	CRichEditCtrl m_ctrCapacityView;

	void DrawCapacityView();
};
