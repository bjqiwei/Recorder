
// RecorderDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Recorder.h"
#include "RecorderDlg.h"
#include "afxdialogex.h"
#include <log4cplus/loggingmacros.h>
#include <iostream>
#include <math.h>
#include "oledb2.h"
#include <iosfwd>
#include <map>
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define  ChannelWidth 60
#define  StatusWidth 100
#define	 CallingWidth	100
#define  CalleeWidth	100
#define  DTMFWidth 120
//#define  OutDTMFWidth	120
#define  RecordTimesWidth	100
#define	 StartTimeWidth		100
#define  FileNameWidth		400
enum{
	Channel = 0,
	ChState,
	ChCaller,
	ChCallee,
	//ChInDTMF,
	//ChDTMF,
	ChTimes,
	ChStartTime,
	ChFileName,
};
#define  ColumnNumber (ChFileName + 1)
static LPTSTR ColumnNameCh[ColumnNumber] = {"通道号",		"通道状态",	"主叫号码",		"被叫号码",	 /*"DTMF",*/	"录音次数",		"开始时间",	 "录音文件名称"};
static LPTSTR ColumnName[ColumnNumber] =   {"Ch",			"CicState",	"CallerId",		"CalleeId",	 /*"DTMF",*/    "Times",		"StartTime",   "FileName"};
static int    ColumnWidth[ColumnNumber] =  {ChannelWidth,	StatusWidth, CallingWidth,	CalleeWidth, /* DTMFWidth,*/RecordTimesWidth,StartTimeWidth,FileNameWidth};

static LPTSTR	StateName[] = {"空闲", "活动", "收号","振铃","通话","录音","摘机","断线","不可用","通信中","占用","停止"};		
// CRecorderDlg 对话框



int CRecorderDlg::nMaxCh = 0;
CH_INFO CRecorderDlg::ChMap[MAX_CH];
int CRecorderDlg::nIPRBoardId = -1;
int CRecorderDlg::nIPABoardId = -1;
int CRecorderDlg::nSlaverCount = 0;
int CRecorderDlg::nIPRChNum = 0;
IPR_SLAVERADDR CRecorderDlg::IPR_SlaverAddr[MAX_SLAVER_COUNT];
typedef std::map<ULONG,std::shared_ptr<MYIPR_CALL_INFO>> MAP_IPR_CALL_INFO;
static MAP_IPR_CALL_INFO g_IPR_CALL_INFO;

CRecorderDlg::CRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRecorderDlg::IDD, pParent),m_freeCapacity(0),m_totalCapacity(1)
	, m_strFileDir(_T(""))
	, m_strDataBase(_T(""))
	, m_KeepDays(0)
	, m_DBKeepDays(0)
	, m_RecordingSum(0)
	, m_DetailLog(FALSE)
	, m_AutoBackup(0)
	, m_strTotalSize(_T(""))
	, m_strFreeSize(_T(""))
{
	//m_nRecFormat = 2;
	m_nCallFnMode = 0;

	this->log = log4cplus::Logger::getInstance(_T("Recorder"));
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hIconOffhook = AfxGetApp()->LoadIcon(IDI_ICON_OFFHOOK);
	m_hIconOnhook = AfxGetApp()->LoadIcon(IDI_ICON_ONHOOK);
	m_strApplySize = _T("");
}

void CRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DTP, m_ChList);
	DDX_Control(pDX, IDC_RICHEDIT21, m_ctrCapacityView);
	DDX_Text(pDX, IDC_EDIT1, m_strFileDir);
	DDX_Text(pDX, IDC_EDIT_DATABASE, m_strDataBase);
	DDX_Text(pDX, IDC_EDIT2, m_KeepDays);
	DDX_Text(pDX, IDC_EDIT3, m_DBKeepDays);
	DDX_Text(pDX, IDC_EDIT4, m_RecordingSum);
	DDX_Check(pDX, IDC_CHECK1, m_DetailLog);
	DDX_Check(pDX, IDC_CHECK3, m_AutoBackup);
	DDX_Text(pDX, IDC_EDIT_TOTALSIZE, m_strTotalSize);
	DDX_Text(pDX, IDC_EDIT7, m_strFreeSize);
	DDX_Text(pDX, IDC_EDIT6, m_strApplySize);
}

BEGIN_MESSAGE_MAP(CRecorderDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, &CRecorderDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CRecorderDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CRecorderDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CRecorderDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_CHECK1, &CRecorderDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK3, &CRecorderDlg::OnBnClickedCheck3)
	ON_MESSAGE(WM_ICON_NOTIFY, OnTrayNotification)
	ON_WM_SYSCOMMAND()
	ON_WM_CREATE()
	ON_COMMAND(ID_SHOW, &CRecorderDlg::OnShow)
	ON_COMMAND(ID_EXIT, &CRecorderDlg::OnExit)
END_MESSAGE_MAP()


// CRecorderDlg 消息处理程序

BOOL CRecorderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// TODO: Add extra initialization here
	if(!InitCtiBoard())		
	{
		//PostQuitMessage(0);
		//return FALSE;
	}
	//Set event-driven mode
	EVENT_SET_INFO EventSet;		
	EventSet.dwWorkMode = EVENT_CALLBACKA;
	EventSet.lpHandlerParam = EventCallback;
	EventSet.dwUser = (DWORD)this;

	if(SsmSetEvent(-1, -1, TRUE, &EventSet) == -1)
		LOG4CPLUS_ERROR(log, _T("Fail to call SsmSetEvent"));
	if(SsmSetEvent(E_CHG_SpyState, -1, TRUE, &EventSet) == -1)
		LOG4CPLUS_ERROR(log, _T("Fail to call SsmSetEvent when setting E_CHG_SpyState"));

	InitCircuitListCtrl();		//initialize list

	m_strFileDir = ReadRegKeyString("FileDir");
	m_strDataBase = ReadRegKeyString("DataBase");
	m_sqlServerDB.SetConnectionString(m_strDataBase);

	m_KeepDays = ReadRegKeyDWORD("KeepDays");
	m_DBKeepDays = ReadRegKeyDWORD("DBKeepDays");
	m_DetailLog = ReadRegKeyDWORD("DetailLog");
	ReadRegKeyDWORD("AutoBackup") == 1 ? m_AutoBackup =1:NULL;

	m_sqlServerDB.startDataBaseThread();
	
	checkDiskSize();
	UpdateData(FALSE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRecorderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		DrawCapacityView();
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRecorderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


//Initialize board
BOOL CRecorderDlg::InitCtiBoard()
{
	LOG4CPLUS_INFO(log,_T("Application start..."));
	CString szCurPath;				//Current path
	CString szShIndex;					//path to ShIndex.ini
	CString szShConfig;				//path to ShConfig.ini

	CRegKey shKey;
	LONG lResult = shKey.Open(HKEY_LOCAL_MACHINE,"SoftWare\\ShCti");
	if (lResult != ERROR_SUCCESS)
	{
		LOG4CPLUS_ERROR(log, _T("Open ShCti RegKey failed."));
	}
	else{
		ULONG dwLen = MAX_PATH;
		LONG ret = shKey.QueryStringValue("AppPath", szCurPath.GetBuffer(dwLen),&dwLen);
		szCurPath.ReleaseBuffer();
		if(ret != ERROR_SUCCESS){
			LOG4CPLUS_ERROR(log,"Read  AppPath Error:" << ret);
		}

		LOG4CPLUS_DEBUG(log,"ShCti AppPath:" << szCurPath.GetBuffer());
		if(szCurPath.IsEmpty()){
			szCurPath = "C:\\ShCti\\";
		}
		LOG4CPLUS_DEBUG(log,"ShCti AppPath:" << szCurPath.GetBuffer());
		szShIndex = szCurPath;
		szShConfig = szCurPath;
	}


	szShIndex.Append("ShIndex.ini");
	szShConfig.Append("ShConfig.ini");

	//load configuration file and initialize system
	if(SsmStartCti(szShConfig, szShIndex) == -1)
	{
		LOG4CPLUS_ERROR(log, GetSsmLastErrMsg());
		return FALSE;
	}

	//Judge if the number of initialized boards is the same as
	//		   that of boards specified in the configuration file
	int nTotalBoards;
	nTotalBoards = SsmGetMaxUsableBoard();
	if(nTotalBoards != SsmGetMaxCfgBoard())
	{
		LOG4CPLUS_ERROR(log, GetSsmLastErrMsg());
		return FALSE;
	}
	//序列号检查
	DWORD SerialNo = SsmGetPciSerialNo(0);
	LOG4CPLUS_INFO(log,"SerialNo:" << SerialNo);
	if (SerialNo != 190997)
	{
		MessageBox("此程序只允许在特定的版本上允许，请联系开发者","序列号错误",MB_OK);
		LOG4CPLUS_ERROR(log, "此程序只允许在特定的版本上允许，请联系开发者");
		return FALSE;
	}
	for( int i=0; i < nTotalBoards; i++)
	{
		if(SsmGetBoardModel(i) == 0xfd)	//IPRecorder card type is 0xfd
		{
			nIPRBoardId = i;
			LOG4CPLUS_INFO(log, "IPRBoardId:" << nIPRBoardId);
			break;
		}
	}

	for(int i=0; i < nTotalBoards; i++)
	{
		if(SsmGetBoardModel(i) == 0xfe)	//IPAnalyzer card type is 0xfe
		{
			nIPABoardId = i;
			break;
		}
	}

	if(nIPRBoardId == -1)	//If not find IPRecorder card, show error message.
	{
		LOG4CPLUS_INFO(log, "No IPRecorder card, please check!");
	}

	if(nIPABoardId == -1)	//If not find IPRecorder card, show error message.
	{
		LOG4CPLUS_INFO(log, "No IPAnalyzer card, please check!");
	}

	//Get the maximum number of the monitored circuits
	nMaxCh = SpyGetMaxCic();
	if(nMaxCh == -1){
		LOG4CPLUS_ERROR(log, GetSsmLastErrMsg());
	}
	if (nMaxCh < 1)
	{
		nMaxCh = SsmGetMaxCh();	  //retrieve channel amounts declared in configuration file 
		if(nMaxCh == -1)
		{
			LOG4CPLUS_ERROR(log, GetSsmLastErrMsg());
		}
	}
	LOG4CPLUS_DEBUG(log, "MaxCh:" << nMaxCh);

	for(int i = 0; i < nMaxCh; i++)
	{
		SetChannelState(i, CH_IDLE);
		ChMap[i].wRecDirection = MIX_RECORD;	    //mix-record
		ChMap[i].nCallInCh = -1;	
		ChMap[i].nCallOutCh = -1;
		ChMap[i].nRecordTimes = 0;
		ChMap[i].tStartTime = CTime::GetCurrentTime();
		ChMap[i].nChType = SsmGetChType(i);
		ChMap[i].bIgnoreLineVoltage = false;
		if (ChMap[i].nChType == CH_TYPE_ANALOG_RECORD)
		{
			int nResult = SsmGetIgnoreLineVoltage(i);
			if(nResult == 1){
				ChMap[i].bIgnoreLineVoltage = true;
				LOG4CPLUS_INFO(log, "Ingore line voltage.");
			}
		}
		else if (ChMap[i].nChType == CH_TYPE_IPR)
		{
			nIPRChNum++;
		}
		ChMap[i].nCallRef = -1;
		ChMap[i].dwSessionId = 0;
		ChMap[i].nPtlType = -1;
		ChMap[i].nStationId = -1;
		ChMap[i].nRecSlaverId = -1;
		ChMap[i].nFowardingPPort = -1;
		ChMap[i].nFowardingSPort = -1;
		ChMap[i].nPrimaryCodec = -1;
		for(int j=0; j < MAX_ACTIVE_LINE_NUM; j++)
		{
			ChMap[i].nSCCPActiveCallref[j] = 0;
		}
	}

	return TRUE;
}

//Initialize list
void CRecorderDlg::InitCircuitListCtrl()
{

	//m_CicList.SetBkColor(RGB(0,0,0));
	//m_CicList.SetTextColor(RGB(0,255,0));
	m_ChList.SetTextBkColor(RGB(255,255,204));
	m_ChList.SetOutlineColor(RGB(0,0,0));

	DWORD dwExtendedStyle = m_ChList.GetExtendedStyle();
	dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	dwExtendedStyle |= LVS_EX_GRIDLINES; 
	m_ChList.SetExtendedStyle(dwExtendedStyle);

	m_ImageList.Create(16,16,ILC_COLOR16,1,1);
	int iIcon = m_ImageList.Add(m_hIconOnhook);
	LOG4CPLUS_DEBUG(log,"m_hIconOnhook index:" << iIcon);
	iIcon = m_ImageList.Add(m_hIconOffhook);
	LOG4CPLUS_DEBUG(log,"m_hIconOffhook index:" << iIcon);
	m_ChList.SetImageList(&m_ImageList,LVSIL_SMALL);

	LV_COLUMN lvc;
	lvc.mask =  LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;

	for (int i = 0; i< ColumnNumber; i++)
	{
		lvc.iSubItem = i;
		lvc.pszText = ColumnNameCh[i];
		lvc.cx = ColumnWidth[i];
		m_ChList.InsertColumn(i, &lvc);
	}

	CString str;
	for(int i = 0; i < nMaxCh; i++)
	{
		str.Format("%d",i);
		m_ChList.InsertItem(i,str);
	}
}

//Update list
void CRecorderDlg::UpdateCircuitListCtrl(unsigned int nIndex)
{
	CString strNewData;
	LVITEM lvItem={0};
	lvItem.mask = LVIF_IMAGE|LVIF_TEXT|LVIF_STATE;  //文字、图片、状态  
	lvItem.iItem = nIndex;
	lvItem.iSubItem = Channel;    //子列号  
	if ( ChMap[nIndex].nState == CH_TALKING 
		|| ChMap[nIndex].nState == CH_RECORDING
		|| ChMap[nIndex].nState == CH_USING
		|| ChMap[nIndex].nState == CH_ACTIVE)
	{
		lvItem.iImage = 1;    //图片索引号(第2幅图片)  offHook
		
	}else
	{
		lvItem.iImage = 0;  //图片索引号(第一幅图片)  OnHook
	}
	CString str;
	str.Format("%d",nIndex);
	m_ChList.SetItem(&lvItem);
	m_ChList.SetItemText(nIndex,Channel,str);

	m_ChList.SetItemText(nIndex, ChState, ChMap[nIndex].szState);
	m_ChList.SetItemText(nIndex, ChCaller, ChMap[nIndex].szCallerId);
	m_ChList.SetItemText(nIndex, ChCallee, ChMap[nIndex].szCalleeId);
	//m_ChList.SetItemText(nIndex, ChDTMF, ChMap[nIndex].szDtmf);
	//m_CicList.SetItemText(nIndex, ChOutDTMF, CicState[nIndex].szCallOutDtmf);
	strNewData.Format("%d", ChMap[nIndex].nRecordTimes);
	m_ChList.SetItemText(nIndex, ChTimes, strNewData);
	m_ChList.SetItemText(nIndex, ChStartTime , ChMap[nIndex].tStartTime.Format("%Y-%m-%d %H:%M:%S"));
	m_ChList.SetItemText(nIndex, ChFileName, ChMap[nIndex].szFileName);

}


int CALLBACK CRecorderDlg::EventCallback(PSSM_EVENT pEvent)
{
	// TODO: Add your specialized code here and/or call the base class

	//Adopt windows message mechanism
	//	   windows message code：event code + 0x7000(WM_USER)
	static log4cplus::Logger log = log4cplus::Logger::getInstance(_T("Recorder"));
	static CRecorderDlg * This = reinterpret_cast<CRecorderDlg*>(pEvent->dwUser);
	ULONG32 nEventCode = pEvent->wEventCode;	
	switch(nEventCode)
	{
#pragma region E_CHG_SpyState
		//Event notifying the state change of the monitored circuit
		case E_CHG_SpyState:	
		{
			int nCic = pEvent->nReference;
			LOG4CPLUS_DEBUG(log,"Ch:" << nCic << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			UINT32 nNewState = pEvent->dwParam & 0xFFFF;
			if(nCic >= 0 && nCic <= nMaxCh)
			{
				LOG4CPLUS_DEBUG(log, "Ch:" << nCic << "newState:" << GetShStateName(nNewState));
				switch(nNewState)
				{
#pragma region 空闲 //Idle state
				case S_SPY_STANDBY:
					{
						This->StopRecording(nCic);
						ClearChVariable(nCic);
						SetChannelState(nCic,CH_IDLE);
					}
					break;
#pragma endregion 空闲
#pragma region DTMF //Receiving phone number
				case S_SPY_RCVPHONUM:
					{
						if(ChMap[nCic].nState == CH_IDLE){
							SetChannelState(nCic, CH_RCV_PHONUM);
						}			
					}
					break;	
#pragma endregion DTMF
#pragma region 振铃 //Ringing
				case S_SPY_RINGING:
					{
						SetChannelState(nCic, CH_RINGING);
						GetCallerAndCallee(nCic);
						LOG4CPLUS_INFO(log, "Ch:" << nCic << "Get Caller:" << ChMap[nCic].szCallerId << ", Callee:" << ChMap[nCic].szCalleeId);
					}
					break;
#pragma endregion 振铃
#pragma region 通话 //Talking
				case S_SPY_TALKING:
					{
						if(ChMap[nCic].nState == CH_RCV_PHONUM)
						{
							GetCallerAndCallee(nCic);
							LOG4CPLUS_INFO(log, "Ch:" << nCic << "Get Caller:" << ChMap[nCic].szCallerId << ", Callee:" << ChMap[nCic].szCalleeId);
						}

						SetChannelState(nCic, CH_TALKING);
						/*
						if(ChMap[nCic].szCallerId.Compare("4008001100")){
						   ChMap[nCic].szCalleeId.ReleaseBuffer();
						   LOG4CPLUS_INFO(log, "Ch:" << nCic << "主叫号码判断不通过。");
						   break;
						} */

						//Start recording	
						
						//根据主被叫号码判断录音方向
						/*if(!ChMap[nCic].szCallerId.Compare("4008001100")){
							ChMap[nCic].wRecDirection=CALL_OUT_RECORD;
						}else{
							ChMap[nCic].wRecDirection = CALL_IN_RECORD;
						} */


						LOG4CPLUS_INFO(log, "Ch:" <<  nCic << " StartRecording.");
						if(This->StartRecording(nCic)){
							SetChannelState(nCic, CH_RECORDING);
						}
					}
					break;
#pragma endregion 通话
#pragma region S_CALL_UNAVAILABLE
				case S_CALL_UNAVAILABLE:
					{
						SetChannelState(nCic,CH_UNAVAILABLE);
					}
					break;
#pragma endregion S_CALL_UNAVAILABLE
#pragma region unknown
				default:
					{
						LOG4CPLUS_WARN(log, "Ch:" << nCic <<  "UNKNOWN STATE:" << std::hex << nNewState);
					}
					break;
#pragma endregion unknown
				}
			}
			else{
				LOG4CPLUS_WARN(log,"Ch:"<< nCic << " Error, not exsist.");
			}
			This->UpdateCircuitListCtrl(nCic);
		}
		break;
#pragma endregion E_CHG_SpyState
#pragma region E_CHG_RcvDTMF
		//Event generated by the driver when DTMF is received
		case E_CHG_RcvDTMF:
		{
			int nCh		= pEvent->nReference;	//number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			if(nCh != -1)
			{
				char cNewDtmf = (char)(0xFFFF & pEvent->dwParam);	//Newly received DTMF
				LOG4CPLUS_INFO(log, "Ch:" << nCh << " newDTMF:" << cNewDtmf);
				ChMap[nCh].szDtmf.AppendChar(cNewDtmf);
			}
			This->UpdateCircuitListCtrl(nCh);
		}
		break;
#pragma endregion E_CHG_RcvDTMF
#pragma region E_PROC_RecordEnd
		//recording end event
		case E_PROC_RecordEnd:
		{
			//int nCh		= pEvent->nReference;	//number of channel output the event 
			//LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			//if(ChMap[nCh].nState == CH_RECORDING)
			//{
			//	SetChannelState(nCh, CH_PICKUP);					
			//}
			//This->UpdateCircuitListCtrl(nCh);
		}
		break;
#pragma endregion E_PROC_RecordEnd
#pragma region E_CHG_RingCount
		//ring count event
		case E_CHG_RingCount:
		{
			int nCh		= pEvent->nReference;	//number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			if((ChMap[nCh].nState == CH_IDLE ) && (pEvent->dwParam == 2))
			{
				SetChannelState(nCh, CH_RINGING);
				//receive CallerId
				GetCallerAndCallee(nCh);
			}
			This->UpdateCircuitListCtrl(nCh);
		}
		break;
#pragma endregion E_CHG_RingCount
#pragma region E_CHG_HookState
		case E_CHG_HookState:
		{
			//Switching from channel number to circuit number
			int nCh = pEvent->nReference;
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));
#pragma region on hook
			if(pEvent->dwParam == 0){
				LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " S_CALL_STANDBY");
				This->StopRecording(nCh);
				ClearChVariable(nCh);
				SetChannelState(nCh, CH_IDLE);
			}

#pragma endregion on hook
#pragma region off hook
			else if (pEvent->dwParam ==1){
				LOG4CPLUS_DEBUG(log, "Ch:" << nCh<< " S_CALL_PICKUPED");
				if (This->StartRecording(nCh)){
					SetChannelState(nCh, CH_RECORDING);
				}
				else
					SetChannelState(nCh, CH_PICKUP);
			}

#pragma endregion off hook
			This->UpdateCircuitListCtrl(nCh);
		}
		break;
#pragma endregion E_CHG_HookState
#pragma region E_CHG_ChState
		case E_CHG_ChState:
		{
			int nCh = pEvent->nReference; //wParam: number of channel output the event 
			int nNewState = (int)(pEvent->dwParam & 0xFFFF); //lParam:DWORD(HWORD wOldState---old state，LWORD wNewState---new state)
			int nOldState = (int)(pEvent->dwParam >> sizeof(WORD)*8);
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode) << ",NewState:" << GetShStateName(nNewState));
			switch(nNewState) 
			{
#pragma region S_CALL_STANDBY
			case S_CALL_STANDBY:
				{
					if (ChMap[nCh].nState == CH_RECORDING && ChMap[nCh].nChType == CH_TYPE_ANALOG_RECORD){
						This->StopRecording(nCh);
						ClearChVariable(nCh);
					}
					SetChannelState(nCh,CH_IDLE);
				}
				break;
#pragma endregion S_CALL_STANDBY
#pragma region S_CALL_RINGING
			case S_CALL_RINGING:
				{
					SetChannelState(nCh,CH_RINGING);
				}
				break;
#pragma endregion S_CALL_RINGING
#pragma region S_CALL_PICKUPED
			case S_CALL_PICKUPED:
				{
					SetChannelState(nCh,CH_PICKUP);
				}
				break;
#pragma endregion S_CALL_PICKUPED
#pragma region S_CALL_TALKING
			case S_CALL_TALKING:
				{
					SetChannelState(nCh, CH_TALKING);
					if (This->StartRecording(nCh)){
						SetChannelState(nCh, CH_RECORDING);
					}
				}
				break;
#pragma endregion S_CALL_TALKING
#pragma region S_CALL_OFFLINE
			case S_CALL_OFFLINE:
				{
					This->StopRecording(nCh);
					ClearChVariable(nCh);
					SetChannelState(nCh,CH_OFFLINE);
				}
				break;
#pragma endregion S_CALL_OFFLINE
#pragma region S_CALL_UNAVAILABLE
			case S_CALL_UNAVAILABLE:
				{
					SetChannelState(nCh,CH_UNAVAILABLE);
				}
				break;
#pragma endregion S_CALL_UNAVAILABLE
#pragma region S_IPR_COMMUNICATING
			case S_IPR_COMMUNICATING:
				{
					SetChannelState(nCh, CH_COMMUNICATING);
				}
				break;
#pragma endregion S_IPR_COMMUNICATING
#pragma region S_IPR_USING
			case S_IPR_USING:
				{
					SetChannelState(nCh, CH_USING);
				}
				break;
#pragma endregion S_IPR_USING
#pragma region default

			default:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCh << GetShStateName(nNewState)<< " do nothing.");
				}
				break;
