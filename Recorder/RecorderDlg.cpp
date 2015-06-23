
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

static LPTSTR	StateName[] = {"空闲","收号","振铃","通话","录音","摘机","断线"};		
// CRecorderDlg 对话框




CRecorderDlg::CRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRecorderDlg::IDD, pParent),nMaxCh(0),m_freeCapacity(0),m_totalCapacity(1)
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
	EventSet.dwWorkMode = EVENT_MESSAGE;	
	EventSet.lpHandlerParam = this->GetSafeHwnd();	

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


	CString CErrMsg;			    //error message

	szShIndex.Append("ShIndex.ini");
	szShConfig.Append("ShConfig.ini");

	//load configuration file and initialize system
	if(SsmStartCti(szShConfig, szShIndex) == -1)
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300));//Get error message
		CErrMsg.ReleaseBuffer();
		LOG4CPLUS_ERROR(log, CErrMsg.GetBuffer());
		return FALSE;
	}

	//Judge if the number of initialized boards is the same as
	//		   that of boards specified in the configuration file
	if(SsmGetMaxUsableBoard() != SsmGetMaxCfgBoard())
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300)); //Get error message
		CErrMsg.ReleaseBuffer();
		LOG4CPLUS_ERROR(log, CErrMsg.GetBuffer());
		return FALSE;
	}


	//Get the maximum number of the monitored circuits
	nMaxCh = SpyGetMaxCic();
	if(nMaxCh == -1){
		LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetMaxCic"));
		nMaxCh = SsmGetMaxCh();	  //retrieve channel amounts declared in configuration file 
		if(nMaxCh == -1)
		{
			LOG4CPLUS_ERROR(log, _T("failed to call function SsmGetMaxCh()"));
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
		ChMap[i].CtrlState	= VOLTAGE_CTRL; 
		if (ChMap[i].nChType == CH_TYPE_ANALOG_RECORD)
		{
			int nResult = SsmGetIgnoreLineVoltage(i);
			if(nResult == 0)	//detect voltage
				ChMap[i].bIgnoreLineVoltage = false;
			else if(nResult == 1)
				ChMap[i].bIgnoreLineVoltage = true;
		}
		else{
			ChMap[i].bIgnoreLineVoltage = false;
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
	m_ImageList.Add(m_hIconOnhook);
	m_ImageList.Add(m_hIconOffhook);
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
		||  ChMap[nIndex].nState == CH_RECORDING)
	{
		lvItem.iImage = 0;  //图片索引号(第一幅图片)  
		
	}else
	{
		lvItem.iImage = 1;    //图片索引号(第2幅图片)   
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


LRESULT CRecorderDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class

	//Adopt windows message mechanism
	//	   windows message code：event code + 0x7000(WM_USER)
	if(message > WM_USER)  
	{	

		ULONG32 nEventCode = message - WM_USER;	
		
#pragma region E_CHG_SpyState
		//Event notifying the state change of the monitored circuit
		if(nEventCode == E_CHG_SpyState)	
		{
			LOG4CPLUS_DEBUG(log,"Ch:" << wParam << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			int nCic = wParam;
			UINT32 nNewState = (int)lParam & 0xFFFF;
			if(nCic >= 0 && nCic <= this->nMaxCh)
			{
				LOG4CPLUS_DEBUG(log, "Ch:" << nCic << "newState:" << GetShStateName(nNewState));
				switch(nNewState)
				{
#pragma region 空闲 //Idle state
				case S_SPY_STANDBY:
					{
						StopRecording(nCic);
						SetChannelState(nCic,CH_IDLE);
						UpdateData(FALSE);
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
						if((ChMap[nCic].nCallInCh = SpyGetCallInCh(nCic)) == -1)	//Get the number of incoming channel
							LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyGetCallInCh"));
						if((ChMap[nCic].nCallOutCh = SpyGetCallOutCh(nCic)) == -1)//Get the number of outgoing channel
							LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyGetCallOutCh"));

						SetChannelState(nCic, CH_TALKING);
						/*
						if(ChMap[nCic].szCallerId.Compare("4008001100")){
						   ChMap[nCic].szCalleeId.ReleaseBuffer();
						   LOG4CPLUS_INFO(log, "Ch:" << nCic << "主叫号码判断不通过。");
						   break;
						  } */

						//Start recording
						//Record file name + Monitored circuit number + Time(hour-minute-second)
						SYSTEMTIME st;
						GetLocalTime(&st);
						ChMap[nCic].szFileName.Format("%s\\%04d\\%02d\\%02d\\%04d%02d%02d%02d%02d%02d_%s_%s.wav", m_strFileDir, 
							st.wYear, st.wMonth, st.wDay,
							st.wYear, st.wMonth, st.wDay, 
							st.wHour, st.wMinute, st.wSecond,
							ChMap[nCic].szCallerId, ChMap[nCic].szCalleeId);
						
						 //根据主被叫号码判断录音方向
						if(!ChMap[nCic].szCallerId.Compare("4008001100")){
						   ChMap[nCic].wRecDirection=CALL_OUT_RECORD;
						}else{
						   ChMap[nCic].wRecDirection = CALL_IN_RECORD;
						} 
						LOG4CPLUS_INFO(log, "Ch:" <<  nCic << " StartRecording.");
						if(StartRecording(nCic)){
							SetChannelState(nCic, CH_RECORDING);
							ChMap[nCic].tStartTime = CTime::GetCurrentTime();
							ChMap[nCic].nRecordTimes++;
							ChMap[nCic].sql = "INSERT INTO RecordLog  ( CallerNum,CalleeNum,CustomerID,StarTime,F_Path ,Flag)";
							ChMap[nCic].sql += "VALUES ( '" + ChMap[nCic].szCallerId + "','9" + ChMap[nCic].szCalleeId + "','','" + ChMap[nCic].tStartTime.Format("%Y-%m-%d %H:%M:%S") + "','" + ChMap[nCic].szFileName + "','0') ";
							m_sqlServerDB.addSql2Queue(ChMap[nCic].sql.GetBuffer());
							LOG4CPLUS_TRACE(log, "Ch:" << nCic << " addSql2Queue:" << ChMap[nCic].sql.GetBuffer());
							m_RecordingSum++;
							UpdateData(FALSE);
						}
					}
					break;
#pragma endregion 通话
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
			UpdateCircuitListCtrl(nCic);
		}
#pragma endregion E_CHG_SpyState
#pragma region E_CHG_RcvDTMF
		//Event generated by the driver when DTMF is received
		else if(nEventCode == E_CHG_RcvDTMF)
		{
			int nCh		= wParam;	//number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			if(nCh != -1)
			{
				char cNewDtmf = (char)(0xFFFF & lParam);	//Newly received DTMF
				LOG4CPLUS_INFO(log, "Ch:" << nCh << " newDTMF:" << cNewDtmf);
				ChMap[nCh].szDtmf.AppendChar(cNewDtmf);
			}
			UpdateCircuitListCtrl(nCh);
		}
#pragma endregion E_CHG_RcvDTMF
#pragma region E_PROC_RecordEnd
		//recording end event
		else if(nEventCode == E_PROC_RecordEnd)
		{
			int nCh		= wParam;	//number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			if(ChMap[nCh].nState == CH_RECORDING)
			{
				SetChannelState(nCh, CH_PICKUP);					
			}
		}
#pragma endregion E_PROC_RecordEnd
#pragma region E_CHG_RingCount
		//ring count event
		else if(nEventCode == E_CHG_RingCount)
		{
			int nCh		= wParam;	//number of channel output the event 
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode));			
			if((ChMap[nCh].nState == CH_IDLE ) && (lParam == 2))
			{
				SetChannelState(nCh, CH_RINGING);
				//receive CallerId
				GetCallerAndCallee(nCh);
			}
		}
#pragma endregion E_CHG_RingCount
#pragma region E_CHG_HookState
		else if (nEventCode == E_CHG_HookState)
		{
			//Switching from channel number to circuit number
			int nCic = MySpyChToCic(wParam);
			LOG4CPLUS_DEBUG(log, "Ch:" << nCic << " E_CHG_HookState");
			UINT32 nNewState = lParam;
			switch(nNewState)
			{
#pragma region on hook
			case S_CALL_STANDBY:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCic << " S_CALL_STANDBY");
					SetChannelState(nCic, CH_IDLE);
					ChMap[nCic].szDtmf.Empty();
					ChMap[nCic].szCalleeId.Empty();
					ChMap[nCic].szCallerId.Empty();
					ChMap[nCic].szFileName.Empty();
				}
				break;
#pragma endregion on hook
#pragma region off hook

			case S_CALL_PICKUPED:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCic << " S_CALL_PICKUPED");
					SetChannelState(nCic, CH_PICKUP);
				}
				break;
#pragma endregion off hook
#pragma region S_CALL_TALKING
			case S_CALL_TALKING:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCic << " S_CALL_TALKING");
					SetChannelState(nCic, CH_TALKING);
				}
				break;
#pragma endregion S_CALL_TALKING
#pragma region unkown
			default:
				{
					LOG4CPLUS_WARN(log, "Ch:" << nCic << " unknown state:" << std::hex << nNewState);
				}
				break;
#pragma endregion unkown
			}
			UpdateCircuitListCtrl(nCic);
		}
