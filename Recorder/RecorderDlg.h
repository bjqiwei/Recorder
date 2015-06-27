
// RecorderDlg.h : ͷ�ļ�
//

#pragma once

#include "C:\ShCti\api\Vc6.0\Inc\shpa3api.h"
#include <string>
#include <log4cplus/logger.h>
#include "afxcmn.h"
#include "DataBase.h"
#include "CSystemTray.h"
/////////////////////////////////////////////////////////////////////////////

// CRecorderDlg �Ի���
#define MAX_CH 1000		//Maximum number of the monitored circuits
#define  MAX_ACTIVE_LINE_NUM 20
#define MAX_SLAVER_COUNT 6


#define PCM_8	1
#define PCM_16	-2
#define A_LAW	6
#define U_LAW	7
#define ADPCM	17
#define VOX		23
#define GSM		49
#define MP3		85
#define GC8		131
#define G729A	65411

//User-defined circuit status
enum CH_STATE
{
	CH_IDLE,			//Idle state
	CH_ACTIVE,			// active
	CH_RCV_PHONUM,		//State of receiving phone number
	CH_RINGING,		//State of ringing
	CH_TALKING,		//State of talking
	CH_RECORDING,		//¼��
	CH_PICKUP,			//ժ��
	CH_OFFLINE,			//����
	CH_UNAVAILABLE,		//������
	CH_COMMUNICATING,	//ͨ����
	CH_USING,		//ʹ��
	CH_PAUSED,		//ֹͣ
};


enum RECORD_DIRECTION
{
	CALL_IN_RECORD,		//Incoming call recording
	CALL_OUT_RECORD,	//Outgoing call recording
	MIX_RECORD			//Mix-record of incoming/outgoing call 
};

enum CH_TYPE
{
	CH_TYPE_ERROR = -1,//	����ʧ��
	CH_TYPE_ANALOG_TRUNK = 0,//	ģ���м���ͨ��
	CH_TYPE_AGENT = 2,//��ϯͨ��
	CH_TYPE_ANALOG_RECORD = 3,//	ģ���м���¼��ͨ��
	CH_TYPE_SS1 = 4,//	SS1ͨ��
	CH_TYPE_TUP = 6,//	TUPͨ��
	CH_TYPE_ISDN_ISUP = 7,//	ISDNͨ�����û��ࣩ
	CH_TYPE_ISDN = 8,//	ISDNͨ��������ࣩ
	CH_TYPE_FAX = 9,//	������Դͨ��
	CH_TYPE_MAGENT = 10,//	��ʯͨ��
	CH_TYPE_SS7_ISUP = 11,//	ISUPͨ�����й�SS7����ISUP��
	CH_TYPE_E1_RECORD = 12,//	���ֵ绰��¼��ͨ��
	CH_TYPE_EM = 13,//	Channel Bank��EMͨ��
	CH_TYPE_CRACK = 14,//	����ͨ��
	CH_TYPE_SIP = 16,//	SIPͨ��
	CH_TYPE_IP = 17,//	IP��Դ��ͨ��
	CH_TYPE_DASS2 = 19,//	DASS2ͨ��
	CH_TYPE_NOMODULE = 20,//	SHTϵ�а忨δ��װҵ��ģ���ͨ��
	CH_TYPE_EM_CONTROL = 21,//	EM����ͨ��
	CH_TYPE_EM_VOICE = 22,//	EM����ͨ��
	CH_TYPE_IPR = 25, //	IPRͨ������SynIPR¼��ͨ��
	CH_TYPE_IPA = 26,//	IPAͨ������SynIPR���ݰ�����ͨ

};

enum Control			//recording working mode
{   
	VOICE_CTRL,			//voice-control mode
	VOLTAGE_CTRL,		//voltage-control mode
	DTMF_CTRL			//code-control mode
};

enum
{
	RECORDING_BASE_SESSION,
	RECORDING_BASE_DEVENT,
};

typedef struct tagCH_STRUCT
{
	CH_STATE  nState;				//State of monitored channel
	CString szState;
	CString szCallerId;		//Calling party number
	CString szCalleeId;		//Called party number
	CString szDtmf;		//DTMF received on the incoming channel
	CString szCallOutDtmf;	//DTMF received on the outgoing channel
	WORD wRecDirection;			//Recording direction
	int  nCallInCh;				//Incoming channel
	int  nCallOutCh;			//Outgoing channel
	unsigned int nRecordTimes;  //Record Times 
	CTime tStartTime;// record start time
	CTime tEndTime;// record end time
	CString szFileName; // record file name
	CString sql;
	int nChType;
	int	CtrlState;		//recording working mode
	bool bIgnoreLineVoltage;
	// VOIP 
	int nCallRef;
	DWORD		dwSessionId;				//		Session ID
	int			nPtlType;					//		Protocol type
	int			nStationId;					//		Station ID
	int			nFowardingPPort;			//		Primary forwarding port
	int			nFowardingSPort;			//		Secondary forwarding port
	CString		szIPP;			//		IP address of primary
	CString		szIPS;			//		IP address of slavery
	int			nRecordingCtrl;				//		Recording controlled by DChannel event or just by session
	int			nRecSlaverId;				//		Destination Slaver
	DWORD		dwActiveTime;				//		the time enter active
	int			nSCCPActiveCallref[MAX_ACTIVE_LINE_NUM];
	pIPR_SessionInfo pSessionInfo;
	
}CH_STRUCT;

class CRecorderDlg : public CDialogEx
{
// ����
public:
	CRecorderDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_RECORDER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;
	HICON m_hIconOffhook;
	HICON m_hIconOnhook;
	log4cplus::Logger log;
	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ChList;
	CImageList m_ImageList;
	//int		m_nRecFormat;
	int		m_nCallFnMode;
	static int CALLBACK EventCallback(PSSM_EVENT pEvent);
	static CH_STRUCT ChMap[MAX_CH];	//Monitored circuits
	static int nMaxCh;					//Maximum number of the monitored channel
	static int   nIPRBoardId;
	static int   nIPABoardId;
	static int   nSlaverCount;
	static IPR_SLAVERADDR IPR_SlaverAddr[MAX_SLAVER_COUNT];
private:
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
	static void SetChannelState(unsigned long nIndex, CH_STATE newState);
	static void GetCaller(unsigned long nIndex);
	static void GetCallee(unsigned long nIndex);
	static void GetCallerAndCallee(unsigned long nIndex);
	static void ClearChVariable(unsigned long nCh);
	static void ScanSlaver();
	bool CreateMultipleDirectory(const CString& szPath);
	int MySpyChToCic(int nCh);
	static std::string GetShEventName(unsigned int nEvent);
	static std::string GetShStateName(unsigned int nState);
	static std::string GetDSTStateName(unsigned int nState);
	static std::string GetSsmLastErrMsg();
#define WM_ICON_NOTIFY (WM_USER + MAX_EVENT_SIZE + 1)
	CSystemTray m_TrayIcon; 
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnShow();
	afx_msg void OnExit();
};