#pragma endregion default
			}
			This->UpdateCircuitListCtrl(nCh);
		}
		break;
#pragma endregion E_CHG_ChState
#pragma region E_CHG_PcmLinkStatus
		case E_CHG_PcmLinkStatus:
		{
			LOG4CPLUS_DEBUG(log,"Pcm:" << pEvent->nReference << ",SanHui nEventCode:" << GetShEventName(nEventCode));
		}
		break;
#pragma endregion E_CHG_PcmLinkStatus
#pragma region E_RCV_IPR_DChannel
		case E_RCV_IPR_DChannel:
		{
			//LOG4CPLUS_DEBUG(log, "SanHui nEventCode:" << GetShEventName(nEventCode) << ", State:" << GetDSTStateName(pEvent->dwParam));
			int nPtlType = pEvent->dwXtraInfo >> 16;
			int nStationId = pEvent->dwXtraInfo & 0xffff;
			switch(nPtlType) 
			{
#pragma region PTL_SIP
			case PTL_SIP:
			case PTL_CISCO_SKINNY:
				if(pEvent->dwParam >= DE_CALL_IN_PROGRESS && pEvent->dwParam <= DE_CALL_REJECTED)
				{
					//LOG4CPLUS_DEBUG(log, "PTL_SIP");
					PIPR_CALL_INFO pCallInfo = (PIPR_CALL_INFO)pEvent->pvBuffer;
					LOG4CPLUS_TRACE(log, "StationId:" << nStationId<< ",CallRef:" << pCallInfo->CallRef 
						<< ",SanHui nEventCode:" << GetShEventName(nEventCode) << ", State:" << GetDSTStateName(pEvent->dwParam));
					int key  = pCallInfo->CallRef;
					
					if(pEvent->dwParam == DE_CALL_RELEASED 
						|| pEvent->dwParam == DE_CALL_SUSPENDED 
						|| pEvent->dwParam == DE_CALL_REJECTED 
						|| pEvent->dwParam == DE_CALL_ABANDONED)
					{
						MAP_IPR_CALL_INFO::const_iterator it = g_IPR_CALL_INFO.find(key);
						if (it != g_IPR_CALL_INFO.end())
						{
							g_IPR_CALL_INFO.erase(it);
						}
						LOG4CPLUS_TRACE(log, "StationId:" << nStationId<< ",CallRef:" << pCallInfo->CallRef << ",erase IPR CALL INFO g_IPR_CALL_INFO.size:" << g_IPR_CALL_INFO.size());
					}
					else if(pEvent->dwParam >= DE_CALL_IN_PROGRESS && pEvent->dwParam <= DE_CALL_CONNECTED)
					{
						MAP_IPR_CALL_INFO::const_iterator it = g_IPR_CALL_INFO.find(key);
						if (it == g_IPR_CALL_INFO.end())
						{
							PMYIPR_CALL_INFO pCInfo = new MYIPR_CALL_INFO(*pCallInfo);
							pCInfo->m_nState = pEvent->dwParam;

							g_IPR_CALL_INFO.insert(MAP_IPR_CALL_INFO::value_type(key,pCInfo));
							LOG4CPLUS_DEBUG(log, "StationId:" << nStationId
								<<",CallRef:" << pCallInfo->CallRef
								<< ",insert IPR CALL INFO g_IPR_CALL_INFO.size: " << g_IPR_CALL_INFO.size()
								<<",CallSource:" << pCallInfo->CallSource
								<<",Cause:" << pCallInfo->Cause
								<<",CallerId:" << pCallInfo->szCallerId
								<< ",CalledId:" << pCallInfo->szCalledId
								<< ",ReferredBy:" << pCallInfo->szReferredBy
								<<", ReferTo:" << pCallInfo->szReferTo);
						}
						else {
							it->second->m_nState = pEvent->dwParam;
						}
						//LOG4CPLUS_TRACE(log, "g_IPR_CALL_INFO.size:" << g_IPR_CALL_INFO.size());

						if (pEvent->dwParam == DE_CALL_CONNECTED)//开始通话
						{
							//查找对应的IPA通道
							bool bFind =false;
							int ipaCh ;
							for (ipaCh=0; ipaCh<nMaxCh; ipaCh++)
							{
								if (ChMap[ipaCh].nChType == CH_TYPE_IPA 
									&& (ChMap[ipaCh].nCallRef == pCallInfo->CallRef)
									&& ChMap[ipaCh].dwSessionId != 0)
								{
									bFind = true;
									break;
								}
							}
							//开始通话后在IPA通道上找到dwSessionId，表示此通道上已经有媒体 
							//并且在E_RCV_IPR_MEDIA_SESSION_STARTED逻辑中还没有录音
							if (bFind)
							{
								LOG4CPLUS_INFO(log,"StationId:" << nStationId
									<<",CallRef:" << pCallInfo->CallRef 
									<< ", find IPA channel:" << ipaCh);
								int iprCh = SerchIdleSlaverAndIPA(ipaCh, CALL_OUT_RECORD);
								if (iprCh >0)
								{
									if(This->StartRecording(iprCh)){
										SetChannelState(iprCh, CH_ACTIVE);
									}
									LOG4CPLUS_INFO(log, "Ch:" << ipaCh << ",SessionId:" << ChMap[ipaCh].dwSessionId 
										<< ",CallRef:" << ChMap[ipaCh].nCallRef
										<< ",StationId:" << ChMap[ipaCh].nStationId<< " Start record");
								}
								This->UpdateCircuitListCtrl(iprCh);
							}
						}
					}
				}			
			break;
#pragma endregion PTL_CISCO_SKINNY
#pragma region DE_MSG_CHG
			case DE_MSG_CHG:
				{
					LOG4CPLUS_DEBUG(log, "StationId:" << nStationId << "," << (const char *)pEvent->pvBuffer);
				}
				break;
#pragma endregion DE_MSG_CHG
#pragma region DEFAULT
		default:
			if(pEvent->dwParam == DE_OFFHOOK 
				|| pEvent->dwParam == DE_ONHOOK
				|| pEvent->dwParam == DE_RING_ON
				|| pEvent->dwParam == DE_RING_OFF)	//control by dchannnel event example
			{
				LOG4CPLUS_WARN(log, "unknown control,do nothing,:0x" << std::hex << pEvent->dwParam);			
			}
			break;
#pragma endregion DEFAULT
			}
		}
		break;
#pragma endregion E_RCV_IPR_DChannel
#pragma region E_RCV_IPR_DONGLE_ADDED
		case E_RCV_IPR_DONGLE_ADDED:
		case E_RCV_IPR_DONGLE_REMOVED:
		case E_RCV_IPA_DONGLE_ADDED:
		case E_RCV_IPA_DONGLE_REMOVED:
		case E_RCV_IPA_APPLICATION_PENDING:
			LOG4CPLUS_DEBUG(log, "SanHui nEventCode:" << GetShEventName(nEventCode));
			break;
#pragma endregion E_RCV_IPA_APPLICATION_PENDING
#pragma region E_RCV_IPR_STATION_ADDED
		case E_RCV_IPR_STATION_ADDED:
			{
				StationInfoEx SInfoEx;
				int nStationId = pEvent->dwXtraInfo & 0xffff;
				SsmIPRGetStationInfoEx(nIPABoardId, nStationId, &SInfoEx);
				LOG4CPLUS_INFO(log,"SanHui nEventCode:" << GetShEventName(nEventCode) << " StationId:" << SInfoEx.nStationId 
					<< " Call Ctrl Protocal: " << (int)SInfoEx.ucCallCtrlPtl
					<< " IP: "<< (int)SInfoEx.CallCtrlAddr.S_un_b.s_b1 << "." 
					<< (int)SInfoEx.CallCtrlAddr.S_un_b.s_b2 << "."
					<< (int)SInfoEx.CallCtrlAddr.S_un_b.s_b3 << "."
					<< (int)SInfoEx.CallCtrlAddr.S_un_b.s_b4
					<< ":" << SInfoEx.CallCtrlAddr.usPort
					<< " MAC: " << std::hex <<(int)SInfoEx.ucMacAddr[0] << "-"
					<< (int)SInfoEx.ucMacAddr[1] << "-"
					<< (int)SInfoEx.ucMacAddr[2] << "-" 
					<< (int)SInfoEx.ucMacAddr[3] << "-"
					<< (int)SInfoEx.ucMacAddr[4] << "-"
					<< (int)SInfoEx.ucMacAddr[5]);
				}
			break;
		case E_RCV_IPR_STATION_REMOVED:
			LOG4CPLUS_DEBUG(log, "SanHui nEventCode:" << GetShEventName(nEventCode) << "protocol:" 
				<<  (pEvent->dwXtraInfo >> 16) << "StationId:" << (pEvent->dwXtraInfo & 0xffff));
			break;
#pragma endregion E_RCV_IPR_STATION_REMOVED
#pragma region E_RCV_IPR_AUTH_OVERFLOW
		case E_RCV_IPR_AUTH_OVERFLOW:
			LOG4CPLUS_ERROR(log, "SanHui nEventCode:" << GetShEventName(nEventCode));
			break;
#pragma endregion E_RCV_IPR_AUTH_OVERFLOW
#pragma region E_IPR_SLAVER_INIT_CB
		case E_IPR_SLAVER_INIT_CB:
		case E_IPR_START_SLAVER_CB:
		case E_IPR_CLOSE_SLAVER_CB:
			{
				int nSlaverId = pEvent->dwParam >> 16;
				int nResult = pEvent->dwParam & 0xffff;
				LOG4CPLUS_INFO(log, "SanHui nEventCode:" << GetShEventName(nEventCode) << ", SlaverId:" << nSlaverId << ", result:" << GetSalverResultMsg(nResult));
				ScanSlaver();
			}
			break;
#pragma endregion E_IPR_CLOSE_SLAVER_CB
#pragma region E_RCV_IPR_MEDIA_SESSION_STARTED
		case E_RCV_IPR_MEDIA_SESSION_STARTED:
		//case E_RCV_IPR_AUX_MEDIA_SESSION_STARTED:
			{
				LOG4CPLUS_DEBUG(log, "Ch:" << pEvent->nReference << ",SanHui nEventCode:" << GetShEventName(nEventCode));
				pIPR_SessionInfo pSessionInfo = (pIPR_SessionInfo)pEvent->pvBuffer;
				int nPtlType = pEvent->dwXtraInfo >> 16;
				int nStationId = pEvent->dwXtraInfo & 0xffff;
				int nCh = pEvent->nReference;
				ChMap[nCh].dwSessionId = pSessionInfo->dwSessionId;
				ChMap[nCh].nCallRef = pSessionInfo->nCallRef;
				ChMap[nCh].nStationId = nStationId;
				ChMap[nCh].nPtlType = nPtlType;
				ChMap[nCh].nPrimaryCodec = pSessionInfo->nPrimaryCodec;

				ChMap[nCh].szIPP.Format("%d.%d.%d.%d:%d", pSessionInfo->PrimaryAddr.S_un_b.s_b1, pSessionInfo->PrimaryAddr.S_un_b.s_b2, 
					pSessionInfo->PrimaryAddr.S_un_b.s_b3, pSessionInfo->PrimaryAddr.S_un_b.s_b4, pSessionInfo->PrimaryAddr.usPort);
				ChMap[nCh].szIPS.Format("%d.%d.%d.%d:%d", pSessionInfo->SecondaryAddr.S_un_b.s_b1, pSessionInfo->SecondaryAddr.S_un_b.s_b2, 
					pSessionInfo->SecondaryAddr.S_un_b.s_b3, pSessionInfo->SecondaryAddr.S_un_b.s_b4, pSessionInfo->SecondaryAddr.usPort);


				//关联主被叫号码等消息
				int key = ChMap[nCh].nCallRef;

				MAP_IPR_CALL_INFO::const_iterator it = g_IPR_CALL_INFO.find(key);

				if(it == g_IPR_CALL_INFO.end())
				{
					LOG4CPLUS_ERROR(log, "Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId 
						<< ",CallRef:" << ChMap[nCh].nCallRef
						<< ",StationId:" << nStationId << " not find idle IPR channel");
					break;
				}
				else{
					ChMap[nCh].szCallerId = it->second->szCallerId;
					ChMap[nCh].szCalleeId = it->second->szCalledId;
				}

				LOG4CPLUS_DEBUG(log,  "Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId
					<< ",CallRef:" << ChMap[nCh].nCallRef
					<< ",StationId:" << ChMap[nCh].nStationId 
					<< ",nPtlType:" << ChMap[nCh].nPtlType
					<< ",PrimaryIP:" << ChMap[nCh].szIPP.GetBuffer()
					<< ",PrimaryCodec:" << ChMap[nCh].nPrimaryCodec
					<< ",SecondaryIP:" << ChMap[nCh].szIPS.GetBuffer()
					<< ",szCallerId:" << ChMap[nCh].szCallerId
					<< ",szCalleeId:" << ChMap[nCh].szCalleeId);
				
				if (it->second->m_nState == DE_CALL_CONNECTED)
				{
					int iprCh = SerchIdleSlaverAndIPA(nCh, CALL_OUT_RECORD);
					if (iprCh >0)
					{
						LOG4CPLUS_INFO(log,"Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId
							<< ",CallRef:" << ChMap[nCh].nCallRef
							<< ",StationId:" << ChMap[nCh].nStationId 
							<< ",get a IPA channel:" << iprCh);

						if(This->StartRecording(iprCh)){
							SetChannelState(iprCh, CH_ACTIVE);
						}
						LOG4CPLUS_INFO(log, "Ch:" << iprCh << ",SessionId:" << ChMap[nCh].dwSessionId 
							<< ",CallRef:" << ChMap[nCh].nCallRef
							<< ",StationId:" << ChMap[nCh].nStationId<< " Start record");
					}
					This->UpdateCircuitListCtrl(iprCh);
				}

				This->UpdateCircuitListCtrl(nCh);
				ScanSlaver();
			}
			break;