#pragma endregion E_CHG_HookState
#pragma region E_CHG_ChState
		else if (nEventCode == E_CHG_ChState)
		{
			int nCh = wParam; //wParam: number of channel output the event 
			int nNewState = (int)(lParam & 0xFFFF); //lParam:DWORD(HWORD wOldState---old state，LWORD wNewState---new state)
			int nOldState = (int)(lParam >> sizeof(WORD)*8);
			LOG4CPLUS_DEBUG(log,"Ch:" << nCh << ",SanHui nEventCode:" << GetShEventName(nEventCode) << ",NewState:" << GetShStateName(nNewState));
			if(nNewState == S_CALL_STANDBY)
			{
				StopRecording(nCh);
				SetChannelState(nCh,CH_IDLE);
			}
			else if(nNewState == S_CALL_PICKUPED)
			{
				SetChannelState(nCh, CH_PICKUP);
				//receive CallerId
				GetCallerAndCallee(nCh);
				if(ChMap[nCh].CtrlState == VOLTAGE_CTRL){
					if (StartRecording(nCh)){
						SetChannelState(nCh, CH_RECORDING);
					}
				}
			}
			else if(nNewState == S_CALL_OFFLINE)
			{
				StopRecording(nCh);
				SetChannelState(nCh,CH_OFFLINE);
			}

		}