#pragma endregion E_RCV_IPR_MEDIA_SESSION_STARTED
#pragma region E_RCV_IPR_MEDIA_SESSION_STOPED
		case E_RCV_IPR_MEDIA_SESSION_STOPED:
		//case E_RCV_IPR_AUX_MEDIA_SESSION_STOPED:
			{
				LOG4CPLUS_DEBUG(log, "Ch:" << pEvent->nReference << ",SanHui nEventCode:" << GetShEventName(nEventCode));

				pIPR_SessionInfo pSessionInfo = (pIPR_SessionInfo)pEvent->pvBuffer;
				int nPtlType = pEvent->dwXtraInfo >> 16;
				int nStationId = pEvent->dwXtraInfo & 0xffff;
				int nCh = pEvent->nReference;
				LOG4CPLUS_DEBUG(log,  "Ch:" << nCh <<",CallRef:" << pSessionInfo->nCallRef
					<< ",StationId:" << nStationId 
					<< ",SessionId:" << pSessionInfo->dwSessionId
					<< ",PrimaryIP:" << (int)pSessionInfo->PrimaryAddr.S_un_b.s_b1 << "." << (int)pSessionInfo->PrimaryAddr.S_un_b.s_b2 << "." << (int)pSessionInfo->PrimaryAddr.S_un_b.s_b3 << "." << (int)pSessionInfo->PrimaryAddr.S_un_b.s_b4 << ":" << pSessionInfo->PrimaryAddr.usPort
					<< ",PrimaryCodec:" << pSessionInfo->nPrimaryCodec
					<< ",SecondaryIP:" <<(int)pSessionInfo->SecondaryAddr.S_un_b.s_b1 << "." <<(int)pSessionInfo->SecondaryAddr.S_un_b.s_b2 << "." << (int)pSessionInfo->SecondaryAddr.S_un_b.s_b3 << "." << (int)pSessionInfo->SecondaryAddr.S_un_b.s_b4 << ":" << (int)pSessionInfo->SecondaryAddr.usPort
					<< ",SecondaryCodec:" << pSessionInfo->nSecondaryCodec);
				

				BOOL bFind = FALSE;
				int iprCh = 0;
				for(iprCh=0; iprCh<nMaxCh; iprCh++)
				{
					if(ChMap[iprCh].dwSessionId && ChMap[iprCh].dwSessionId == ChMap[nCh].dwSessionId && ChMap[iprCh].nChType == CH_TYPE_IPR)
					{
						bFind = TRUE;
						break;
					}
				} 
				if(bFind)
				{
					LOG4CPLUS_INFO(log,"Ch:" << nCh << " find binding IPR channel:" << iprCh);
					LOG4CPLUS_DEBUG(log, "Ch:" << iprCh << ",stop recording.");
					This->StopRecording(iprCh);
				}
				else{
					LOG4CPLUS_ERROR(log, "Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId <<" not find binding IPR channel");
				}
				ClearChVariable(nCh);
			}
			break;
#pragma endregion E_RCV_IPR_AUX_MEDIA_SESSION_STOPED
#pragma region E_IPR_ACTIVE_AND_REC_CB
		case E_IPR_ACTIVE_AND_REC_CB:
			{
				int nCh = pEvent->nReference;
				LOG4CPLUS_DEBUG(log, "Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));
				int err = (int)(pEvent->dwParam & 0xffff);
				if(err)	//error occurred
				{
					LOG4CPLUS_ERROR(log, "Ch:" << pEvent->nReference << ",SessionId:" << pEvent->dwSubReason << ",error code:"<< err);
					if(err == 0x6)
					{
						if(This->StartRecording(nCh)){
							SetChannelState(nCh, CH_ACTIVE);
						}
						LOG4CPLUS_INFO(log, "Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId 
							<< ",CallRef:" << ChMap[nCh].nCallRef
							<< ",StationId:" << ChMap[nCh].nStationId<< " ReStart record.");
					}
					else{
						ClearChVariable(nCh);
					}
					break;
				}
				BOOL bFind = FALSE;
				int ipaCh = 0;
				for(ipaCh = 0; ipaCh < nMaxCh; ipaCh++)
				{
					if(ChMap[ipaCh].dwSessionId && ChMap[ipaCh].dwSessionId == ChMap[nCh].dwSessionId && ChMap[ipaCh].nChType == CH_TYPE_IPA)
					{
						bFind = TRUE;
						break;
					}
				}
				if(!bFind){
					LOG4CPLUS_ERROR(log, "Ch:" << nCh << " not find binding IPA channel");
					break;
				}else{
					LOG4CPLUS_INFO(log,"Ch:" << nCh << " find binding IPA channel:" << ipaCh);
				}


				if(SsmIPRSendSession(ipaCh, ChMap[nCh].szIPP_Rec.GetBuffer(), ChMap[nCh].nFowardingPPort, 
					ChMap[nCh].szIPS_Rec.GetBuffer(), ChMap[nCh].nFowardingSPort) != 0)
				{
					LOG4CPLUS_ERROR(log, "Ch:" << ipaCh << ","<< GetSsmLastErrMsg());
				}
				else{
					LOG4CPLUS_INFO(log,"Ch:" << ipaCh << " SendSession to " << ChMap[nCh].szIPS_Rec.GetBuffer() << ":" << ChMap[nCh].nFowardingPPort
						<<"; " << ChMap[nCh].szIPS_Rec.GetBuffer() <<":"<<  ChMap[nCh].nFowardingSPort);
				}
				SetChannelState(nCh,CH_RECORDING);
				This->UpdateCircuitListCtrl(nCh);
			}
			break;
#pragma endregion E_IPR_ACTIVE_AND_REC_CB
#pragma region E_IPR_DEACTIVE_AND_STOPREC_CB
		case E_IPR_DEACTIVE_AND_STOPREC_CB:
			{
				LOG4CPLUS_DEBUG(log, "Ch:" << pEvent->nReference << ",SanHui nEventCode:" << GetShEventName(nEventCode));
				if(pEvent->dwParam & 0xffff)	//error occurred
				{
					LOG4CPLUS_ERROR(log, "Ch:" << pEvent->nReference << ",error code:"<< (int)(pEvent->dwParam & 0xffff));
					break;
				}
				//if((int)(pEvent->dwParam >> 16) == IPR_SlaverAddr[This->nSlaverSelectedIndex].nRecSlaverID)//update recorder slaver resoures
				//{
				//	ScanSlaver();
				//}
			}
			break;
#pragma endregion E_IPR_DEACTIVE_AND_STOPREC_CB
#pragma region E_IPR_LINK_REC_SLAVER_CONNECTED
		case E_IPR_LINK_REC_SLAVER_CONNECTED:
		case E_IPR_LINK_REC_SLAVER_DISCONNECTED:
			LOG4CPLUS_DEBUG(log, "Ch:" << pEvent->nReference << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			ScanSlaver();//update recorder slaver resoures
			StartSlaver();
			break;
#pragma endregion E_IPR_LINK_REC_SLAVER_DISCONNECTED
#pragma region E_IPR_RCV_DTMF
		case E_IPR_RCV_DTMF:
		case E_RCV_IPR_MEDIA_SESSION_FOWARDING:
		case E_RCV_IPR_MEDIA_SESSION_FOWARD_STOPED:
		case E_IPR_PAUSE_REC_CB:
		case E_IPR_RESTART_REC_CB:
			LOG4CPLUS_DEBUG(log, "Ch:" << pEvent->nReference << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			break;
#pragma endregion E_IPR_RESTART_REC_CB
#pragma region E_SYS_BargeIn
		case E_SYS_BargeIn: //BargeIn event is detected 
		{
			int nCh = pEvent->nReference; //wParam: number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			if(ChMap[nCh].bIgnoreLineVoltage && pEvent->dwParam == 1)	//ignore voltage-detecting
			{
				if(ChMap[nCh].nState == CH_IDLE)
				{
					if(This->StartRecording(nCh)){
						SetChannelState(nCh, CH_RECORDING);
					}
				}
			}	
		}
		break;
#pragma endregion E_SYS_BargeIn
#pragma region E_SYS_NoSound
		case E_SYS_NoSound:
		{
			int nCh = pEvent->nReference; //wParam: number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			if (ChMap[nCh].bIgnoreLineVoltage)
			{
				This->StopRecording(nCh);
				ClearChVariable(nCh);
				SetChannelState(nCh, CH_IDLE);
			}
		}
		break;
#pragma endregion E_SYS_NoSound
		default:
		{
			INT32 nCh= pEvent->nReference;
			LOG4CPLUS_WARN(log, "Ch:" << nCh << " Do nothing Event:" << GetShEventName(nEventCode));
		}
		break;
	}
	This->UpdateData(FALSE);
	return 0;
}


void CRecorderDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	//Close board driver
	LOG4CPLUS_INFO(log,_T("Application exit..."));
	for (int i = 0 ;i < nSlaverCount; i++)
	{
		if(SsmIPRCloseRecSlaver(nIPRBoardId, IPR_SlaverAddr[i].nRecSlaverID)<0)
			LOG4CPLUS_ERROR(log, GetSsmLastErrMsg());
	}
	
	if(SsmCloseCti() == -1)
		LOG4CPLUS_ERROR(log,GetSsmLastErrMsg());
	m_sqlServerDB.stopDataBaseThread();
}


void CRecorderDlg::DrawCapacityView()
{
	CDC * dc = this->m_ctrCapacityView.GetDC();
	const unsigned int radius = 60;
	const double pi =3.141592;
	CPen pen,*pOldPen;
	CBrush brush,*pOldBrush;
	COLORREF penColor = RGB(0xFF,0xFF,0xFF);
	COLORREF freeColor = RGB(0x00,0x33,0x99);
	COLORREF applyColor = RGB(0xCC,0x33,0x99);

	CRect rect;
	CWnd *pWnd = NULL;
	CDC *pDC = NULL;
	pWnd = GetDlgItem(IDC_STATIC_APPLY); 
	pDC = pWnd->GetDC();
	pWnd->GetClientRect(&rect);
	FillRect(pDC->GetSafeHdc(),&rect,CBrush(applyColor));
	ReleaseDC(pDC);

	pWnd = GetDlgItem(IDC_STATIC_FREE); 
	pDC = pWnd->GetDC();
	pWnd->GetClientRect(&rect);
	FillRect(pDC->GetSafeHdc(),&rect,CBrush(freeColor));
	ReleaseDC(pDC);
	
	pen.CreatePen(PS_NULL,0,penColor);
	pOldPen = dc->SelectObject(&pen);  

	brush.CreateSolidBrush(freeColor);
	pOldBrush = dc->SelectObject(&brush);

	unsigned int nXRadial1 = radius*2;
	unsigned int nYRadial1 = radius;

	unsigned int nXRadial2 = radius + radius * cos(m_freeCapacity*360/m_totalCapacity*pi/180);
	unsigned int nYRadial2 = radius - radius * sin(m_freeCapacity*360/m_totalCapacity*pi/180);

	dc->Pie(0, 0, radius*2, radius*2, nXRadial1, nYRadial1, nXRadial2, nYRadial2); 
	nXRadial1 = nXRadial2;
	nYRadial1 = nYRadial2;
	nXRadial2 = radius * 2;
	nYRadial2 = radius;

	brush.DeleteObject();
	brush.CreateSolidBrush(applyColor);
	dc->SelectObject(&brush);

	dc->Pie(0, 0, radius*2, radius*2, nXRadial1, nYRadial1, nXRadial2, nYRadial2); 
	//Release GDI Object       
	dc->SelectObject(pOldPen);         
	dc->SelectObject(pOldBrush);
	pen.DeleteObject();
	brush.DeleteObject();
	ReleaseDC(dc);
	UpdateData(FALSE);
}


void CRecorderDlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	
	TCHAR szDir[MAX_PATH];
	
	BROWSEINFO bi;
	ITEMIDLIST *pidl;

	bi.hwndOwner = this->m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szDir;
	bi.lpszTitle = "请选择目录";
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	bi.lpfn = NULL;
	bi.lParam = 0;
	bi.iImage = 0;

	pidl = SHBrowseForFolder(&bi);
	if(pidl == NULL)
		return;
	if(!SHGetPathFromIDList(pidl, szDir)) 
		return;

	m_strFileDir = szDir;
	SetRegKey("FileDir",m_strFileDir);
	checkDiskSize();
	UpdateData(FALSE);
}

void CRecorderDlg::SetRegKey(CString name, CString strValue)
{
	CRegKey shKey;

	LONG lResult = shKey.Create(HKEY_LOCAL_MACHINE,"SoftWare\\Recorder");
	if (lResult != ERROR_SUCCESS)
	{
		LOG4CPLUS_ERROR(log, _T("Create Recorder RegKey failed."));
	}
	else{
		shKey.SetStringValue(name, strValue,REG_SZ);
	}
	return ;
}

void CRecorderDlg::SetRegKey( CString name, DWORD value )
{
	CRegKey shKey;

	LONG lResult = shKey.Create(HKEY_LOCAL_MACHINE,"SoftWare\\Recorder");
	if (lResult != ERROR_SUCCESS)
	{
		LOG4CPLUS_ERROR(log, _T("Create Recorder RegKey failed."));
	}
	else{
		shKey.SetDWORDValue(name, value);
	}
}

CString CRecorderDlg::ReadRegKeyString(CString name)
{
	CRegKey shKey;
	CString strValue;
	LONG lResult = shKey.Open(HKEY_LOCAL_MACHINE,"SoftWare\\Recorder");
	if (lResult != ERROR_SUCCESS)
	{
		LOG4CPLUS_ERROR(log, _T("Open Recorder RegKey failed."));
	}
	else{
		ULONG dwLen = MAX_PATH;
		shKey.QueryStringValue(name, strValue.GetBuffer(dwLen),&dwLen);
		strValue.ReleaseBuffer();
	}
	return strValue;
}


void CRecorderDlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	COLEDBDataLink dl;
	m_strDataBase = dl.Edit(m_strDataBase, this->m_hWnd);
	m_sqlServerDB.SetConnectionString(m_strDataBase);
	SetRegKey("DataBase",m_strDataBase);
	UpdateData(FALSE);
}


void CRecorderDlg::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	SetRegKey("KeepDays",m_KeepDays);
	UpdateData(FALSE);
}

DWORD CRecorderDlg::ReadRegKeyDWORD( CString name )
{
	CRegKey shKey;
	DWORD dwValue = 360;
	LONG lResult = shKey.Open(HKEY_LOCAL_MACHINE,"SoftWare\\Recorder");
	if (lResult != ERROR_SUCCESS)
	{
		LOG4CPLUS_ERROR(log, _T("Open Recorder RegKey failed."));
	}
	else{
		shKey.QueryDWORDValue(name, dwValue);
	}
	return dwValue;
}


void CRecorderDlg::OnBnClickedButton4()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	SetRegKey("DBKeepDays",m_DBKeepDays);
	UpdateData(FALSE);
}


void CRecorderDlg::OnBnClickedCheck1()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_DetailLog)
	{
		log4cplus::Logger::getRoot().setLogLevel(log4cplus::ALL_LOG_LEVEL);
	}else{
		log4cplus::Logger::getRoot().setLogLevel(log4cplus::INFO_LOG_LEVEL);
	}

	SetRegKey("DetailLog",m_DetailLog);
	UpdateData(FALSE);
}


void CRecorderDlg::OnBnClickedCheck3()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	SetRegKey("AutoBackup",m_AutoBackup);
	UpdateData(FALSE);
}


void CRecorderDlg::checkDiskSize(void)
{

	CString TotalDiskSize, FreeDiskSize, ApplySize;
	ULARGE_INTEGER lpuse;
	ULARGE_INTEGER lptotal;
	ULARGE_INTEGER lpfree;
	if(GetDiskFreeSpaceEx(m_strFileDir,&lpuse,&lptotal,&lpfree))
	{ 
		//得到DiskName盘符的的总容量、已用空间大小、剩余空间大小
		m_freeCapacity = lpuse.QuadPart;
		m_totalCapacity = lptotal.QuadPart;
		LOG4CPLUS_DEBUG(log, "当前磁盘:" << m_strFileDir);

		TotalDiskSize.Format("总容量:%4.2fGB",lptotal.QuadPart/1024.0/1024.0/1024.0);
		m_strTotalSize = TotalDiskSize;
		LOG4CPLUS_DEBUG(log, TotalDiskSize);

		FreeDiskSize.Format("可用:%4.2fGB",lpuse.QuadPart/1024.0/1024.0/1024.0);
		m_strFreeSize = FreeDiskSize;
		LOG4CPLUS_DEBUG(log, FreeDiskSize);

		ApplySize.Format("已用:%4.2fGB",(lptotal.QuadPart - lpuse.QuadPart)/1024.0/1024.0/1024.0);
		m_strApplySize = ApplySize;
		LOG4CPLUS_DEBUG(log, ApplySize);
	}else{
		LOG4CPLUS_ERROR(log, "query " << m_strFileDir << " error.");
	}
	DrawCapacityView();
}


bool CRecorderDlg::StopRecording(unsigned long nCh)
{
	/*if(ChMap[nCh].nState != CH_RECORDING){
		return false;
	}*/
	LOG4CPLUS_TRACE(log,"Ch:" << nCh << " Stop recording.");

#pragma region E1
	if (ChMap[nCh].nChType == CH_TYPE_E1_RECORD)
	{
		if(m_nCallFnMode == 0)				
		{
			//stop recording
			if(SpyStopRecToFile(nCh) == -1){
				LOG4CPLUS_ERROR(log, "Ch:" << nCh << ","<< GetSsmLastErrMsg());
				return false;
			}
		}
		//Call the function with channel number as its parameter
		else
		{
			if(ChMap[nCh].wRecDirection == CALL_IN_RECORD)
			{
				if(SsmStopRecToFile(ChMap[nCh].nCallInCh) == -1){
					LOG4CPLUS_ERROR(log, "Ch:" << nCh << ","<< GetSsmLastErrMsg());
					return false;
				}
			}
			else if(ChMap[nCh].wRecDirection == CALL_OUT_RECORD)
			{
				if(SsmStopRecToFile(ChMap[nCh].nCallOutCh) == -1){
					LOG4CPLUS_ERROR(log, "Ch:" << nCh << ","<< GetSsmLastErrMsg());
					return false;
				}
			}
			else
			{
				if(SsmSetRecMixer(ChMap[nCh].nCallInCh, FALSE, 0) == -1)//Turn off the record mixer
					LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmSetRecMixer"));
				if(SsmStopLinkFrom(ChMap[nCh].nCallOutCh, ChMap[nCh].nCallInCh) == -1)//Cut off the bus connect from outgoing channel to incoming channel
					LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmStopLinkFrom"));
				if(SsmStopRecToFile(ChMap[nCh].nCallInCh) == -1)		//Stop recording
				{
					LOG4CPLUS_ERROR(log, "Ch:" << nCh << ","<< GetSsmLastErrMsg());
					return false;
				}
			}
		}
	}
#pragma endregion E1
#pragma region ANALOG_RECORD
	else if (ChMap[nCh].nChType == CH_TYPE_ANALOG_RECORD)
	{
		if(SsmStopRecToFile(nCh) == -1){  //stop recording
			LOG4CPLUS_ERROR(log, "Ch:" << nCh << ","<< GetSsmLastErrMsg());
			return false;
		}
	}
#pragma endregion ANALOG_RECORD
#pragma region IPR
	else if (ChMap[nCh].nChType == CH_TYPE_IPR)
	{
		if(SsmIPRDeActiveAndStopRecToFile(nCh) != 0)
		{
			LOG4CPLUS_ERROR(log, "Ch:" << nCh << ","<< GetSsmLastErrMsg());
			return false;
		}
	}
#pragma endregion IPR
	ChMap[nCh].tEndTime = CTime::GetCurrentTime();
	ChMap[nCh].sql = "update  RecordLog set EndTime= '"+ChMap[nCh].tEndTime.Format("%Y-%m-%d %H:%M:%S") + "'";
	ChMap[nCh].sql += "  where F_Path='" + ChMap[nCh].szFileName + "'";
	m_sqlServerDB.addSql2Queue(ChMap[nCh].sql.GetBuffer());
	LOG4CPLUS_TRACE(log, "Ch:" << nCh << " addSql2Queue:" << ChMap[nCh].sql.GetBuffer());
	m_RecordingSum--;
	checkDiskSize();
	return true;
}

bool CRecorderDlg::StartRecording(unsigned long nCh){
	SYSTEMTIME st;
	GetLocalTime(&st);
	CString _CallerId = ChMap[nCh].szCallerId;
	CString _CalleeId = ChMap[nCh].szCalleeId;
	_CallerId.Replace(':','_');
	_CallerId.Replace('@','A');
	_CalleeId.Replace(':','_');
	_CalleeId.Replace('@','A');

	ChMap[nCh].szFileName.Format("%s\\%04d\\%02d\\%02d\\%04d%02d%02d%02d%02d%02d_%s_%s.wav", m_strFileDir, 
		st.wYear, st.wMonth, st.wDay,
		st.wYear, st.wMonth, st.wDay, 
		st.wHour, st.wMinute, st.wSecond,
		_CallerId, _CalleeId);

	TCHAR szFile[MAX_PATH];
	CString szDir = ChMap[nCh].szFileName;
	lstrcpy(szFile,szDir.GetBuffer());
	szDir = szDir.Left(szDir.ReverseFind('\\'));
	CreateMultipleDirectory(szDir);
	LOG4CPLUS_TRACE(log, "Ch:" << nCh << ", record file:" << szFile);

#pragma region E1
	if (ChMap[nCh].nChType == CH_TYPE_E1_RECORD)
	{
		if(m_nCallFnMode == 0)	//Call the function with circuit number as its parameter
		{
			if(SpyRecToFile(nCh, ChMap[nCh].wRecDirection, szFile, -1, 0L, -1, -1, 0) == -1){
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SpyRecToFile"));
				return false;
			}
		}
		else if(m_nCallFnMode == 1)		//Call the function with channel number as its parameter
		{
			if((ChMap[nCh].nCallInCh = SpyGetCallInCh(nCh)) == -1)	//Get the number of incoming channel
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  GetSsmLastErrMsg());
			if((ChMap[nCh].nCallOutCh = SpyGetCallOutCh(nCh)) == -1)//Get the number of outgoing channel
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  GetSsmLastErrMsg());
		
			if(ChMap[nCh].wRecDirection == CALL_IN_RECORD)
			{
				if(SsmRecToFile(ChMap[nCh].nCallInCh,szFile, -1, 0L, -1, -1, 0) == -1){
					LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmRecToFile"));
					return false;
				}
			}
			else if(ChMap[nCh].wRecDirection == CALL_OUT_RECORD)
			{
				if(SsmRecToFile(ChMap[nCh].nCallOutCh, szFile, -1, 0L, -1, -1, 0) == -1){
					LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmRecToFile"));
					return false;
				}
				
			}
			else
			{
				if(SsmLinkFrom(ChMap[nCh].nCallOutCh, ChMap[nCh].nCallInCh) == -1)  //Connect the bus from outgoing channel to incoming channel
					LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmLinkFrom"));

				if(SsmSetRecMixer(ChMap[nCh].nCallInCh, TRUE, 0) == -1)		//Turn on the record mixer
					LOG4CPLUS_ERROR(log,"Ch:" << nCh <<  _T(" Fail to call SsmSetRecMixer"));

				if(SsmRecToFile(ChMap[nCh].nCallInCh, szFile, -1, 0L, -1, -1, 0) == -1){//Recording
					LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmRecToFile"));
					return false;
				}
				
			}
		}
	}
#pragma endregion E1
#pragma region ANALOG_RECORD
	else if (ChMap[nCh].nChType == CH_TYPE_ANALOG_RECORD)
	{
		if(SsmRecToFile(nCh, szFile, -1, 0L, -1, -1, 0) == -1){ //start recording
			LOG4CPLUS_ERROR(log , "Ch:" << nCh <<" failed to call function SsmRecToFile()");
			return false;
		}
	}
#pragma endregion ANALOG_RECORD
#pragma region IPR
	else if (ChMap[nCh].nChType == CH_TYPE_IPR)
	{
		/*CString _Caller = ChMap[nCh].szCallerId.Left(ChMap[nCh].szCallerId.Find("@"));
		LOG4CPLUS_TRACE(log,"Ch:" << nCh << "Caller:" << _Caller.GetBuffer());
		if(_Caller != "sip:24117496")
		{
			LOG4CPLUS_WARN(log,"Ch:" << nCh << "Caller:" << _Caller.GetBuffer() << ", not recording" );
			return false;
		}*/

		if(SsmIPRSetMixerType(nCh,ChMap[nCh].wRecDirection)<0)
		{
			LOG4CPLUS_ERROR(log,"Ch:" << nCh << "SsmIPRSetMixerType:" << ChMap[nCh].wRecDirection << "," << GetSsmLastErrMsg());
		}

		if(SsmIPRActiveAndRecToFile(nCh, ChMap[nCh].nRecSlaverId, ChMap[nCh].dwSessionId,
			ChMap[nCh].nPrimaryCodec, &ChMap[nCh].nFowardingPPort,
			&ChMap[nCh].nFowardingSPort, 
			szFile, A_LAW, 0, -1, -1, 0) != 0)
		{
			LOG4CPLUS_ERROR(log, "Ch:" << nCh << " IPRActiveAndRecToFile  dwSessionId:" << ChMap[nCh].dwSessionId
				<< " nFowardingPPort:" << ChMap[nCh].nFowardingPPort
				<<"; nFowardingSPort:"<<  ChMap[nCh].nFowardingSPort
				<< ","<< GetSsmLastErrMsg());
			return false;
		}
		else{
			LOG4CPLUS_INFO(log,"Ch:" << nCh << " IPRActiveAndRecToFile  dwSessionId:" << ChMap[nCh].dwSessionId
				<< " nFowardingPPort:" << ChMap[nCh].nFowardingPPort
				<<"; nFowardingSPort:"<<  ChMap[nCh].nFowardingSPort);
		}
	}
#pragma endregion IPR
	ChMap[nCh].tStartTime = CTime::GetCurrentTime();
	ChMap[nCh].nRecordTimes++;
	ChMap[nCh].sql = "INSERT INTO RecordLog  ( CallerNum,CalleeNum,CustomerID,StarTime,F_Path ,Flag)";
	ChMap[nCh].sql += "VALUES ( '" + ChMap[nCh].szCallerId + "','9" + ChMap[nCh].szCalleeId + "','','" + ChMap[nCh].tStartTime.Format("%Y-%m-%d %H:%M:%S") + "','" + ChMap[nCh].szFileName + "','0') ";
	m_sqlServerDB.addSql2Queue(ChMap[nCh].sql.GetBuffer());
	LOG4CPLUS_TRACE(log, "Ch:" << nCh << " addSql2Queue:" << ChMap[nCh].sql.GetBuffer());
	m_RecordingSum++;
	return true;
}

void CRecorderDlg::SetChannelState(unsigned long nIndex, CH_STATE newState)
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("Recorder");
	ChMap[nIndex].nState = newState;
	ChMap[nIndex].szState = StateName[newState];
	LOG4CPLUS_TRACE(log,"Ch:" << nIndex << ",New State:" << ChMap[nIndex].szState);
}

void CRecorderDlg::GetCaller(unsigned long nIndex){
	static log4cplus::Logger log = log4cplus::Logger::getInstance(_T("Recorder"));
	if (ChMap[nIndex].nChType == CH_TYPE_E1_RECORD)
	{
		if(SpyGetCallerId(nIndex, ChMap[nIndex].szCallerId.GetBuffer(50)) == -1)//Get calling party number
			LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SpyGetCallerId"));
		ChMap[nIndex].szCallerId.ReleaseBuffer();
	}
	else if (ChMap[nIndex].nChType == CH_TYPE_ANALOG_RECORD)
	{
		if(SsmGetCallerId(nIndex, ChMap[nIndex].szCallerId.GetBuffer(50)) == -1)
		{
			LOG4CPLUS_ERROR(log, "Ch:" << nIndex << " failed to call function SsmGetCallerId()");
		}
	}
}
void CRecorderDlg::GetCallee(unsigned long nIndex){
	static log4cplus::Logger log = log4cplus::Logger::getInstance(_T("Recorder"));
	if (ChMap[nIndex].nChType == CH_TYPE_E1_RECORD)
	{	
		if(SpyGetCalleeId(nIndex, ChMap[nIndex].szCalleeId.GetBuffer(50)) == -1)//Get called party number
		LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SpyGetCalleeId"));
		ChMap[nIndex].szCalleeId.ReleaseBuffer();
	}

}
void CRecorderDlg::GetCallerAndCallee(unsigned long nIndex)
{
	GetCaller(nIndex);
	GetCallee(nIndex);
}

void CRecorderDlg::ClearChVariable(unsigned long nCh)
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("Recorder");
	LOG4CPLUS_DEBUG(log, "Ch:" << nCh << ",ClearChVariable");
	ChMap[nCh].szDtmf.Empty();
	ChMap[nCh].szCalleeId.Empty();
	ChMap[nCh].szCallerId.Empty();
	ChMap[nCh].szFileName.Empty();
	ChMap[nCh].wRecDirection = MIX_RECORD;
	ChMap[nCh].szCallOutDtmf.Empty();
	ChMap[nCh].szFileName.Empty();
	ChMap[nCh].sql.Empty();
	ChMap[nCh].dwSessionId = 0;
	ChMap[nCh].nPtlType = -1;
	ChMap[nCh].nStationId = -1;
	ChMap[nCh].nCallRef = -1;
	ChMap[nCh].szIPP.Empty();
	ChMap[nCh].szIPS.Empty();
	ChMap[nCh].nRecSlaverId = -1;
	ChMap[nCh].nFowardingPPort = -1;
	ChMap[nCh].nFowardingSPort = -1;
	ChMap[nCh].nPrimaryCodec = -1;

}
std::string CRecorderDlg::GetShEventName(unsigned int nEvent){
	switch(nEvent){
	case E_CHG_SpyState:					return "E_CHG_SpyState";
	case E_CHG_ChState:						return "E_CHG_ChState";
	case E_SYS_BargeIn:						return "E_SYS_BargeIn";
	case E_SYS_NoSound:						return "E_SYS_NoSound";
	case E_PROC_RecordEnd:					return "E_PROC_RecordEnd";
	case E_CHG_RingCount:					return "E_CHG_RingCount";
	case E_RCV_IPR_DChannel:				return "E_RCV_IPR_DChannel";
	case E_CHG_PcmLinkStatus:				return "E_CHG_PcmLinkStatus";
	case E_RCV_IPR_DONGLE_ADDED:			return "E_RCV_IPR_DONGLE_ADDED";
	case E_RCV_IPR_DONGLE_REMOVED:			return "E_RCV_IPR_DONGLE_REMOVED";
	case E_RCV_IPA_DONGLE_ADDED:			return "E_RCV_IPA_DONGLE_ADDED";
	case E_RCV_IPA_DONGLE_REMOVED:			return "E_RCV_IPA_DONGLE_REMOVED";
	case E_RCV_IPA_APPLICATION_PENDING:		return "E_RCV_IPA_APPLICATION_PENDING";
	case E_RCV_IPR_STATION_ADDED:			return "E_RCV_IPR_STATION_ADDED";
	case E_RCV_IPR_STATION_REMOVED:			return "E_RCV_IPR_STATION_REMOVED";
	case E_RCV_IPR_AUTH_OVERFLOW:			return "E_RCV_IPR_AUTH_OVERFLOW";
	case E_IPR_SLAVER_INIT_CB:				return "E_IPR_SLAVER_INIT_CB";
	case E_IPR_START_SLAVER_CB:				return "E_IPR_START_SLAVER_CB";
	case E_IPR_CLOSE_SLAVER_CB:				return "E_IPR_CLOSE_SLAVER_CB";
	case E_RCV_IPR_MEDIA_SESSION_STARTED:	return "E_RCV_IPR_MEDIA_SESSION_STARTED";
	case E_RCV_IPR_AUX_MEDIA_SESSION_STARTED:return "E_RCV_IPR_AUX_MEDIA_SESSION_STARTED";
	case E_RCV_IPR_MEDIA_SESSION_STOPED:	return "E_RCV_IPR_MEDIA_SESSION_STOPED";
	case E_RCV_IPR_AUX_MEDIA_SESSION_STOPED:return "E_RCV_IPR_AUX_MEDIA_SESSION_STOPED";
	case E_IPR_ACTIVE_AND_REC_CB:			return "E_IPR_ACTIVE_AND_REC_CB";
	case E_IPR_DEACTIVE_AND_STOPREC_CB:		return "E_IPR_DEACTIVE_AND_STOPREC_CB";
	case E_IPR_LINK_REC_SLAVER_CONNECTED:	return "E_IPR_LINK_REC_SLAVER_CONNECTED";
	case E_IPR_LINK_REC_SLAVER_DISCONNECTED:return "E_IPR_LINK_REC_SLAVER_DISCONNECTED";
	case E_IPR_RCV_DTMF:					return "E_IPR_RCV_DTMF";
	case E_RCV_IPR_MEDIA_SESSION_FOWARDING:	return "E_RCV_IPR_MEDIA_SESSION_FOWARDING";
	case E_RCV_IPR_MEDIA_SESSION_FOWARD_STOPED:return "E_RCV_IPR_MEDIA_SESSION_FOWARD_STOPED";
	case E_IPR_PAUSE_REC_CB:				return "E_IPR_PAUSE_REC_CB";
	case E_IPR_RESTART_REC_CB:				return "E_IPR_RESTART_REC_CB";
	default:
		{
			std::stringstream oss;
			oss << "unknown Event:0x" << std::hex << nEvent;
			return oss.str();
		}
		break;
	}
}
std::string CRecorderDlg::GetShStateName(unsigned int nState){
	switch(nState){
	case S_CALL_STANDBY: return "S_CALL_STANDBY";
	case S_CALL_PICKUPED: return "S_CALL_PICKUPED";
	case S_CALL_RINGING: return "S_CALL_RINGING";
	case S_CALL_TALKING: return "S_CALL_TALKING";

	case S_CALL_ANALOG_WAITDIALTONE: return "S_CALL_ANALOG_WAITDIALTONE";
	case S_CALL_ANALOG_TXPHONUM: return "S_CALL_ANALOG_TXPHONUM";
	case S_CALL_ANALOG_WAITDIALRESULT: return "S_CALL_ANALOG_WAITDIALRESULT";

	case S_CALL_PENDING: return "S_CALL_PENDING";
	case S_CALL_OFFLINE: return "S_CALL_OFFLINE";
	case S_CALL_WAIT_REMOTE_PICKUP: return "S_CALL_WAIT_REMOTE_PICKUP";
	case S_CALL_ANALOG_CLEAR: return "S_CALL_ANALOG_CLEAR";
	case S_CALL_UNAVAILABLE: return "S_CALL_UNAVAILABLE";
	case S_CALL_LOCKED: return "S_CALL_LOCKED";

	case S_CALL_RemoteBlock: return "S_CALL_RemoteBlock";
	case S_CALL_LocalBlock: return "S_CALL_LocalBlock";

	case S_CALL_Ss1InWaitPhoNum: return "S_CALL_Ss1InWaitPhoNum";
	case S_CALL_Ss1InWaitFwdStop: return "S_CALL_Ss1InWaitFwdStop";
	case S_CALL_Ss1InWaitCallerID: return "S_CALL_Ss1InWaitCallerID";
	case S_CALL_Ss1InWaitKD: return "S_CALL_Ss1InWaitKD";
	case S_CALL_Ss1InWaitKDStop: return "S_CALL_Ss1InWaitKDStop";
	case S_CALL_SS1_SAYIDLE: return "S_CALL_SS1_SAYIDLE";
	case S_CALL_SS1WaitIdleCAS: return "S_CALL_SS1WaitIdleCAS";
	case S_CALL_SS1PhoNumHoldup: return "S_CALL_SS1PhoNumHoldup";
	case S_CALL_Ss1InWaitStopSendA3p: return "S_CALL_Ss1InWaitStopSendA3p";

	case S_CALL_Ss1OutWaitBwdAck: return "S_CALL_Ss1OutWaitBwdAck";	
	case S_CALL_Ss1OutTxPhoNum: return "S_CALL_Ss1OutTxPhoNum";
	case S_CALL_Ss1OutWaitAppendPhoNum: return "S_CALL_Ss1OutWaitAppendPhoNum";
	case S_CALL_Ss1OutTxCallerID: return "S_CALL_Ss1OutTxCallerID";
	case S_CALL_Ss1OutWaitKB: return "S_CALL_Ss1OutWaitKB";
	case S_CALL_Ss1OutDetectA3p: return "S_CALL_Ss1OutDetectA3p";

	//case S_FAX_OK: return "S_FAX_OK";
	//case S_FAX_Wait: return "S_FAX_Wait";
	case S_FAX_ROUND: return "S_FAX_ROUND";
	case S_FAX_PhaseA: return "S_FAX_PhaseA";
	case S_FAX_PhaseB: return "S_FAX_PhaseB";
	case S_FAX_SendDCS: return "S_FAX_SendDCS";
	case S_FAX_Train: return "S_FAX_Train";
	case S_FAX_PhaseC: return "S_FAX_PhaseC";
	case S_FAX_PhaseD: return "S_FAX_PhaseD";
	case S_FAX_NextPage: return "S_FAX_NextPage";
	case S_FAX_AllSent: return "S_FAX_AllSent";
	case S_FAX_PhaseE: return "S_FAX_PhaseE";
	case S_FAX_Reset: return "S_FAX_Reset";
	case S_FAX_Init: return "S_FAX_Init";
	case S_FAX_RcvDCS: return "S_FAX_RcvDCS";
	case S_FAX_SendFTT: return "S_FAX_SendFTT";
	case S_FAX_SendCFR: return "S_FAX_SendCFR";

	case S_FAX_SendPPS: return "S_FAX_SendPPS";
	case S_FAX_RcvPPR: return "S_FAX_RcvPPR";
	case S_FAX_RepeatECMPage: return "S_FAX_RepeatECMPage";
	case S_FAX_CTC_CTR: return "S_FAX_CTC_CTR";
	case S_FAX_SendPPR: return "S_FAX_SendPPR";
	case S_FAX_EOR_ERR: return "S_FAX_EOR_ERR";
	case S_FAX_RNR_RR: return "S_FAX_RNR_RR";
	case S_FAX_RTN: return "S_FAX_RTN";
	case S_FAX_NextPage_EOM: return "S_FAX_NextPage_EOM";
	case S_FAX_V34_PhaseV8: return "S_FAX_V34_PhaseV8";

	case S_TUP_WaitPcmReset: return "S_TUP_WaitPcmReset";
	case S_TUP_WaitSAM: return "S_TUP_WaitSAM";
	case S_TUP_WaitGSM: return "S_TUP_WaitGSM";
	case S_TUP_WaitCLF: return "S_TUP_WaitCLF";
	case S_TUP_WaitPrefix: return "S_TUP_WaitPrefix";
	case S_TUP_WaitDialAnswer: return "S_TUP_WaitDialAnswer";
	case S_TUP_WaitRLG: return "S_TUP_WaitRLG";
	case S_TUP_WaitSetCallerID: return "S_TUP_WaitSetCallerID";

	case S_ISDN_OUT_WAIT_NET_RESPONSE: return "S_ISDN_OUT_WAIT_NET_RESPONSE";
	case S_ISDN_OUT_PLS_APPEND_NO: return "S_ISDN_OUT_PLS_APPEND_NO";
	case S_ISDN_IN_CHK_CALL_IN: return "S_ISDN_IN_CHK_CALL_IN";
	case S_ISDN_IN_RCVING_NO: return "S_ISDN_IN_RCVING_NO";
	case S_ISDN_IN_WAIT_TALK: return "S_ISDN_IN_WAIT_TALK";
	case S_ISDN_OUT_WAIT_ALERT: return "S_ISDN_OUT_WAIT_ALERT";
	case S_ISDN_CALL_BEGIN: return "S_ISDN_CALL_BEGIN";
	case S_ISDN_WAIT_HUANGUP: return "S_ISDN_WAIT_HUANGUP";
	case S_ISDN_IN_CALL_PROCEEDING: return "S_ISDN_IN_CALL_PROCEEDING";

	case S_CALL_SENDRING: return "S_CALL_SENDRING";

	//case S_SPY_STANDBY: return "S_SPY_STANDBY";
	case S_SPY_RCVPHONUM: return "S_SPY_RCVPHONUM";
	//case S_SPY_RINGING: return "S_SPY_RINGING";
	//case S_SPY_TALKING: return "S_SPY_TALKING";

	case S_SPY_SS1RESET: return "S_SPY_SS1RESET";
	case S_SPY_SS1WAITBWDACK: return "S_SPY_SS1WAITBWDACK";
	case S_SPY_SS1WAITKB: return "S_SPY_SS1WAITKB";

	case S_ISUP_WaitSAM: return "S_ISUP_WaitSAM";
	case S_ISUP_WaitRLC: return "S_ISUP_WaitRLC";
	case S_ISUP_WaitReset: return "S_ISUP_WaitReset";
	case S_ISUP_LocallyBlocked: return "S_ISUP_LocallyBlocked";
	case S_ISUP_RemotelyBlocked: return "S_ISUP_RemotelyBlocked";
	case S_ISUP_WaitDialAnswer: return "S_ISUP_WaitDialAnswer";
	case S_ISUP_WaitINF: return "S_ISUP_WaitINF";
	case S_ISUP_WaitSetCallerID: return "S_ISUP_WaitSetCallerID";

	case S_DTRC_ACTIVE: return "S_DTRC_ACTIVE";
	case S_ISUP_Suspend: return "S_ISUP_Suspend";

	case S_CALL_EM_TXPHONUM: return "S_CALL_EM_TXPHONUM";
	case S_CALL_EM_WaitIdleCAS: return "S_CALL_EM_WaitIdleCAS";
	case S_CALL_VOIP_DIALING: return "S_CALL_VOIP_DIALING";
	case S_CALL_VOIP_WAIT_CONNECTED: return "S_CALL_VOIP_WAIT_CONNECTED";
	case S_CALL_VOIP_CHANNEL_UNUSABLE: return "S_CALL_VOIP_CHANNEL_UNUSABLE";

	case S_CALL_DISCONECT: return "S_CALL_DISCONECT";

	case S_CALL_SS1WaitFlashEnd: return "S_CALL_SS1WaitFlashEnd";
	case S_CALL_FlashEnd: return "S_CALL_FlashEnd";
	case S_CALL_SIGNAL_ERROR: return "S_CALL_SIGNAL_ERROR";
	case S_CALL_FRAME_ERROR: return "S_CALL_FRAME_ERROR";

	//150-159, reserved for VoIP board
	case S_CALL_VOIP_SESSION_PROCEEDING: return "S_CALL_VOIP_SESSION_PROCEEDING";
	case S_CALL_VOIP_REG_ING: return "S_CALL_VOIP_REG_ING";
	case S_CALL_VOIP_REG_FAILED: return "S_CALL_VOIP_REG_FAILED";
	case S_CALL_VOIP_CALL_ON_HOLD: return "S_CALL_VOIP_CALL_ON_HOLD";

	//160-169, resoured for VoIP resource board
	case S_IP_MEDIA_LOCK: return "S_IP_MEDIA_LOCK";
	case S_IP_MEDIA_OPEN: return "S_IP_MEDIA_OPEN";
	case S_SPY_RBSWAITACK: return "S_SPY_RBSWAITACK";
	case S_SPY_RBSSENDACK: return "S_SPY_RBSSENDACK";

	case S_IPR_USING: return "S_IPR_USING";
	case S_IPR_COMMUNICATING: return "S_IPR_COMMUNICATING";
	default:
		{
			std::stringstream oss;
			oss << "unknown State:0x" << std::hex << nState;
			return oss.str();
		}
		break;
	}
}