#pragma endregion E_CHG_ChState
#pragma region E_SYS_NoSound
		else if (nEventCode == E_SYS_NoSound)
		{
			INT32 nCic = MySpyChToCic(wParam);;
			LOG4CPLUS_DEBUG(log, "Ch:" << nCic << " E_SYS_NoSound");
		}
#pragma endregion E_SYS_NoSound
#pragma region E_CHG_PcmLinkStatus
		else if (nEventCode == E_CHG_PcmLinkStatus)
		{
			LOG4CPLUS_DEBUG(log, "Pcm:" << wParam << " E_CHG_PcmLinkStatus");
		}
#pragma endregion E_CHG_PcmLinkStatus
#pragma region E_SYS_BargeIn
		else if (nEventCode == E_SYS_BargeIn && lParam == 1)//BargeIn event is detected 
		{
			LOG4CPLUS_DEBUG(log,"Ch:" << wParam << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			int nCh = wParam; //wParam: number of channel output the event 
			if(ChMap[nCh].bIgnoreLineVoltage)	//ignore voltage-detecting
			{
				if(ChMap[nCh].nState == CH_IDLE)
				{
					if((ChMap[nCh].CtrlState == VOICE_CTRL))
					{
						StartRecording(nCh);
					}
				}
			}
			else
			{
				if(ChMap[nCh].nState == CH_PICKUP)
				{
					if((ChMap[nCh].CtrlState == VOICE_CTRL))
					{
						StartRecording(nCh);
					}
				}
			}			
		}
#pragma endregion E_SYS_BargeIn
#pragma region E_SYS_NoSound
		if (nEventCode == E_SYS_NoSound)
		{
			LOG4CPLUS_DEBUG(log,"Ch:" << wParam << ",SanHui nEventCode:" << GetShEventName(nEventCode));
			int nCh = wParam; //wParam: number of channel output the event 
			if (ChMap[nCh].bIgnoreLineVoltage && ChMap[nCh].CtrlState == VOICE_CTRL)
			{
				StopRecording(nCh);
				SetChannelState(nCh, CH_IDLE);
			}
		}
#pragma endregion E_SYS_NoSound
		else
		{
			INT32 nCic = MySpyChToCic(wParam);
			LOG4CPLUS_WARN(log, "Ch:" << nCic << " unresolve Event:" << std::hex << nEventCode);
		}

	}

	return CDialogEx::WindowProc(message, wParam, lParam);
}