std::string CRecorderDlg::GetDSTStateName(unsigned int nState)
{
	switch(nState)
	{
		case DE_OFFHOOK:					return "DE_OFFHOOK";//0x8
		case DE_ONHOOK:						return "DE_ONHOOK";//0xe
		case DE_LT_ON:						return "DE_LT_ON";//0x1001
		case DE_LT_OFF:						return "DE_LT_OFF";//0x1002
		case DE_LT_FLASHING:				return "DE_LT_FLASHING";	//0x1003
		case DE_DGT_PRS:					return "DE_DGT_PRS";//0x1006
		case DE_DGT_RLS:					return "DE_DGT_RLS";//0x1007
		case DE_MSG_CHG:					return "DE_MSG_CHG";//0x1008
		case DE_STARTSTOP_ON:				return "DE_STARTSTOP_ON";//0x1009
		case DE_STARTSTOP_OFF:				return "DE_STARTSTOP_OFF";//0x100a
		case DE_LT_FASTFLASHING:			return "DE_LT_FASTFLASHING";//0x100b
		case DE_DOWNLOAD_STATUS:			return "DE_DOWNLOAD_STATUS";//0x100c
		case DE_FINISHED_PLAY:				return "DE_FINISHED_PLAY";//0x100d
		case DE_FUNC_BTN_PRS:				return "DE_FUNC_BTN_PRS";//0x100e
		case DE_FUNC_BTN_RLS:				return "DE_FUNC_BTN_RLS";//0x100f
		case DE_HOLD_BTN_PRS:				return "DE_HOLD_BTN_PRS";//0x1010
		case DE_HOLD_BTN_RLS:				return "DE_HOLD_BTN_RLS";//0x1011
		case DE_RELEASE_BTN_PRS:			return "DE_RELEASE_BTN_PRS";//0x1012
		case DE_RELEASE_BTN_RLS:			return "DE_RELEASE_BTN_RLS";//0x1013
		case DE_TRANSFER_BTN_PRS:			return "DE_TRANSFER_BTN_PRS";//0x1014
		case DE_ANSWER_BTN_PRS:				return "DE_ANSWER_BTN_PRS";//0x1015
		case DE_SPEAKER_BTN_PRS:			return "DE_SPEAKER_BTN_PRS";//0x1016
		case DE_REDIAL_BTN_PRS:				return "DE_REDIAL_BTN_PRS";//0x1017
		case DE_CONF_BTN_PRS:				return "DE_CONF_BTN_PRS";//0x1018
		case DE_RECALL_BTN_PRS:				return "DE_RECALL_BTN_PRS";//0x1019
		case DE_FEATURE_BTN_PRS:			return "DE_FEATURE_BTN_PRS";//0x101a
		case DE_UP_DOWN:					return "DE_UP_DOWN";//0x101b
		case DE_EXIT_BTN_PRS:				return "DE_EXIT_BTN_PRS";//0x101c
		case DE_HELP_BTN_PRS:				return "DE_HELP_BTN_PRS";//0x101d
		case DE_SOFT_BTN_PRS:				return "DE_SOFT_BTN_PRS";//0x101e
		case DE_RING_ON:					return "DE_RING_ON";//0x101f
		case DE_RING_OFF:					return "DE_RING_OFF";//0x1020
		case DE_LINE_BTN_PRS:				return "DE_LINE_BTN_PRS";//0x1021
		case DE_MENU_BTN_PRS:				return "DE_MENU_BTN_PRS";//0x1022
		case DE_PREVIOUS_BTN_PRS:			return "DE_PREVIOUS_BTN_PRS";//0x1023
		case DE_NEXT_BTN_PRS:				return "DE_NEXT_BTN_PRS";//0x1024
		case DE_LT_QUICKFLASH:				return "DE_LT_QUICKFLASH";//0x1025
		case DE_AUDIO_ON:					return "DE_AUDIO_ON";//0x1026
		case DE_AUDIO_OFF:					return "DE_AUDIO_OFF";//0x1027
		case DE_DISPLAY_CLOCK:				return "DE_DISPLAY_CLOCK";//0x1028
		case DE_DISPLAY_TIMER:				return "DE_DISPLAY_TIMER";//0x1029
		case DE_DISPLAY_CLEAR:				return "DE_DISPLAY_CLEAR";//0x102a
		case DE_CFWD:						return "DE_CFWD";//0x102b
		case DE_CFWD_CANCELED:				return "DE_CFWD_CANCELED";//0x102c
		case DE_AUTO_ANSWER:				return "DE_AUTO_ANSWER";//0x102d
		case DE_AUTO_ANSWER_CANCELED:		return "DE_AUTO_ANSWER_CANCELED";//0x102e
		case DE_SET_BUSY:					return "DE_SET_BUSY";//0x102f
		case DE_SET_BUSY_CANCELED:			return "DE_SET_BUSY_CANCELED";//0x1030
		case DE_DESTINATION_BUSY:			return "DE_DESTINATION_BUSY";//0x1031
		case DE_REORDER:					return "DE_REORDER";//0x1032
		case DE_LT_VERY_FASTFLASHING:		return "DE_LT_VERY_FASTFLASHING";//0x1033
		case DE_SPEAKER_BTN_RLS:			return "DE_SPEAKER_BTN_RLS";//0x1034
		case DE_REDIAL_BTN_RLS:				return "DE_REDIAL_BTN_RLS";//0x1035
		case DE_TRANSFER_BTN_RLS:			return "DE_TRANSFER_BTN_RLS";//0x1036
		case DE_CONF_BTN_RLS:				return "DE_CONF_BTN_RLS";//0x1037
		case DE_DISCONNECTED:				return "DE_DISCONNECTED";//0x1038
		case DE_CONNECTED:					return "DE_CONNECTED";//0x1039
		case DE_ABANDONED:					return "DE_ABANDONED";//0x103a
		case DE_SUSPENDED:					return "DE_SUSPENDED";//0x103b
		case DE_RESUMED:					return "DE_RESUMED";//0x103c
		case DE_HELD:						return "DE_HELD";//0x103d
		case DE_RETRIEVED:					return "DE_RETRIEVED";//0x103e
		case DE_REJECTED:					return "DE_REJECTED";//0x103f
		case DE_MSG_BTN_PRS:				return "DE_MSG_BTN_PRS";//0x1040
		case DE_MSG_BTN_RLS:				return "DE_MSG_BTN_RLS";//0x1041
		case DE_SUPERVISOR_BTN_PRS:			return "DE_SUPERVISOR_BTN_PRS";//0x1042
		case DE_SUPERVISOR_BTN_RLS:			return "DE_SUPERVISOR_BTN_RLS";//0x1043
		case DE_WRAPUP_BTN_PRS:				return "DE_WRAPUP_BTN_PRS";//0x1044
		case DE_WRAPUP_BTN_RLS:				return "DE_WRAPUP_BTN_RLS";//0x1045
		case DE_READY_BTN_PRS:				return "DE_READY_BTN_PRS";//0x1046
		case DE_READY_BTN_RLS:				return "DE_READY_BTN_RLS";//0x1047
		case DE_LOGON_BTN_PRS:				return "DE_LOGON_BTN_PRS";//0x1048
		case DE_BREAK_BTN_PRS:				return "DE_BREAK_BTN_PRS";//0x1049
		case DE_AUDIO_CHG:					return "DE_AUDIO_CHG";//0x104a
		case DE_DISPLAY_MSG:				return "DE_DISPLAY_MSG";//0x104b
		case DE_WORK_BTN_PRS:				return "DE_WORK_BTN_PRS";//0x104c
		case DE_TALLY_BTN_PRS:				return "DE_TALLY_BTN_PRS";//0x104d
		case DE_PROGRAM_BTN_PRS:			return "DE_PROGRAM_BTN_PRS";//0x104e
		case DE_MUTE_BTN_PRS:				return "DE_MUTE_BTN_PRS";//0x104f
		case DE_ALERTING_AUTO_ANSWER:		return "DE_ALERTING_AUTO_ANSWER";//0x1050
		case DE_MENU_BTN_RLS:				return "DE_MENU_BTN_RLS";//0x1051
		case DE_EXIT_BTN_RLS:				return "DE_EXIT_BTN_RLS";//0x1052
		case DE_NEXT_BTN_RLS:				return "DE_NEXT_BTN_RLS";//0x1053
		case DE_PREVIOUS_BTN_RLS:			return "DE_PREVIOUS_BTN_RLS";//0x1054
		case DE_SHIFT_BTN_PRS:				return "DE_SHIFT_BTN_PRS";//0x1055
		case DE_SHIFT_BTN_RLS:				return "DE_SHIFT_BTN_RLS";//0x1056
		case DE_PAGE_BTN_PRS:				return "DE_PAGE_BTN_PRS";//0x1057
		case DE_PAGE_BTN_RLS:				return "DE_PAGE_BTN_RLS";//0x1058
		case DE_SOFT_BTN_RLS:				return "DE_SOFT_BTN_RLS";//0x1059
		case DE_LINE_LT_OFF:				return "DE_LINE_LT_OFF";//0x1060
		case DE_LINE_LT_ON:					return "DE_LINE_LT_ON";//0x1061
		case DE_LINE_LT_FLASHING:			return "DE_LINE_LT_FLASHING";//0x1062
		case DE_LINE_LT_FASTFLASHING:		return "DE_LINE_LT_FASTFLASHING";//0x1063
		case DE_LINE_LT_VERY_FASTFLASHING:	return "DE_LINE_LT_VERY_FASTFLASHING";//0x1064
		case DE_LINE_LT_QUICKFLASH:			return "DE_LINE_LT_QUICKFLASH";//0x1065
		case DE_LINE_LT_WINK:				return "DE_LINE_LT_WINK";//0x1066
		case DE_LINE_LT_SLOW_WINK:			return "DE_LINE_LT_SLOW_WINK";//0x1067
		case DE_FEATURE_LT_OFF:				return "DE_FEATURE_LT_OFF";//0x1068
		case DE_FEATURE_LT_ON:				return "DE_FEATURE_LT_ON";//0x1069
		case DE_FEATURE_LT_FLASHING:		return "DE_FEATURE_LT_FLASHING";//0x106A
		case DE_FEATURE_LT_FASTFLASHING:	return "DE_FEATURE_LT_FASTFLASHING";//0x106B
		case DE_FEATURE_LT_VERY_FASTFLASHING:return "DE_FEATURE_LT_VERY_FASTFLASHING";//0x106C
		case DE_FEATURE_LT_QUICKFLASH:		return "DE_FEATURE_LT_QUICKFLASH";//0x106D
		case DE_FEATURE_LT_WINK:			return "DE_FEATURE_LT_WINK";//0x106E
		case DE_FEATURE_LT_SLOW_WINK:		return "DE_FEATURE_LT_SLOW_WINK";//0x106F
		case DE_SPEAKER_LT_OFF:				return "DE_SPEAKER_LT_OFF";//0x1070
		case DE_SPEAKER_LT_ON:				return "DE_SPEAKER_LT_ON";//0x1071
		case DE_SPEAKER_LT_FLASHING:		return "DE_SPEAKER_LT_FLASHING";//0x1072
		case DE_SPEAKER_LT_FASTFLASHING:	return "DE_SPEAKER_LT_FASTFLASHING";//0x1073
		case DE_SPEAKER_LT_VERY_FASTFLASHING:return "DE_SPEAKER_LT_VERY_FASTFLASHING";//0x1074
		case DE_SPEAKER_LT_QUICKFLASH:		return "DE_SPEAKER_LT_QUICKFLASH";//0x1075
		case DE_SPEAKER_LT_WINK:			return "DE_SPEAKER_LT_WINK";//0x1076
		case DE_SPEAKER_LT_SLOW_WINK:		return "DE_SPEAKER_LT_SLOW_WINK";//0x1077
		case DE_MIC_LT_OFF:					return "DE_MIC_LT_OFF";//0x1078
		case DE_MIC_LT_ON:					return "DE_MIC_LT_ON";//0x1079
		case DE_MIC_LT_FLASHING:			return "DE_MIC_LT_FLASHING";//0x107A
		case DE_MIC_LT_FASTFLASHING:		return "DE_MIC_LT_FASTFLASHING";//0x107B
		case DE_MIC_LT_VERY_FASTFLASHING:	return "DE_MIC_LT_VERY_FASTFLASHING";//0x107C
		case DE_MIC_LT_QUICKFLASH:			return "DE_MIC_LT_QUICKFLASH";//0x107D
		case DE_MIC_LT_WINK:				return "DE_MIC_LT_WINK";//0x107E
		case DE_MIC_LT_SLOW_WINK:			return "DE_MIC_LT_SLOW_WINK";//0x107F
		case DE_HOLD_LT_OFF:				return "DE_HOLD_LT_OFF";//0x1080
		case DE_HOLD_LT_ON:					return "DE_HOLD_LT_ON";//0x1081
		case DE_HOLD_LT_FLASHING:			return "DE_HOLD_LT_FLASHING";//0x1082
		case DE_HOLD_LT_FASTFLASHING:		return "DE_HOLD_LT_FASTFLASHING";//0x1083
		case DE_HOLD_LT_VERY_FASTFLASHING:	return "DE_HOLD_LT_VERY_FASTFLASHING";//0x1084
		case DE_HOLD_LT_QUICKFLASH:			return "DE_HOLD_LT_QUICKFLASH";//0x1085
		case DE_HOLD_LT_WINK:				return "DE_HOLD_LT_WINK";//0x1086
		case DE_HOLD_LT_SLOW_WINK:			return "DE_HOLD_LT_SLOW_WINK";//0x1087
		case DE_RELEASE_LT_OFF:				return "DE_RELEASE_LT_OFF";//0x1088
		case DE_RELEASE_LT_ON:				return "DE_RELEASE_LT_ON";//0x1089
		case DE_RELEASE_LT_FLASHING:		return "DE_RELEASE_LT_FLASHING";//0x108A
		case DE_RELEASE_LT_FASTFLASHING:	return "DE_RELEASE_LT_FASTFLASHING";//0x108B
		case DE_RELEASE_LT_VERY_FASTFLASHING:return "DE_RELEASE_LT_VERY_FASTFLASHING";//0x108C
		case DE_RELEASE_LT_QUICKFLASH:		return "DE_RELEASE_LT_QUICKFLASH";//0x108D
		case DE_RELEASE_LT_WINK:			return "DE_RELEASE_LT_WINK";//0x108E
		case DE_RELEASE_LT_SLOW_WINK:		return "DE_RELEASE_LT_SLOW_WINK";//0x108F
		case DE_HELP_LT_OFF:				return "DE_HELP_LT_OFF";//0x1090
		case DE_HELP_LT_ON:					return "DE_HELP_LT_ON";//0x1091
		case DE_HELP_LT_FLASHING:			return "DE_HELP_LT_FLASHING";//0x1092
		case DE_HELP_LT_FASTFLASHING:		return "DE_HELP_LT_FASTFLASHING";//0x1093
		case DE_HELP_LT_VERY_FASTFLASHING:	return "DE_HELP_LT_VERY_FASTFLASHING";//0x1094
		case DE_HELP_LT_QUICKFLASH:			return "DE_HELP_LT_QUICKFLASH";//0x1095
		case DE_HELP_LT_WINK:				return "DE_HELP_LT_WINK";//0x1096
		case DE_HELP_LT_SLOW_WINK:			return "DE_HELP_LT_SLOW_WINK";//0x1097
		case DE_SUPERVISOR_LT_OFF:			return "DE_SUPERVISOR_LT_OFF";//0x1098
		case DE_SUPERVISOR_LT_ON:			return "DE_SUPERVISOR_LT_ON";//0x1099
		case DE_SUPERVISOR_LT_FLASHING:		return "DE_SUPERVISOR_LT_FLASHING";//0x109A
		case DE_SUPERVISOR_LT_FASTFLASHING:	return "DE_SUPERVISOR_LT_FASTFLASHING";//0x109B
		case DE_SUPERVISOR_LT_VERY_FASTFLASHING:return "DE_SUPERVISOR_LT_VERY_FASTFLASHING";//0x109C
		case DE_SUPERVISOR_LT_QUICKFLASH:	return "DE_SUPERVISOR_LT_QUICKFLASH";//0x109D
		case DE_SUPERVISOR_LT_WINK:			return "DE_SUPERVISOR_LT_WINK";//0x109E
		case DE_SUPERVISOR_LT_SLOW_WINK:	return "DE_SUPERVISOR_LT_SLOW_WINK";//0x109F
		case DE_READY_LT_OFF:				return "DE_READY_LT_OFF";//0x10A0
		case DE_READY_LT_ON:				return "DE_READY_LT_ON";//0x10A1
		case DE_READY_LT_FLASHING:			return "DE_READY_LT_FLASHING";//0x10A2
		case DE_READY_LT_FASTFLASHING:		return "DE_READY_LT_FASTFLASHING";//0x10A3
		case DE_READY_LT_VERY_FASTFLASHING:	return "DE_READY_LT_VERY_FASTFLASHING";//0x10A4
		case DE_READY_LT_QUICKFLASH:		return "DE_READY_LT_QUICKFLASH";//0x10A5
		case DE_READY_LT_WINK:				return "DE_READY_LT_WINK";//0x10A6
		case DE_READY_LT_SLOW_WINK:			return "DE_READY_LT_SLOW_WINK";//0x10A7
		case DE_LOGON_LT_OFF:				return "DE_LOGON_LT_OFF";//0x10A8
		case DE_LOGON_LT_ON:				return "DE_LOGON_LT_ON";//0x10A9
		case DE_LOGON_LT_FLASHING:			return "DE_LOGON_LT_FLASHING";//0x10AA
		case DE_LOGON_LT_FASTFLASHING:		return "DE_LOGON_LT_FASTFLASHING";//0x10AB
		case DE_LOGON_LT_VERY_FASTFLASHING:	return "DE_LOGON_LT_VERY_FASTFLASHING";//0x10AC
		case DE_LOGON_LT_QUICKFLASH:		return "DE_LOGON_LT_QUICKFLASH";//0x10AD
		case DE_LOGON_LT_WINK:				return "DE_LOGON_LT_WINK";//0x10AE
		case DE_LOGON_LT_SLOW_WINK:			return "DE_LOGON_LT_SLOW_WINK";//0x10AF
		case DE_WRAPUP_LT_OFF:				return "DE_WRAPUP_LT_OFF";//0x10B0
		case DE_WRAPUP_LT_ON:				return "DE_WRAPUP_LT_ON";//0x10B1
		case DE_WRAPUP_LT_FLASHING:			return "DE_WRAPUP_LT_FLASHING";//0x10B2
		case DE_WRAPUP_LT_FASTFLASHING:		return "DE_WRAPUP_LT_FASTFLASHING";//0x10B3
		case DE_WRAPUP_LT_VERY_FASTFLASHING:return "DE_WRAPUP_LT_VERY_FASTFLASHING";//0x10B4
		case DE_WRAPUP_LT_QUICKFLASH:		return "DE_WRAPUP_LT_QUICKFLASH";//0x10B5
		case DE_WRAPUP_LT_WINK:				return "DE_WRAPUP_LT_WINK";//0x10B6
		case DE_WRAPUP_LT_SLOW_WINK:		return "DE_WRAPUP_LT_SLOW_WINK";//0x10B7
		case DE_RING_LT_OFF:				return "DE_RING_LT_OFF";//0x10B8
		case DE_RING_LT_ON:					return "DE_RING_LT_ON";//0x10B9
		case DE_RING_LT_FLASHING:			return "DE_RING_LT_FLASHING";//0x10BA
		case DE_RING_LT_FASTFLASHING:		return "DE_RING_LT_FASTFLASHING";//0x10BB
		case DE_RING_LT_VERY_FASTFLASHING:	return "DE_RING_LT_VERY_FASTFLASHING";//0x10BC
		case DE_RING_LT_QUICKFLASH:			return "DE_RING_LT_QUICKFLASH";//0x10BD
		case DE_RING_LT_WINK:				return "DE_RING_LT_WINK";//0x10BE
		case DE_RING_LT_SLOW_WINK:			return "DE_RING_LT_SLOW_WINK";//0x10BF
		case DE_ANSWER_LT_OFF:				return "DE_ANSWER_LT_OFF";//0x10C0
		case DE_ANSWER_LT_ON:				return "DE_ANSWER_LT_ON";//0x10C1
		case DE_ANSWER_LT_FLASHING:			return "DE_ANSWER_LT_FLASHING";//0x10C2
		case DE_ANSWER_LT_FASTFLASHING:		return "DE_ANSWER_LT_FASTFLASHING";//0x10C3
		case DE_ANSWER_LT_VERY_FASTFLASHING:return "DE_ANSWER_LT_VERY_FASTFLASHING";//0x10C4
		case DE_ANSWER_LT_QUICKFLASH:		return "DE_ANSWER_LT_QUICKFLASH";//0x10C5
		case DE_ANSWER_LT_WINK:				return "DE_ANSWER_LT_WINK";//0x10C6
		case DE_ANSWER_LT_SLOW_WINK:		return "DE_ANSWER_LT_SLOW_WINK";//0x10C7
		case DE_PROGRAM_LT_OFF:				return "DE_PROGRAM_LT_OFF";//0x10C8
		case DE_PROGRAM_LT_ON:				return "DE_PROGRAM_LT_ON";//0x10C9
		case DE_PROGRAM_LT_FLASHING:		return "DE_PROGRAM_LT_FLASHING";//0x10CA
		case DE_PROGRAM_LT_FASTFLASHING:	return "DE_PROGRAM_LT_FASTFLASHING";//0x10CB
		case DE_PROGRAM_LT_VERY_FASTFLASHING:return "DE_PROGRAM_LT_VERY_FASTFLASHING";//0x10CC
		case DE_PROGRAM_LT_QUICKFLASH:		return "DE_PROGRAM_LT_QUICKFLASH";//0x10CD
		case DE_PROGRAM_LT_WINK:			return "DE_PROGRAM_LT_WINK";//0x10CE
		case DE_PROGRAM_LT_MEDIUM_WINK:		return "DE_PROGRAM_LT_MEDIUM_WINK";//0x10CF
		case DE_MSG_LT_OFF:					return "DE_MSG_LT_OFF";//0x10D0
		case DE_MSG_LT_ON:					return "DE_MSG_LT_ON";//0x10D1
		case DE_MSG_LT_FLASHING:			return "DE_MSG_LT_FLASHING";//0x10D2
		case DE_MSG_LT_FASTFLASHING:		return "DE_MSG_LT_FASTFLASHING";//0x10D3
		case DE_MSG_LT_VERY_FASTFLASHING:	return "DE_MSG_LT_VERY_FASTFLASHING";//0x10D4
		case DE_MSG_LT_QUICKFLASH:			return "DE_MSG_LT_QUICKFLASH";//0x10D5
		case DE_MSG_LT_WINK:				return "DE_MSG_LT_WINK";//0x10D6
		case DE_MSG_LT_SLOW_WINK:			return "DE_MSG_LT_SLOW_WINK";//0x10D7
		case DE_TRANSFER_LT_OFF:			return "DE_TRANSFER_LT_OFF";//0x10D8
		case DE_TRANSFER_LT_ON:				return "DE_TRANSFER_LT_ON";//0x10D9
		case DE_TRANSFER_LT_FLASHING:		return "DE_TRANSFER_LT_FLASHING";//0x10DA
		case DE_TRANSFER_LT_FASTFLASHING:	return "DE_TRANSFER_LT_FASTFLASHING";//0x10DB
		case DE_TRANSFER_LT_VERY_FASTFLASHING:return "DE_TRANSFER_LT_VERY_FASTFLASHING";//0x10DC
		case DE_TRANSFER_LT_QUICKFLASH:		return "DE_TRANSFER_LT_QUICKFLASH";//0x10DD
		case DE_TRANSFER_LT_WINK:			return "DE_TRANSFER_LT_WINK";//0x10DE
		case DE_TRANSFER_LT_MEDIUM_WINK:	return "DE_TRANSFER_LT_MEDIUM_WINK";//0x10DF
		case DE_CONFERENCE_LT_OFF:			return "DE_CONFERENCE_LT_OFF";//0x10E0
		case DE_CONFERENCE_LT_ON:			return "DE_CONFERENCE_LT_ON";//0x10E1
		case DE_CONFERENCE_LT_FLASHING:		return "DE_CONFERENCE_LT_FLASHING";//0x10E2
		case DE_CONFERENCE_LT_FASTFLASHING:	return "DE_CONFERENCE_LT_FASTFLASHING";//0x10E3
		case DE_CONFERENCE_LT_VERY_FASTFLASHING:return "DE_CONFERENCE_LT_VERY_FASTFLASHING";//0x10E4
		case DE_CONFERENCE_LT_QUICKFLASH:	return "DE_CONFERENCE_LT_QUICKFLASH";//0x10E5
		case DE_CONFERENCE_LT_WINK:			return "DE_CONFERENCE_LT_WINK";//0x10E6
		case DE_CONFERENCE_LT_MEDIUM_WINK:	return "DE_CONFERENCE_LT_MEDIUM_WINK";//0x10E7
		case DE_SOFT_LT_OFF:				return "DE_SOFT_LT_OFF";//0x10E8
		case DE_SOFT_LT_ON:					return "DE_SOFT_LT_ON";//0x10E9
		case DE_SOFT_LT_FLASHING:			return "DE_SOFT_LT_FLASHING";//0x10EA
		case DE_SOFT_LT_FASTFLASHING:		return "DE_SOFT_LT_FASTFLASHING";//0x10EB
		case DE_SOFT_LT_VERY_FASTFLASHING:	return "DE_SOFT_LT_VERY_FASTFLASHING";//0x10EC
		case DE_SOFT_LT_QUICKFLASH:			return "DE_SOFT_LT_QUICKFLASH";//0x10ED
		case DE_SOFT_LT_WINK:				return "DE_SOFT_LT_WINK";//0x10EE
		case DE_SOFT_LT_SLOW_WINK:			return "DE_SOFT_LT_SLOW_WINK";//0x10EF
		case DE_MENU_LT_OFF:				return "DE_MENU_LT_OFF";//0x10F0
		case DE_MENU_LT_ON:					return "DE_MENU_LT_ON";//0x10F1
		case DE_MENU_LT_FLASHING:			return "DE_MENU_LT_FLASHING";//0x10F2
		case DE_MENU_LT_FASTFLASHING:		return "DE_MENU_LT_FASTFLASHING";//0x10F3
		case DE_MENU_LT_VERY_FASTFLASHING:	return "DE_MENU_LT_VERY_FASTFLASHING";//0x10F4
		case DE_MENU_LT_QUICKFLASH:			return "DE_MENU_LT_QUICKFLASH";//0x10F5
		case DE_MENU_LT_WINK:				return "DE_MENU_LT_WINK";//0x10F6
		case DE_MENU_LT_SLOW_WINK:			return "DE_MENU_LT_SLOW_WINK";//0x10F7
		case DE_CALLWAITING_LT_OFF:			return "DE_CALLWAITING_LT_OFF";//0x10F8
		case DE_CALLWAITING_LT_ON:			return "DE_CALLWAITING_LT_ON";//0x10F9
		case DE_CALLWAITING_LT_FLASHING:	return "DE_CALLWAITING_LT_FLASHING";//0x10FA
		case DE_CALLWAITING_LT_FASTFLASHING:return "DE_CALLWAITING_LT_FASTFLASHING";//0x10FB
		case DE_CALLWAITING_LT_VERY_FASTFLASHING:return "DE_CALLWAITING_LT_VERY_FASTFLASHING";//0x10FC
		case DE_CALLWAITING_LT_QUICKFLASH:	return "DE_CALLWAITING_LT_QUICKFLASH";//0x10FD
		case DE_CALLWAITING_LT_WINK:		return "DE_CALLWAITING_LT_WINK";//0x10FE
		case DE_CALLWAITING_LT_SLOW_WINK:	return "DE_CALLWAITING_LT_SLOW_WINK";//0x10FF
		case DE_REDIAL_LT_OFF:				return "DE_REDIAL_LT_OFF";//0x1100
		case DE_REDIAL_LT_ON:				return "DE_REDIAL_LT_ON";//0x1101
		case DE_REDIAL_LT_FLASHING:			return "DE_REDIAL_LT_FLASHING";//0x1102
		case DE_REDIAL_LT_FASTFLASHING:		return "DE_REDIAL_LT_FASTFLASHING";//0x1103
		case DE_REDIAL_LT_VERY_FASTFLASHING:return "DE_REDIAL_LT_VERY_FASTFLASHING";//0x1104
		case DE_REDIAL_LT_QUICKFLASH:		return "DE_REDIAL_LT_QUICKFLASH";//0x1105
		case DE_REDIAL_LT_WINK:				return "DE_REDIAL_LT_WINK";//0x1106
		case DE_REDIAL_LT_SLOW_WINK:		return "DE_REDIAL_LT_SLOW_WINK";//0x1107
		case DE_PAGE_LT_OFF:				return "DE_PAGE_LT_OFF";//0x1108
		case DE_PAGE_LT_ON:					return "DE_PAGE_LT_ON";//0x1109
		case DE_PAGE_LT_FLASHING:			return "DE_PAGE_LT_FLASHING";//0x110A
		case DE_PAGE_LT_FASTFLASHING:		return "DE_PAGE_LT_FASTFLASHING";//0x110B
		case DE_PAGE_LT_VERY_FASTFLASHING:	return "DE_PAGE_LT_VERY_FASTFLASHING";//0x110C
		case DE_PAGE_LT_QUICKFLASH:			return "DE_PAGE_LT_QUICKFLASH";//0x110D
		case DE_CTRL_BTN_PRS:				return "DE_CTRL_BTN_PRS";//0x110E
		case DE_CTRL_BTN_RLS:				return "DE_CTRL_BTN_RLS";//0x110F
		case DE_CANCEL_BTN_PRS:				return "DE_CANCEL_BTN_PRS";//0x1110
		case DE_CANCEL_BTN_RLS:				return "DE_CANCEL_BTN_RLS";//0x1111
		case DE_MIC_BTN_PRS:				return "DE_MIC_BTN_PRS";//0x1112
		case DE_MIC_BTN_RLS:				return "DE_MIC_BTN_RLS";//0x1113
		case DE_FLASH_BTN_PRS:				return "DE_FLASH_BTN_PRS";//0x1114
		case DE_FLASH_BTN_RLS:				return "DE_FLASH_BTN_RLS";//0x1115
		case DE_DIRECTORY_BTN_PRS:			return "DE_DIRECTORY_BTN_PRS";//0x1116
		case DE_DIRECTORY_BTN_RLS:			return "DE_DIRECTORY_BTN_RLS";//0x1117
		case DE_HANDSFREE_BTN_PRS:			return "DE_HANDSFREE_BTN_PRS";//0x1118
		case DE_HANDSFREE_BTN_RLS:			return "DE_HANDSFREE_BTN_RLS";//0x1119
		case DE_RINGTONE_BTN_PRS:			return "DE_RINGTONE_BTN_PRS";//0x111A
		case DE_RINGTONE_BTN_RLS:			return "DE_RINGTONE_BTN_RLS";//0x111B
		case DE_SAVE_BTN_PRS:				return "DE_SAVE_BTN_PRS";//0x111C
		case DE_SAVE_BTN_RLS:				return "DE_SAVE_BTN_RLS";//0x111D
		case DE_MUTE_LT_OFF:				return "DE_MUTE_LT_OFF";//0x111E
		case DE_MUTE_LT_ON:					return "DE_MUTE_LT_ON";//0x111F
		case DE_MUTE_LT_FLASHING:			return "DE_MUTE_LT_FLASHING";//0x1120
		case DE_MUTE_LT_FASTFLASHING:		return "DE_MUTE_LT_FASTFLASHING";//0x1121
		case DE_MUTE_LT_VERY_FASTFLASHING:	return "DE_MUTE_LT_VERY_FASTFLASHING";//0x1122
		case DE_MUTE_LT_QUICKFLASH:			return "DE_MUTE_LT_QUICKFLASH";//0x1123
		case DE_MUTE_LT_WINK:				return "DE_MUTE_LT_WINK";//0x1124
		case DE_MUTE_LT_SLOW_WINK:			return "DE_MUTE_LT_SLOW_WINK";//0x1125
		case DE_MUTE_LT_MEDIUM_WINK:		return "DE_MUTE_LT_MEDIUM_WINK";//0x1126
		case DE_HANDSFREE_LT_OFF:			return "DE_HANDSFREE_LT_OFF";//0x1127
		case DE_HANDSFREE_LT_ON:			return "DE_HANDSFREE_LT_ON";//0x1128
		case DE_HANDSFREE_LT_FLASHING:		return "DE_HANDSFREE_LT_FLASHING";//0x1129
		case DE_HANDSFREE_LT_FASTFLASHING:	return "DE_HANDSFREE_LT_FASTFLASHING";//0x112A
		case DE_HANDSFREE_LT_VERY_FASTFLASHING:return "DE_HANDSFREE_LT_VERY_FASTFLASHING";//0x112B
		case DE_HANDSFREE_LT_QUICKFLASH:	return "DE_HANDSFREE_LT_QUICKFLASH";//0x112C
		case DE_HANDSFREE_LT_WINK:			return "DE_HANDSFREE_LT_WINK";//0x112D
		case DE_HANDSFREE_LT_SLOW_WINK:		return "DE_HANDSFREE_LT_SLOW_WINK";//0x112E
		case DE_HANDSFREE_LT_MEDIUM_WINK:	return "DE_HANDSFREE_LT_MEDIUM_WINK";//0x112F
		case DE_DIRECTORY_LT_OFF:			return "DE_DIRECTORY_LT_OFF";//0x1130
		case DE_DIRECTORY_LT_ON:			return "DE_DIRECTORY_LT_ON";//0x1131
		case DE_DIRECTORY_LT_FLASHING:		return "DE_DIRECTORY_LT_FLASHING";//0x1132
		case DE_DIRECTORY_LT_FASTFLASHING:	return "DE_DIRECTORY_LT_FASTFLASHING";//0x1133
		case DE_DIRECTORY_LT_VERY_FASTFLASHING:return "DE_DIRECTORY_LT_VERY_FASTFLASHING";//0x1134
		case DE_DIRECTORY_LT_QUICKFLASH:	return "DE_DIRECTORY_LT_QUICKFLASH";//0x1135
		case DE_DIRECTORY_LT_WINK:			return "DE_DIRECTORY_LT_WINK";//0x1136
		case DE_DIRECTORY_LT_SLOW_WINK:		return "DE_DIRECTORY_LT_SLOW_WINK";//0x1137
		case DE_DIRECTORY_LT_MEDIUM_WINK:	return "DE_DIRECTORY_LT_MEDIUM_WINK";//0x1138
		case DE_RINGTONE_LT_OFF:			return "DE_RINGTONE_LT_OFF";//0x1139
		case DE_RINGTONE_LT_ON:				return "DE_RINGTONE_LT_ON";//0x113A
		case DE_RINGTONE_LT_FLASHING:		return "DE_RINGTONE_LT_FLASHING";//0x113B
		case DE_RINGTONE_LT_FASTFLASHING:	return "DE_RINGTONE_LT_FASTFLASHING";//0x113C
		case DE_RINGTONE_LT_VERY_FASTFLASHING:return "DE_RINGTONE_LT_VERY_FASTFLASHING";//0x113D
		case DE_RINGTONE_LT_QUICKFLASH:		return "DE_RINGTONE_LT_QUICKFLASH";//0x113E
		case DE_RINGTONE_LT_WINK:			return "DE_RINGTONE_LT_WINK";//0x113F
		case DE_RINGTONE_LT_SLOW_WINK:		return "DE_RINGTONE_LT_SLOW_WINK";//0x1140
		case DE_RINGTONE_LT_MEDIUM_WINK:	return "DE_RINGTONE_LT_MEDIUM_WINK";//0x1141
		case DE_SAVE_LT_OFF:				return "DE_SAVE_LT_OFF";//0x1142
		case DE_SAVE_LT_ON:					return "DE_SAVE_LT_ON";//0x1143
		case DE_SAVE_LT_FLASHING:			return "DE_SAVE_LT_FLASHING";//0x1144
		case DE_SAVE_LT_FASTFLASHING:		return "DE_SAVE_LT_FASTFLASHING";//0x1145
		case DE_SAVE_LT_VERY_FASTFLASHING:	return "DE_SAVE_LT_VERY_FASTFLASHING";//0x1146
		case DE_SAVE_LT_QUICKFLASH:			return "DE_SAVE_LT_QUICKFLASH";//0x1147
		case DE_SAVE_LT_WINK:				return "DE_SAVE_LT_WINK";//0x1148
		case DE_SAVE_LT_SLOW_WINK:			return "DE_SAVE_LT_SLOW_WINK";//0x1149
		case DE_SAVE_LT_MEDIUM_WINK:		return "DE_SAVE_LT_MEDIUM_WINK";//0x114A
		case DE_FUNC_LT_WINK:				return "DE_FUNC_LT_WINK";//0x114B
		case DE_FUNC_LT_SLOW_WINK:			return "DE_FUNC_LT_SLOW_WINK";//0x114C
		case DE_FUNC_LT_MEDIUM_WINK:		return "DE_FUNC_LT_MEDIUM_WINK";//0x114D
		case DE_CALLWAITING_BTN_PRS:		return "DE_CALLWAITING_BTN_PRS";//0x114E
		case DE_CALLWAITING_BTN_RLS:		return "DE_CALLWAITING_BTN_RLS";//0x114F
		case DE_PARK_BTN_PRS:				return "DE_PARK_BTN_PRS";//0x1150
		case DE_PARK_BTN_RLS:				return "DE_PARK_BTN_RLS";//0x1151
		case DE_NEWCALL_BTN_PRS:			return "DE_NEWCALL_BTN_PRS";//0x1152
		case DE_NEWCALL_BTN_RLS:			return "DE_NEWCALL_BTN_RLS";//0x1153
		case DE_PARK_LT_OFF:				return "DE_PARK_LT_OFF";//0x1154
		case DE_PARK_LT_ON:					return "DE_PARK_LT_ON";//0x1155
		case DE_PARK_LT_FLASHING:			return "DE_PARK_LT_FLASHING";//0x1156
		case DE_PARK_LT_FASTFLASHING:		return "DE_PARK_LT_FASTFLASHING";//0x1157
		case DE_PARK_LT_VERY_FASTFLASHING:	return "DE_PARK_LT_VERY_FASTFLASHING";//0x1158
		case DE_PARK_LT_QUICKFLASH:			return "DE_PARK_LT_QUICKFLASH";//0x1159
		case DE_PARK_LT_WINK:				return "DE_PARK_LT_WINK";//0x115A
		case DE_PARK_LT_SLOW_WINK:			return "DE_PARK_LT_SLOW_WINK";//0x115B
		case DE_PARK_LT_MEDIUM_WINK:		return "DE_PARK_LT_MEDIUM_WINK";//0x115C
		case DE_SCROLL_BTN_PRS:				return "DE_SCROLL_BTN_PRS";//0x115D
		case DE_SCROLL_BTN_RLS:				return "DE_SCROLL_BTN_RLS";//0x115E
		case DE_DIVERT_BTN_PRS:				return "DE_DIVERT_BTN_PRS";//0x115F
		case DE_DIVERT_BTN_RLS:				return "DE_DIVERT_BTN_RLS";//0x1160
		case DE_GROUP_BTN_PRS:				return "DE_GROUP_BTN_PRS";//0x1161
		case DE_GROUP_BTN_RLS:				return "DE_GROUP_BTN_RLS";//0x1162
		case DE_SPEEDDIAL_BTN_PRS:			return "DE_SPEEDDIAL_BTN_PRS";//0x1163
		case DE_SPEEDDIAL_BTN_RLS:			return "DE_SPEEDDIAL_BTN_RLS";//0x1164
		case DE_DND_BTN_PRS:				return "DE_DND_BTN_PRS";//0x1165
		case DE_DND_BTN_RLS:				return "DE_DND_BTN_RLS";//0x1166
		case DE_ENTER_BTN_PRS:				return "DE_ENTER_BTN_PRS";//0x1167
		case DE_ENTER_BTN_RLS:				return "DE_ENTER_BTN_RLS";//0x1168
		case DE_CLEAR_BTN_PRS:				return "DE_CLEAR_BTN_PRS";//0x1169
		case DE_CLEAR_BTN_RLS:				return "DE_CLEAR_BTN_RLS";//0x116A
		case DE_DESTINATION_BTN_PRS:		return "DE_DESTINATION_BTN_PRS";//0x116B
		case DE_DESTINATION_BTN_RLS:		return "DE_DESTINATION_BTN_RLS";//0x116C
		case DE_DND_LT_OFF:					return "DE_DND_LT_OFF";//0x116D
		case DE_DND_LT_ON:					return "DE_DND_LT_ON";//0x116E
		case DE_DND_LT_FLASHING:			return "DE_DND_LT_FLASHING";//0x116F
		case DE_DND_LT_FASTFLASHING:		return "DE_DND_LT_FASTFLASHING";//0x1170
		case DE_DND_LT_VERY_FASTFLASHING:	return "DE_DND_LT_VERY_FASTFLASHING";//0x1171
		case DE_DND_LT_QUICKFLASH:			return "DE_DND_LT_QUICKFLASH";//0x1172
		case DE_DND_LT_WINK:				return "DE_DND_LT_WINK";//0x1173
		case DE_DND_LT_SLOW_WINK:			return "DE_DND_LT_SLOW_WINK";//0x1174
		case DE_DND_LT_MEDIUM_WINK:			return "DE_DND_LT_MEDIUM_WINK";//0x1175
		case DE_GROUP_LT_OFF:				return "DE_GROUP_LT_OFF";//0x1176
		case DE_GROUP_LT_ON:				return "DE_GROUP_LT_ON";//0x1177
		case DE_GROUP_LT_FLASHING:			return "DE_GROUP_LT_FLASHING";//0x1178
		case DE_GROUP_LT_FASTFLASHING:		return "DE_GROUP_LT_FASTFLASHING";//0x1179
		case DE_GROUP_LT_VERY_FASTFLASHING:return "DE_GROUP_LT_VERY_FASTFLASHING";//0x117A
		case DE_GROUP_LT_QUICKFLASH:		return "DE_GROUP_LT_QUICKFLASH";//0x117B
		case DE_GROUP_LT_WINK:				return "DE_GROUP_LT_WINK";//0x117C
		case DE_GROUP_LT_SLOW_WINK:			return "DE_GROUP_LT_SLOW_WINK";//0x117D
		case DE_GROUP_LT_MEDIUM_WINK:		return "DE_GROUP_LT_MEDIUM_WINK";//0x117E
		case DE_DIVERT_LT_OFF:				return "DE_DIVERT_LT_OFF";//0x117F
		case DE_DIVERT_LT_ON:				return "DE_DIVERT_LT_ON";//0x1180
		case DE_DIVERT_LT_FLASHING:			return "DE_DIVERT_LT_FLASHING";//0x1181
		case DE_DIVERT_LT_FASTFLASHING:		return "DE_DIVERT_LT_FASTFLASHING";//0x1182
		case DE_DIVERT_LT_VERY_FASTFLASHING:return "DE_DIVERT_LT_VERY_FASTFLASHING";//0x1183
		case DE_DIVERT_LT_QUICKFLASH:		return "DE_DIVERT_LT_QUICKFLASH";//0x1184
		case DE_DIVERT_LT_WINK:				return "DE_DIVERT_LT_WINK";//0x1185
		case DE_DIVERT_LT_SLOW_WINK:		return "DE_DIVERT_LT_SLOW_WINK";//0x1186
		case DE_DIVERT_LT_MEDIUM_WINK:		return "DE_DIVERT_LT_MEDIUM_WINK";//0x1187
		case DE_SCROLL_LT_OFF:				return "DE_SCROLL_LT_OFF";//0x1188
		case DE_SCROLL_LT_ON:				return "DE_SCROLL_LT_ON";//0x1189
		case DE_SCROLL_LT_FLASHING:			return "DE_SCROLL_LT_FLASHING";//0x118A
		case DE_SCROLL_LT_FASTFLASHING:		return "DE_SCROLL_LT_FASTFLASHING";//0x118B
		case DE_SCROLL_LT_VERY_FASTFLASHING:return "DE_SCROLL_LT_VERY_FASTFLASHING";//0x118C
		case DE_SCROLL_LT_QUICKFLASH:		return "DE_SCROLL_LT_QUICKFLASH";//0x118D
		case DE_SCROLL_LT_WINK:				return "DE_SCROLL_LT_WINK";//0x118E
		case DE_SCROLL_LT_SLOW_WINK:		return "DE_SCROLL_LT_SLOW_WINK";//0x118F
		case DE_SCROLL_LT_MEDIUM_WINK:		return "DE_SCROLL_LT_MEDIUM_WINK";//0x1190
		case DE_CALLBACK_BTN_PRS:			return "DE_CALLBACK_BTN_PRS";//0x1191
		case DE_CALLBACK_BTN_RLS:			return "DE_CALLBACK_BTN_RLS";//0x1192
		case DE_FLASH_LT_OFF:				return "DE_FLASH_LT_OFF";//0x1193
		case DE_FLASH_LT_ON:				return "DE_FLASH_LT_ON";//0x1194
		case DE_FLASH_LT_FLASHING:			return "DE_FLASH_LT_FLASHING";//0x1195
		case DE_FLASH_LT_FASTFLASHING:		return "DE_FLASH_LT_FASTFLASHING";//0x1196
		case DE_FLASH_LT_VERY_FASTFLASHING:return "DE_FLASH_LT_VERY_FASTFLASHING";//0x1197
		case DE_FLASH_LT_QUICKFLASH:		return "DE_FLASH_LT_QUICKFLASH";//0x1198
		case DE_FLASH_LT_WINK:				return "DE_FLASH_LT_WINK";//0x1199
		case DE_FLASH_LT_SLOW_WINK:			return "DE_FLASH_LT_SLOW_WINK";//0x119A
		case DE_FLASH_LT_MEDIUM_WINK:		return "DE_FLASH_LT_MEDIUM_WINK";//0x119B
		case DE_MODE_BTN_PRS:				return "DE_MODE_BTN_PRS";//0x119C
		case DE_MODE_BTN_RLS:				return "DE_MODE_BTN_RLS";//0x119D
		case DE_SPEAKER_LT_MEDIUM_WINK:		return "DE_SPEAKER_LT_MEDIUM_WINK";//0x119E
		case DE_MSG_LT_MEDIUM_WINK:			return "DE_MSG_LT_MEDIUM_WINK";//0x119F
		case DE_SPEEDDIAL_LT_OFF:			return "DE_SPEEDDIAL_LT_OFF";//0x11A0
		case DE_SPEEDDIAL_LT_ON:			return "DE_SPEEDDIAL_LT_ON";//0x11A1
		case DE_SPEEDDIAL_LT_FLASHING:		return "DE_SPEEDDIAL_LT_FLASHING";//0x11A2
		case DE_SPEEDDIAL_LT_FASTFLASHING:	return "DE_SPEEDDIAL_LT_FASTFLASHING";//0x11A3
		case DE_SPEEDDIAL_LT_VERY_FASTFLASHING:return "DE_SPEEDDIAL_LT_VERY_FASTFLASHING";//0x11A4
		case DE_SPEEDDIAL_LT_QUICKFLASH:	return "DE_SPEEDDIAL_LT_QUICKFLASH";//0x11A5
		case DE_SPEEDDIAL_LT_WINK:			return "DE_SPEEDDIAL_LT_WINK";//0x11A6
		case DE_SPEEDDIAL_LT_SLOW_WINK:		return "DE_SPEEDDIAL_LT_SLOW_WINK";//0x11A7
		case DE_SPEEDDIAL_LT_MEDIUM_WINK:	return "DE_SPEEDDIAL_LT_MEDIUM_WINK";//0x11A8
		case DE_SELECT_BTN_PRS:				return "DE_SELECT_BTN_PRS";//0x11A9
		case DE_SELECT_BTN_RLS:				return "DE_SELECT_BTN_RLS";//0x11AA
		case DE_PAUSE_BTN_PRS:				return "DE_PAUSE_BTN_PRS";//0x11AB
		case DE_PAUSE_BTN_RLS:				return "DE_PAUSE_BTN_RLS";//0x11AC
		case DE_INTERCOM_BTN_PRS:			return "DE_INTERCOM_BTN_PRS";//0x11AD
		case DE_INTERCOM_BTN_RLS:			return "DE_INTERCOM_BTN_RLS";//0x11AE
		case DE_INTERCOM_LT_OFF:			return "DE_INTERCOM_LT_OFF";//0x11AF
		case DE_INTERCOM_LT_ON:				return "DE_INTERCOM_LT_ON";//0x11B0
		case DE_INTERCOM_LT_FLASHING:		return "DE_INTERCOM_LT_FLASHING";//0x11B1
		case DE_INTERCOM_LT_FASTFLASHING:	return "DE_INTERCOM_LT_FASTFLASHING";//0x11B2
		case DE_INTERCOM_LT_VERY_FASTFLASHING:return "DE_INTERCOM_LT_VERY_FASTFLASHING";//0x11B3
		case DE_INTERCOM_LT_QUICKFLASH:		return "DE_INTERCOM_LT_QUICKFLASH";//0x11B4
		case DE_INTERCOM_LT_WINK:			return "DE_INTERCOM_LT_WINK";//0x11B5
		case DE_INTERCOM_LT_SLOW_WINK:		return "DE_INTERCOM_LT_SLOW_WINK";//0x11B6
		case DE_INTERCOM_LT_MEDIUM_WINK:	return "DE_INTERCOM_LT_MEDIUM_WINK";//0x11B7
		case DE_CFWD_LT_OFF:				return "DE_CFWD_LT_OFF";//0x11B8
		case DE_CFWD_LT_ON:					return "DE_CFWD_LT_ON";//0x11B9
		case DE_CFWD_LT_FLASHING:			return "DE_CFWD_LT_FLASHING";//0x11BA
		case DE_CFWD_LT_FASTFLASHING:		return "DE_CFWD_LT_FASTFLASHING";//0x11BB
		case DE_CFWD_LT_VERY_FASTFLASHING:	return "DE_CFWD_LT_VERY_FASTFLASHING";//0x11BC
		case DE_CFWD_LT_QUICKFLASH:			return "DE_CFWD_LT_QUICKFLASH";//0x11BD
		case DE_CFWD_LT_WINK:				return "DE_CFWD_LT_WINK";//0x11BE
		case DE_CFWD_LT_SLOW_WINK:			return "DE_CFWD_LT_SLOW_WINK";//0x11BF
		case DE_CFWD_LT_MEDIUM_WINK:		return "DE_CFWD_LT_MEDIUM_WINK";//0x11C0
		case DE_CFWD_BTN_PRS:				return "DE_CFWD_BTN_PRS";//0x11C1
		case DE_CFWD_BTN_RLS:				return "DE_CFWD_BTN_RLS";//0x11C2
		case DE_SPECIAL_LT_OFF:				return "DE_SPECIAL_LT_OFF";//0x11C3
		case DE_SPECIAL_LT_ON:				return "DE_SPECIAL_LT_ON";//0x11C4
		case DE_SPECIAL_LT_FLASHING:		return "DE_SPECIAL_LT_FLASHING";//0x11C5
		case DE_SPECIAL_LT_FASTFLASHING:	return "DE_SPECIAL_LT_FASTFLASHING";//0x11C6
		case DE_SPECIAL_LT_VERY_FASTFLASHING:return "DE_SPECIAL_LT_VERY_FASTFLASHING";//0x11C7
		case DE_SPECIAL_LT_QUICKFLASH:		return "DE_SPECIAL_LT_QUICKFLASH";//0x11C8
		case DE_SPECIAL_LT_WINK:			return "DE_SPECIAL_LT_WINK";//0x11C9
		case DE_SPECIAL_LT_SLOW_WINK:		return "DE_SPECIAL_LT_SLOW_WINK";//0x11CA
		case DE_SPECIAL_LT_MEDIUM_WINK:		return "DE_SPECIAL_LT_MEDIUM_WINK";//0x11CB
		case DE_SPECIAL_BTN_PRS:			return "DE_SPECIAL_BTN_PRS";//0x11CC
		case DE_SPECIAL_BTN_RLS:			return "DE_SPECIAL_BTN_RLS";//0x11CD
		case DE_FORWARD_LT_OFF:				return "DE_FORWARD_LT_OFF";//0x11CE
		case DE_FORWARD_LT_ON:				return "DE_FORWARD_LT_ON";//0x11CF
		case DE_FORWARD_LT_FLASHING:		return "DE_FORWARD_LT_FLASHING";//0x11D0
		case DE_FORWARD_LT_FASTFLASHING:	return "DE_FORWARD_LT_FASTFLASHING";//0x11D1
		case DE_FORWARD_LT_VERY_FASTFLASHING:return "DE_FORWARD_LT_VERY_FASTFLASHING";//0x11D2
		case DE_FORWARD_LT_QUICKFLASH:		return "DE_FORWARD_LT_QUICKFLASH";//0x11D3
		case DE_FORWARD_LT_WINK:			return "DE_FORWARD_LT_WINK";//0x11D4
		case DE_FORWARD_LT_SLOW_WINK:		return "DE_FORWARD_LT_SLOW_WINK";//0x11D5
		case DE_FORWARD_LT_MEDIUM_WINK:		return "DE_FORWARD_LT_MEDIUM_WINK";//0x11D6
		case DE_FORWARD_BTN_PRS:			return "DE_FORWARD_BTN_PRS";//0x11D7
		case DE_FORWARD_BTN_RLS:			return "DE_FORWARD_BTN_RLS";//0x11D8
		case DE_OUTGOING_LT_OFF:			return "DE_OUTGOING_LT_OFF";//0x11D9
		case DE_OUTGOING_LT_ON:				return "DE_OUTGOING_LT_ON";//0x11DA
		case DE_OUTGOING_LT_FLASHING:		return "DE_OUTGOING_LT_FLASHING";//0x11DB
		case DE_OUTGOING_LT_FASTFLASHING:	return "DE_OUTGOING_LT_FASTFLASHING";//0x11DC
		case DE_OUTGOING_LT_VERY_FASTFLASHING:return "DE_OUTGOING_LT_VERY_FASTFLASHING";//0x11DD
		case DE_OUTGOING_LT_QUICKFLASH:		return "DE_OUTGOING_LT_QUICKFLASH";//0x11DE
		case DE_OUTGOING_LT_WINK:			return "DE_OUTGOING_LT_WINK";//0x11DF
		case DE_OUTGOING_LT_SLOW_WINK:		return "DE_OUTGOING_LT_SLOW_WINK";//0x11E0
		case DE_OUTGOING_LT_MEDIUM_WINK:	return "DE_OUTGOING_LT_MEDIUM_WINK";//0x11E1
		case DE_OUTGOING_BTN_PRS:			return "DE_OUTGOING_BTN_PRS";//0x11E2
		case DE_OUTGOING_BTN_RLS:			return "DE_OUTGOING_BTN_RLS";//0x11E3
		case DE_BACKSPACE_LT_OFF:			return "DE_BACKSPACE_LT_OFF";//0x11E4
		case DE_BACKSPACE_LT_ON:			return "DE_BACKSPACE_LT_ON";//0x11E5
		case DE_BACKSPACE_LT_FLASHING:		return "DE_BACKSPACE_LT_FLASHING";//0x11E6
		case DE_BACKSPACE_LT_FASTFLASHING:	return "DE_BACKSPACE_LT_FASTFLASHING";//0x11E7
		case DE_BACKSPACE_LT_VERY_FASTFLASHING:return "DE_BACKSPACE_LT_VERY_FASTFLASHING";//0x11E8
		case DE_BACKSPACE_LT_QUICKFLASH:	return "DE_BACKSPACE_LT_QUICKFLASH";//0x11E9
		case DE_BACKSPACE_LT_WINK:			return "DE_BACKSPACE_LT_WINK";//0x11EA
		case DE_BACKSPACE_LT_SLOW_WINK:		return "DE_BACKSPACE_LT_SLOW_WINK";//0x11EB
		case DE_BACKSPACE_LT_MEDIUM_WINK:	return "DE_BACKSPACE_LT_MEDIUM_WINK";//0x11EC
		case DE_BACKSPACE_BTN_PRS:			return "DE_BACKSPACE_BTN_PRS";//0x11ED
		case DE_BACKSPACE_BTN_RLS:			return "DE_BACKSPACE_BTN_RLS";//0x11EE
		case DE_START_TONE:					return "DE_START_TONE";//0x11EF
		case DE_STOP_TONE:					return "DE_STOP_TONE";//0x11F0
		case DE_FLASHHOOK:					return "DE_FLASHHOOK";//0x11F1
		case DE_LINE_BTN_RLS:				return "DE_LINE_BTN_RLS";//0x11F2
		case DE_FEATURE_BTN_RLS:			return "DE_FEATURE_BTN_RLS";//0x11F3
		case DE_MUTE_BTN_RLS:				return "DE_MUTE_BTN_RLS";//0x11F4
		case DE_HELP_BTN_RLS:				return "DE_HELP_BTN_RLS";//0x11F5
		case DE_LOGON_BTN_RLS:				return "DE_LOGON_BTN_RLS";//0x11F6
		case DE_ANSWER_BTN_RLS:				return "DE_ANSWER_BTN_RLS";//0x11F7
		case DE_PROGRAM_BTN_RLS:			return "DE_PROGRAM_BTN_RLS";//0x11F8
		case DE_CONFERENCE_BTN_RLS:			return "DE_CONFERENCE_BTN_RLS";//0x11F9
		case DE_RECALL_BTN_RLS:				return "DE_RECALL_BTN_RLS";//0x11FA
		case DE_BREAK_BTN_RLS:				return "DE_BREAK_BTN_RLS";//0x11FB
		case DE_WORK_BTN_RLS:				return "DE_WORK_BTN_RLS";//0x11FC
		case DE_TALLY_BTN_RLS:				return "DE_TALLY_BTN_RLS";//0x11FD
		case DE_EXPAND_LT_OFF:				return "DE_EXPAND_LT_OFF";//0x1200
		case DE_EXPAND_LT_ON:				return "DE_EXPAND_LT_ON";//0x1201
		case DE_EXPAND_LT_FLASHING:			return "DE_EXPAND_LT_FLASHING";//0x1202
		case DE_EXPAND_LT_FASTFLASHING:		return "DE_EXPAND_LT_FASTFLASHING";//0x1203
		case DE_EXPAND_LT_VERY_FASTFLASHING:return "DE_EXPAND_LT_VERY_FASTFLASHING";//0x1204
		case DE_EXPAND_LT_QUICKFLASH:		return "DE_EXPAND_LT_QUICKFLASH";//0x1205
		case DE_EXPAND_LT_WINK:				return "DE_EXPAND_LT_WINK";//0x1206
		case DE_EXPAND_LT_SLOW_WINK:		return "DE_EXPAND_LT_SLOW_WINK";//0x1207
		case DE_EXPAND_LT_MEDIUM_WINK:		return "DE_EXPAND_LT_MEDIUM_WINK";//0x1208
		case DE_EXPAND_BTN_PRS:				return "DE_EXPAND_BTN_PRS";//0x1209
		case DE_EXPAND_BTN_RLS:				return "DE_EXPAND_BTN_RLS";//0x120A
		case DE_SERVICES_LT_OFF:			return "DE_SERVICES_LT_OFF";//0x1210
		case DE_SERVICES_LT_ON:				return "DE_SERVICES_LT_ON";//0x1211
		case DE_SERVICES_LT_FLASHING:		return "DE_SERVICES_LT_FLASHING";//0x1212
		case DE_SERVICES_LT_FASTFLASHING:	return "DE_SERVICES_LT_FASTFLASHING";//0x1213
		case DE_SERVICES_LT_VERY_FASTFLASHING:return "DE_SERVICES_LT_VERY_FASTFLASHING";//0x1214
		case DE_SERVICES_LT_QUICKFLASH:		return "DE_SERVICES_LT_QUICKFLASH";//0x1215
		case DE_SERVICES_LT_WINK:			return "DE_SERVICES_LT_WINK";//0x1216
		case DE_SERVICES_LT_SLOW_WINK:		return "DE_SERVICES_LT_SLOW_WINK";//0x1217
		case DE_SERVICES_LT_MEDIUM_WINK:	return "DE_SERVICES_LT_MEDIUM_WINK";//0x1218
		case DE_SERVICES_BTN_PRS:			return "DE_SERVICES_BTN_PRS";//0x1219
		case DE_SERVICES_BTN_RLS:			return "DE_SERVICES_BTN_RLS";//0x121A
		case DE_HEADSET_LT_OFF:				return "DE_HEADSET_LT_OFF";//0x1220
		case DE_HEADSET_LT_ON:				return "DE_HEADSET_LT_ON";//0x1221
		case DE_HEADSET_LT_FLASHING:		return "DE_HEADSET_LT_FLASHING";//0x1222
		case DE_HEADSET_LT_FASTFLASHING:	return "DE_HEADSET_LT_FASTFLASHING";//0x1223
		case DE_HEADSET_LT_VERY_FASTFLASHING:return "DE_HEADSET_LT_VERY_FASTFLASHING";//0x1224
		case DE_HEADSET_LT_QUICKFLASH:		return "DE_HEADSET_LT_QUICKFLASH";//0x1225
		case DE_HEADSET_LT_WINK:			return "DE_HEADSET_LT_WINK";//0x1226
		case DE_HEADSET_LT_SLOW_WINK:		return "DE_HEADSET_LT_SLOW_WINK";//0x1227
		case DE_HEADSET_LT_MEDIUM_WINK:		return "DE_HEADSET_LT_MEDIUM_WINK";//0x1228
		case DE_HEADSET_BTN_PRS:			return "DE_HEADSET_BTN_PRS";//0x1229
		case DE_HEADSET_BTN_RLS:			return "DE_HEADSET_BTN_RLS";//0x122A
		case DE_NAVIGATION_BTN_PRS:			return "DE_NAVIGATION_BTN_PRS";//0x1239
		case DE_NAVIGATION_BTN_RLS:			return "DE_NAVIGATION_BTN_RLS";//0x123A
		case DE_COPY_LT_OFF:				return "DE_COPY_LT_OFF";//0x1240
		case DE_COPY_LT_ON:					return "DE_COPY_LT_ON";//0x1241
		case DE_COPY_LT_FLASHING:			return "DE_COPY_LT_FLASHING";//0x1242
		case DE_COPY_LT_FASTFLASHING:		return "DE_COPY_LT_FASTFLASHING";//0x1243
		case DE_COPY_LT_VERY_FASTFLASHING:	return "DE_COPY_LT_VERY_FASTFLASHING";//0x1244
		case DE_COPY_LT_QUICKFLASH:			return "DE_COPY_LT_QUICKFLASH";//0x1245
		case DE_COPY_LT_WINK:				return "DE_COPY_LT_WINK";//0x1246
		case DE_COPY_LT_SLOW_WINK:			return "DE_COPY_LT_SLOW_WINK";//0x1247
		case DE_COPY_LT_MEDIUM_WINK:		return "DE_COPY_LT_MEDIUM_WINK";//0x1248
		case DE_COPY_BTN_PRS:				return "DE_COPY_BTN_PRS";//0x1249
		case DE_COPY_BTN_RLS:				return "DE_COPY_BTN_RLS";//0x124A
		case DE_LINE_LT_MEDIUM_WINK:		return "DE_LINE_LT_MEDIUM_WINK";//0x1250
		case DE_MIC_LT_MEDIUM_WINK:			return "DE_MIC_LT_MEDIUM_WINK";//0x1251
		case DE_HOLD_LT_MEDIUM_WINK:		return "DE_HOLD_LT_MEDIUM_WINK";//0x1252
		case DE_RELEASE_LT_MEDIUM_WINK:		return "DE_RELEASE_LT_MEDIUM_WINK";//0x1253
		case DE_HELP_LT_MEDIUM_WINK:		return "DE_HELP_LT_MEDIUM_WINK";//0x1254
		case DE_SUPERVISOR_LT_MEDIUM_WINK:	return "DE_SUPERVISOR_LT_MEDIUM_WINK";//0x1255
		case DE_READY_LT_MEDIUM_WINK:		return "DE_READY_LT_MEDIUM_WINK";//0x1256
		case DE_LOGON_LT_MEDIUM_WINK:		return "DE_LOGON_LT_MEDIUM_WINK";//0x1257
		case DE_WRAPUP_LT_MEDIUM_WINK:		return "DE_WRAPUP_LT_MEDIUM_WINK";//0x1258
		case DE_RING_LT_MEDIUM_WINK:		return "DE_RING_LT_MEDIUM_WINK";//0x1259
		case DE_ANSWER_LT_MEDIUM_WINK:		return "DE_ANSWER_LT_MEDIUM_WINK";//0x125A
		case DE_PROGRAM_LT_SLOW_WINK:		return "DE_PROGRAM_LT_SLOW_WINK";//0x125B
		case DE_TRANSFER_LT_SLOW_WINK:		return "DE_TRANSFER_LT_SLOW_WINK";//0x125C
		case DE_CONFERENCE_LT_SLOW_WINK:	return "DE_CONFERENCE_LT_SLOW_WINK";//0x125D
		case DE_SOFT_LT_MEDIUM_WINK:		return "DE_SOFT_LT_MEDIUM_WINK";//0x125E
		case DE_MENU_LT_MEDIUM_WINK:		return "DE_MENU_LT_MEDIUM_WINK";//0x125F
		case DE_CALLWAITING_LT_MEDIUM_WINK:return "DE_CALLWAITING_LT_MEDIUM_WINK";//0x1260
		case DE_REDIAL_LT_MEDIUM_WINK:		return "DE_REDIAL_LT_MEDIUM_WINK";//0x1261
		case DE_PAGE_LT_MEDIUM_WINK:		return "DE_PAGE_LT_MEDIUM_WINK";//0x1262
		case DE_FEATURE_LT_MEDIUM_WINK:		return "DE_FEATURE_LT_MEDIUM_WINK";//0x1263
		case DE_PAGE_LT_WINK:				return "DE_PAGE_LT_WINK";//0x1264
		case DE_PAGE_LT_SLOW_WINK:			return "DE_PAGE_LT_SLOW_WINK";//0x1265
		case DE_CALLBACK_LT_ON:				return "DE_CALLBACK_LT_ON";//0x1267
		case DE_CALLBACK_LT_FLASHING:		return "DE_CALLBACK_LT_FLASHING";//0x1268
		case DE_CALLBACK_LT_WINK:			return "DE_CALLBACK_LT_WINK";//0x1269
		case DE_CALLBACK_LT_FASTFLASHING:	return "DE_CALLBACK_LT_FASTFLASHING";//0x126a
		case DE_ICM_LT_OFF:					return "DE_ICM_LT_OFF";//0x126b
		case DE_ICM_LT_ON:					return "DE_ICM_LT_ON";//0x126c
		case DE_ICM_LT_FLASHING:			return "DE_ICM_LT_FLASHING";//0x126d
		case DE_ICM_LT_WINK:				return "DE_ICM_LT_WINK";//0x126e
		case DE_ICM_LT_FASTFLASHING:		return "DE_ICM_LT_FASTFLASHING";//0x126f
		case DE_ICM_BTN_PRS:				return "DE_ICM_BTN_PRS";//0x1270
		case DE_ICM_BTN_RLS:				return "DE_ICM_BTN_RLS";//0x1271
		case DE_CISCO_SCCP_CALL_INFO:		return "DE_CISCO_SCCP_CALL_INFO";//0x1280
		case DE_CALLBACK_LT_OFF:			return "DE_CALLBACK_LT_OFF";//0x1266
		//case DE_CONFERENCE_BTN_PRS:		return "DE_CONFERENCE_BTN_PRS";
		//case DE_FUNC_LT_FASTFLASHING:		return "DE_FUNC_LT_FASTFLASHING";
		//case DE_FUNC_LT_FLASHING:			return "DE_FUNC_LT_FLASHING";
		//case DE_FUNC_LT_OFF:				return "DE_FUNC_LT_OFF";
		//case DE_FUNC_LT_ON:				return "DE_FUNC_LT_ON";
		//case DE_FUNC_LT_QUICKFLASH:		return "DE_FUNC_LT_QUICKFLASH";
		//case DE_FUNC_LT_VERY_FASTFLASHING:	return "DE_FUNC_LT_VERY_FASTFLASHING";
		case DE_DC_BTN_PRS:					return "DE_DC_BTN_PRS";//0x1301
		case DE_LND_BTN_PRS:				return "DE_LND_BTN_PRS";//0x1302
		case DE_CHK_BTN_PRS:				return "DE_CHK_BTN_PRS";//0x1303
		case DE_CALLSTATE_IDLE:				return "DE_CALLSTATE_IDLE";//0x1304
		case DE_CALLSTATE_DIALING:			return "DE_CALLSTATE_DIALING";//0x1306
		case DE_CALLSTATE_ALERTING:			return "DE_CALLSTATE_ALERTING";//0x1307
		case DE_CALLSTATE_FAR_END_RINGBACK:	return "DE_CALLSTATE_FAR_END_RINGBACK";//0x1308
		case DE_CALLSTATE_TALK:				return "DE_CALLSTATE_TALK";//0x1309
		case DE_SPEEDDIAL_NUMBER:			return "DE_SPEEDDIAL_NUMBER";//0x130a
		case DE_CALLSTATE_DIAL_COMPLETED:	return "DE_CALLSTATE_DIAL_COMPLETED";//0x130b
		case DE_CALLSTATE_BUSY_TONE:		return "DE_CALLSTATE_BUSY_TONE";//0x130c
		case DE_CALLSTATE_INUSE:			return "DE_CALLSTATE_INUSE";//0x130d

		case DE_SIP_RAW_MSG:				return "DE_SIP_RAW_MSG"; //0x1400

		case DE_CALL_IN_PROGRESS:			return "DE_CALL_IN_PROGRESS";//0x6b
		case DE_CALL_ALERTING:				return "DE_CALL_ALERTING";//0x6e
		case DE_CALL_CONNECTED:				return "DE_CALL_CONNECTED";//0x6f
		case DE_CALL_RELEASED:				return "DE_CALL_RELEASED";//0x70
		case DE_CALL_SUSPENDED:				return "DE_CALL_SUSPENDED";//0x71
		case DE_CALL_RESUMED:				return "DE_CALL_RESUMED";//0x72
		case DE_CALL_HELD:					return "DE_CALL_HELD";//0x73
		case DE_CALL_RETRIEVED:				return "DE_CALL_RETRIEVED";//0x74
		case DE_CALL_ABANDONED:				return "DE_CALL_ABANDONED";//0x75
		case DE_CALL_REJECTED:				return "DE_CALL_REJECTED";//0x76
		default:
			{
				std::stringstream oss;
				oss << "unknown State:0x" << std::hex << nState;
				return oss.str();
			}
	}
}