void CRecorderDlg::OnDestroy()
{
	CDialogEx::OnDestroy();

	// TODO: Add your message handler code here
	//Close board driver
	LOG4CPLUS_INFO(log,_T("Application exit..."));
	if(SsmCloseCti() == -1)
		LOG4CPLUS_ERROR(log,_T("Fail to call SsmCloseCti"));
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
	ChMap[nCh].szDtmf.Empty();
	ChMap[nCh].szCalleeId.Empty();
	ChMap[nCh].szCallerId.Empty();
	ChMap[nCh].szFileName.Empty();

	if(ChMap[nCh].nState != CH_RECORDING){
		return false;
	}
	LOG4CPLUS_TRACE(log,"Ch:" << nCh << " Stop recording:");
	CicState[nCic].szCallOutDtmf.Empty();
	ChMap[nCic].tEndTime = CTime::GetCurrentTime();
	ChMap[nCic].sql = "update  RecordLog set EndTime= '"+ChMap[nCic].tEndTime.Format("%Y-%m-%d %H:%M:%S") + "'";
	ChMap[nCic].sql += "  where F_Path='" + ChMap[nCic].szFileName + "'";
	m_sqlServerDB.addSql2Queue(ChMap[nCic].sql.GetBuffer());
	LOG4CPLUS_TRACE(log, "Ch:" << nCic << " addSql2Queue:" << ChMap[nCic].sql.GetBuffer());
	m_RecordingSum--;
	checkDiskSize();
	if(m_nCallFnMode == 0)				
	{
		//stop recording
		if(SpyStopRecToFile(nCh) == -1){
			LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SpyStopRecToFile"));
			return false;
		}
	}
	//Call the function with channel number as its parameter
	else
	{
		if(ChMap[nCh].wRecDirection == CALL_IN_RECORD)
		{
			if(SsmStopRecToFile(ChMap[nCh].nCallInCh) == -1){
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmStopRecToFile"));
				return false;
			}
		}
		else if(ChMap[nCh].wRecDirection == CALL_OUT_RECORD)
		{
			if(SsmStopRecToFile(ChMap[nCh].nCallOutCh) == -1){
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmStopRecToFile"));
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
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SsmStopRecToFile"));
				return false;
			}
		}
	}
	return true;
}