std::string CRecorderDlg::GetSsmLastErrMsg()
{
	char Err[600];
	SsmGetLastErrMsg(Err); //Get error message
	return Err;
}

std::string CRecorderDlg::GetSalverResultMsg(unsigned int result)
{
	switch(result)
	{
	case 0: return "成功";
	case 1: return "未知";
	case 2: return "超时";
	case 3: return "报文数据异常";
	case 4: return "该SessionId已处于活动中";
	case 5: return "忙资源列表中找不到该SessionId";
	case 6: return "创建新的Session失败";
	case 7: return "文件创建失败";
	case 8: return "找不到活动的SessionId";
	case 9: return "该SessionId已处于录音中";
	case 10: return "该SessionId已处于录音停止中";
	case 11: return "该SessionId未处于录音状态"; 
	case 12: return "该SessionId未处于录音到文件状态";
	case 13: return "该SessionId未处于录音暂停状态";
	case 14: return "要求录音的数据长度不够";
	case 15: return "初始化WAV文件头出错";
	case 16: return "该Slaver之前已经被分配过资源并开启";
	case 17: return "申请资源过程失败";
	case 18: return "该Slaver本来就没有被开启";
	case 19: return "不支持的文件编码格式";
	case 20: return "不支持的RTP";
	case 21: return "没有足够的空闲资源";
	default: {
		std::stringstream oss;
		oss << "unknown :"  << result;
		return oss.str();
	}
	}
}
bool CRecorderDlg::CreateMultipleDirectory(const CString& szPath)
{
	CString strDir(szPath);//存放要创建的目录字符串
	//确保以'\'结尾以创建最后一个目录
	if (strDir.GetAt(strDir.GetLength()-1)!=_T('\\'))
	{
		strDir.AppendChar(_T('\\'));
	}
	std::vector<CString> vPath;//存放每一层目录字符串
	CString strTemp;//一个临时变量,存放目录字符串
	bool bSuccess = false;//成功标志
	//遍历要创建的字符串
	for (int i=0;i<strDir.GetLength();++i)
	{
		if (strDir.GetAt(i) != _T('\\')) 
		{//如果当前字符不是'\\'
			strTemp.AppendChar(strDir.GetAt(i));
		}
		else 
		{//如果当前字符是'\\'
			vPath.push_back(strTemp);//将当前层的字符串添加到数组中
			strTemp.AppendChar(_T('\\'));
		}
	}

	//遍历存放目录的数组,创建每层目录
	std::vector<CString>::const_iterator vIter;
	for (vIter = vPath.begin(); vIter != vPath.end(); vIter++) 
	{
		//如果CreateDirectory执行成功,返回true,否则返回false
		if(!PathFileExists(*vIter))
			bSuccess = CreateDirectory(*vIter, NULL) ? true : false;    
	}

	return bSuccess;
}


void CRecorderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: Add your message handler code here and/or call default
	if(nID==SC_MINIMIZE || nID == SC_CLOSE)
	{
		ShowWindow(SW_SHOWMINIMIZED);
		ShowWindow(SW_HIDE);
	}
	else
		CDialogEx::OnSysCommand(nID, lParam);
}


int CRecorderDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here
	if (!m_TrayIcon.Create(this, WM_ICON_NOTIFY, _T("录音程序"),this->m_hIcon, IDR_TRAY_MENU))
		return -1;
	return 0;
}

LRESULT CRecorderDlg::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	if (wParam != IDR_TRAY_MENU)
		return 0L;

	CMenu menu, *pSubMenu;
	if (LOWORD(lParam) == WM_RBUTTONUP)
	{ 
		CPoint pos;
		GetCursorPos(&pos);
		if (!menu.LoadMenu(IDR_TRAY_MENU)) return 0;

		if (!(pSubMenu=menu.GetSubMenu(0))) return 0;

		::SetMenuDefaultItem(pSubMenu->m_hMenu, 3, TRUE);
		SetForegroundWindow(); 
		pSubMenu->TrackPopupMenu(TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON, pos.x, pos.y,
			this);
		menu.DestroyMenu();
	}
	else if (LOWORD(lParam) == WM_LBUTTONDBLCLK) 
	{
		if (!menu.LoadMenu(IDR_TRAY_MENU)) return 0;
		if (!(pSubMenu = menu.GetSubMenu(0))) return 0;

		SetForegroundWindow();
		//激活第显示菜单项
		SendMessage(WM_COMMAND,ID_SHOW, 0);
		menu.DestroyMenu();

	}
		return 0;
}