bool CRecorderDlg::StartRecording(unsigned long nIndex){
	TCHAR szFile[MAX_PATH];
	CString szDir = ChMap[nIndex].szFileName;
	lstrcpy(szFile,szDir.GetBuffer());
	szDir = szDir.Left(szDir.ReverseFind('\\'));
	CreateMultipleDirectory(szDir);
	LOG4CPLUS_TRACE(log, "Ch:" << nIndex << ", record file:" << szFile);
	if(m_nCallFnMode == 0)	//Call the function with circuit number as its parameter
	{
		if(SpyRecToFile(nIndex, ChMap[nIndex].wRecDirection, szFile, -1, 0L, -1, -1, 0) == -1)
			LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SpyRecToFile"));
		else{
			return true;
		}
	}
	else if(m_nCallFnMode == 1)		//Call the function with channel number as its parameter
	{
		if(ChMap[nIndex].wRecDirection == CALL_IN_RECORD)
		{
			if(SsmRecToFile(ChMap[nIndex].nCallInCh,szFile, -1, 0L, -1, -1, 0) == -1)
				LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SsmRecToFile"));
			else{
				return true;
			}
		}
		else if(ChMap[nIndex].wRecDirection == CALL_OUT_RECORD)
		{
			if(SsmRecToFile(ChMap[nIndex].nCallOutCh, szFile, -1, 0L, -1, -1, 0) == -1)
				LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SsmRecToFile"));
			else{
				return true;
			}
		}
		else
		{
			if(SsmLinkFrom(ChMap[nIndex].nCallOutCh, ChMap[nIndex].nCallInCh) == -1)  //Connect the bus from outgoing channel to incoming channel
				LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SsmLinkFrom"));

			if(SsmSetRecMixer(ChMap[nIndex].nCallInCh, TRUE, 0) == -1)		//Turn on the record mixer
				LOG4CPLUS_ERROR(log,"Ch:" << nIndex <<  _T(" Fail to call SsmSetRecMixer"));

			if(SsmRecToFile(ChMap[nIndex].nCallInCh, szFile, -1, 0L, -1, -1, 0) == -1)//Recording
				LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SsmRecToFile"));
			else{
				return true;
			}
		}
	}
	return false;
}

void CRecorderDlg::SetChannelState(unsigned long nIndex, CH_STATE newState)
{
	ChMap[nIndex].nState = newState;
	ChMap[nIndex].szState = StateName[newState];
}

void CRecorderDlg::GetCaller(unsigned long nIndex){
	if(SpyGetCallerId(nIndex, ChMap[nIndex].szCallerId.GetBuffer(20)) == -1)//Get calling party number
		LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SpyGetCallerId"));
	ChMap[nIndex].szCallerId.ReleaseBuffer();
}
void CRecorderDlg::GetCallee(unsigned long nIndex){
	if(SpyGetCalleeId(nIndex, ChMap[nIndex].szCalleeId.GetBuffer(20)) == -1)//Get called party number
		LOG4CPLUS_ERROR(log, "Ch:" << nIndex <<  _T(" Fail to call SpyGetCalleeId"));
	ChMap[nIndex].szCalleeId.ReleaseBuffer();
}
void CRecorderDlg::GetCallerAndCallee(unsigned long nIndex)
{
	GetCaller(nIndex);
	GetCallee(nIndex);
}

std::string CRecorderDlg::GetShEventName(unsigned int nEvent){
	switch(nEvent){
	case E_CHG_SpyState:	return "E_CHG_SpyState";
	case S_SPY_RCVPHONUM:	return "S_SPY_RCVPHONUM";
	case S_SPY_RINGING:		return "S_SPY_RINGING";
	case S_SPY_TALKING:		return "S_SPY_TALKING";
	case E_SYS_BargeIn:		return "E_SYS_BargeIn";
	case E_SYS_NoSound:		return "E_SYS_NoSound";
	case E_PROC_RecordEnd:	return "E_PROC_RecordEnd";
	case E_CHG_RingCount:	return "E_CHG_RingCount";
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