void CRecorderDlg::OnShow()
{
	// TODO: Add your command handler code here
	ShowWindow(SW_SHOW);
	ShowWindow(SW_RESTORE);
}


void CRecorderDlg::OnExit()
{
	// TODO: Add your command handler code here
	this->EndDialog(0);
}

int CRecorderDlg::MySpyChToCic(int nCh)
{
	int nCic = -1;
	if((nCic = SpyChToCic(nCh)) == -1)
	{
		LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SpyChToCic"));
	}else{
		LOG4CPLUS_DEBUG(log,"Ch:" << nCh << " change to cic nCic:" << nCic);
	}
	return nCic;
}

void CRecorderDlg::ScanSlaver() 
{
	// TODO: Add your control notification handler code here
	static log4cplus::Logger log = log4cplus::Logger::getInstance("Recorder");

	nSlaverCount = SsmIPRGetRecSlaverCount(nIPRBoardId);
	LOG4CPLUS_INFO(log, "SlaverCount:" << nSlaverCount);
	if(nSlaverCount)
	{
		SsmIPRGetRecSlaverList(nIPRBoardId, nSlaverCount, &nSlaverCount, &IPR_SlaverAddr[0]);

		for (int i =0; i < nSlaverCount; i++)
		{
			COMPUTER_INFO ComputerInfo;
			BOOL bStartd;
			SsmIPRGetRecSlaverInfo(nIPRBoardId,IPR_SlaverAddr[i].nRecSlaverID, &bStartd,
				&IPR_SlaverAddr[i].nTotalResources,
				&IPR_SlaverAddr[i].nUsedResources,
				&IPR_SlaverAddr[i].nThreadPairs,
				&ComputerInfo);
			LOG4CPLUS_INFO(log, "Active Slaver:" << i << ", SlaverID:" << IPR_SlaverAddr[i].nRecSlaverID
				<< ", Start:" << bStartd
				<< ",IP:" << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b1) << "." << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b2) << "." << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b3) << "." << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b4) << ":" << IPR_SlaverAddr[i].ipAddr.usPort
				<< ", ThreadPairs:" << IPR_SlaverAddr[i].nThreadPairs
				<< ", TotalResources:" << IPR_SlaverAddr[i].nTotalResources
				<< ", UsedResources:"<< IPR_SlaverAddr[i].nUsedResources 
				<< ",IPAddr:" << ComputerInfo.szIPAddr);
		}
	}
}

void CRecorderDlg::StartSlaver()
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("Recorder");
	for (int i =0; i < nSlaverCount; i++)
	{
		int m_nInitTotalResources = nIPRChNum;
		int m_nInitThreadPairs = nIPRChNum;
		LOG4CPLUS_DEBUG(log, "Start Slaver:" << i << ", SlaverID:" << IPR_SlaverAddr[i].nRecSlaverID);
		int nResult = SsmIPRStartRecSlaver(nIPRBoardId, IPR_SlaverAddr[i].nRecSlaverID, &m_nInitTotalResources, &m_nInitThreadPairs);
		if(nResult < 0)
		{
			LOG4CPLUS_ERROR(log, GetSsmLastErrMsg());
		}
		else{
			LOG4CPLUS_INFO(log, "Start Slaver:" << i << ", SlaverID:" << IPR_SlaverAddr[i].nRecSlaverID<< ", " << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b1) << "." << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b2) << "." << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b3) << "." << (int)(IPR_SlaverAddr[i].ipAddr.S_un_b.s_b4) << ":" << IPR_SlaverAddr[i].ipAddr.usPort
				<< ", ThreadPairs:" << m_nInitThreadPairs
				<< ", TotalResources:" << m_nInitTotalResources );
		}
	}
}

int CRecorderDlg::SerchIdleSlaverAndIPA(int nCh, RECORD_DIRECTION rDirction)
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("Recorder");
	if(nSlaverCount > 0)
	{
		//查找空闲的 IPR通道
		BOOL bFind = FALSE;
		int iprCh ;
		for(iprCh=0; iprCh<nMaxCh; iprCh++)
		{
			if(ChMap[iprCh].nChType == CH_TYPE_IPR 
				&& ChMap[iprCh].nState == CH_IDLE)
			{
				ChMap[iprCh].dwSessionId = ChMap[nCh].dwSessionId;
				ChMap[iprCh].nStationId = ChMap[nCh].nStationId;
				ChMap[iprCh].nCallRef = ChMap[nCh].nCallRef;
				ChMap[iprCh].nPtlType = ChMap[nCh].nPtlType;
				ChMap[iprCh].szCalleeId = ChMap[nCh].szCalleeId;
				ChMap[iprCh].szCallerId = ChMap[nCh].szCallerId;
				ChMap[iprCh].nPrimaryCodec = ChMap[nCh].nPrimaryCodec;
				ChMap[iprCh].wRecDirection = rDirction;//只录外呼声音
				bFind = TRUE;
				break;
			}
		}

		if(!bFind)
		{
			LOG4CPLUS_ERROR(log, "Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId 
				<< ",CallRef:" <<  ChMap[nCh].nCallRef
				<< ",StationId:" <<  ChMap[nCh].nStationId << " not find idle IPR channel");
			return -1;
		}

		//查找空闲的SlaverId
		int nSlaverIndex = -1;
		for(int j=0; j<nSlaverCount; j++)
		{
			if(IPR_SlaverAddr[j].nTotalResources - IPR_SlaverAddr[j].nUsedResources > 0)
			{
				nSlaverIndex = j;
				break;
			}
		}
		if(nSlaverIndex < 0){
			LOG4CPLUS_ERROR(log, "Ch:" << nCh << ",SessionId:" << ChMap[nCh].dwSessionId 
				<< ",CallRef:" << ChMap[nCh].nCallRef
				<< ",StationId:" << ChMap[nCh].nStationId << "not find idle Slaver");
			return -2;
		}

		ChMap[iprCh].nRecSlaverId = IPR_SlaverAddr[nSlaverIndex].nRecSlaverID;
		IPR_Addr ipAddr = IPR_SlaverAddr[nSlaverIndex].ipAddr;
		ChMap[iprCh].szIPP_Rec.Format("%d.%d.%d.%d", ipAddr.S_un_b.s_b1, 
			ipAddr.S_un_b.s_b2, 
			ipAddr.S_un_b.s_b3, 
			ipAddr.S_un_b.s_b4);
		ChMap[iprCh].szIPS_Rec.Format("%d.%d.%d.%d",ipAddr.S_un_b.s_b1, 
			ipAddr.S_un_b.s_b2, 
			ipAddr.S_un_b.s_b3, 
			ipAddr.S_un_b.s_b4);
		return iprCh;
	}
	return -1;
}