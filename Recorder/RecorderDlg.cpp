
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

static LPTSTR	StateName[] = {"空闲","收号","振铃","通话","摘机"};		
// CRecorderDlg 对话框




CRecorderDlg::CRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRecorderDlg::IDD, pParent),nMaxCh(0),m_freeCapacity(100),m_totalCapacity(50)
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
		ULONG dwLen;
		shKey.QueryStringValue("AppPath", szCurPath.GetBuffer(256),&dwLen);
		szCurPath.ReleaseBuffer();
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
		LOG4CPLUS_ERROR(log, CErrMsg.GetBuffer());
		CErrMsg.ReleaseBuffer();
		return FALSE;
	}

	//Judge if the number of initialized boards is the same as
	//		   that of boards specified in the configuration file
	if(SsmGetMaxUsableBoard() != SsmGetMaxCfgBoard())
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300)); //Get error message	
		LOG4CPLUS_ERROR(log, CErrMsg.GetBuffer());
		CErrMsg.ReleaseBuffer();
		return FALSE;
	}


	//Get the maximum number of the monitored circuits
	nMaxCh = SpyGetMaxCic();
	if(nMaxCh <= 0){
		nMaxCh = SsmGetMaxCh();
		if(nMaxCh == -1)
		{
			LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetMaxCic"));
		}
	}

	for(int i = 0; i < nMaxCh; i++)
	{
		ChMap[i].nState = CIRCUIT_IDLE;
		ChMap[i].szState = StateName[CIRCUIT_IDLE];
		ChMap[i].wRecDirection = MIX_RECORD;	    //mix-record
		ChMap[i].nCallInCh = -1;	
		ChMap[i].nCallOutCh = -1;
		ChMap[i].nRecordTimes = 0;
		ChMap[i].tStartTime = CTime::GetCurrentTime();
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
	if ( ChMap[nIndex].nState == CIRCUIT_TALKING)
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
	static int nCic;
	static int nCh;
	static char cNewDtmf;
	static ULONG nEventCode;
	static UINT nNewState;

	//Adopt windows message mechanism
	//	   windows message code：event code + 0x7000(WM_USER)
	if(message > WM_USER)  
	{	

		nEventCode = message - WM_USER;	
		LOG4CPLUS_DEBUG(log,"message:" << message << ",SanHui nEventCode:" << std::hex <<nEventCode);

		//Event notifying the state change of the monitored circuit
		if(nEventCode == E_CHG_SpyState)	
		{
#pragma region E_CHG_SpyState
			nCic = wParam;
			LOG4CPLUS_DEBUG(log,"Ch:" << nCic <<  " E_CHG_SpyState");
			nNewState = (int)lParam & 0xFFFF;

			if(nCic >= 0 && nCic < MAX_CH)
			{
				switch(nNewState)
				{
					//Idle state
#pragma region 空闲
				case S_SPY_STANDBY:

					{
						LOG4CPLUS_DEBUG(log, "Ch:" << nCic <<  " State:S_SPY_STANDBY");
						if(ChMap[nCic].nState == CIRCUIT_TALKING)
						{
							//Call the function with circuit number as its parameter
							if(m_nCallFnMode == 0)				
							{
								//stop recording
								if(SpyStopRecToFile(nCic) == -1)
									LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyStopRecToFile"));
							}
							//Call the function with channel number as its parameter
							else
							{
								if(ChMap[nCic].wRecDirection == CALL_IN_RECORD)
								{
									if(SsmStopRecToFile(ChMap[nCic].nCallInCh) == -1)
										LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmStopRecToFile"));
								}
								else if(ChMap[nCic].wRecDirection == CALL_OUT_RECORD)
								{
									if(SsmStopRecToFile(ChMap[nCic].nCallOutCh) == -1)
										LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmStopRecToFile"));
								}
								else
								{
									if(SsmSetRecMixer(ChMap[nCic].nCallInCh, FALSE, 0) == -1)//Turn off the record mixer
										LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmSetRecMixer"));
									if(SsmStopLinkFrom(ChMap[nCic].nCallOutCh, ChMap[nCic].nCallInCh) == -1)//Cut off the bus connect from outgoing channel to incoming channel
										LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmStopLinkFrom"));
									if(SsmStopRecToFile(ChMap[nCic].nCallInCh) == -1)		//Stop recording
										LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmStopRecToFile"));
								}
							}
						}
						ChMap[nCic].nState = CIRCUIT_IDLE;
						ChMap[nCic].szState = StateName[CIRCUIT_IDLE];
						ChMap[nCic].szDtmf.Empty();
						ChMap[nCic].szCalleeId.Empty();
						ChMap[nCic].szCallerId.Empty();
						ChMap[nCic].szFileName.Empty();
						//CicState[nCic].szCallOutDtmf.Empty();
					}
					break;
#pragma endregion 空闲
#pragma region DTMF
					//Receiving phone number
				case S_SPY_RCVPHONUM:
					{
						LOG4CPLUS_DEBUG(log,"Ch:" << nCic <<  " State:S_SPY_RCVPHONUM");
						if(ChMap[nCic].nState == CIRCUIT_IDLE)
						{
							ChMap[nCic].nState = CIRCUIT_RCV_PHONUM;
							ChMap[nCic].szState = StateName[CIRCUIT_RCV_PHONUM];
						}
					}
					break;	
#pragma endregion DTMF
#pragma region 振铃
					//Ringing
				case S_SPY_RINGING:
					{
						LOG4CPLUS_DEBUG(log,"Ch:" << nCic <<  " State:S_SPY_RINGING");
						ChMap[nCic].nState = CIRCUIT_RINGING;
						ChMap[nCic].szState = StateName[CIRCUIT_RINGING];

						if(SpyGetCallerId(nCic, ChMap[nCic].szCallerId.GetBuffer(20)) == -1)//Get calling party number
							LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyGetCallerId"));
						ChMap[nCic].szCallerId.ReleaseBuffer();
						if(SpyGetCalleeId(nCic, ChMap[nCic].szCalleeId.GetBuffer(20)) == -1)//Get called party number
							LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyGetCalleeId"));
						ChMap[nCic].szCalleeId.ReleaseBuffer();
					}
					break;
#pragma endregion 振铃
#pragma region 通话
					//Talking
				case S_SPY_TALKING:
					{
						LOG4CPLUS_DEBUG(log,"Ch:" << nCic << " State:S_SPY_TALKING");
						if(ChMap[nCic].nState == CIRCUIT_RCV_PHONUM)
						{
							if(SpyGetCallerId(nCic, ChMap[nCic].szCallerId.GetBuffer(20)) == -1)
								LOG4CPLUS_ERROR(log, "Ch:" << nCic << _T(" Fail to call SpyGetCallerId"));
							ChMap[nCic].szCallerId.ReleaseBuffer();
							if(SpyGetCalleeId(nCic, ChMap[nCic].szCalleeId.GetBuffer(20)) == -1)
								LOG4CPLUS_ERROR(log, "Ch:" << nCic << _T(" Fail to call SpyGetCalleeId"));
							ChMap[nCic].szCalleeId.ReleaseBuffer();
						}
						if((ChMap[nCic].nCallInCh = SpyGetCallInCh(nCic)) == -1)	//Get the number of incoming channel
							LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyGetCallInCh"));
						if((ChMap[nCic].nCallOutCh = SpyGetCallOutCh(nCic)) == -1)//Get the number of outgoing channel
							LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyGetCallOutCh"));

						ChMap[nCic].nState = CIRCUIT_TALKING;
						ChMap[nCic].szState = StateName[CIRCUIT_TALKING];

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
						if(ChMap[nCic].szCallerId.Compare("40012345678")){
							ChMap[nCic].wRecDirection = CALL_OUT_RECORD;
						}else{
							ChMap[nCic].wRecDirection = CALL_IN_RECORD;
						}

						if(m_nCallFnMode == 0)	//Call the function with circuit number as its parameter
						{
							if(SpyRecToFile(nCic, ChMap[nCic].wRecDirection, ChMap[nCic].szFileName.GetBuffer(), -1, 0L, -1, -1, 0) == -1)
								LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyRecToFile"));
							else{
								ChMap[nCic].tStartTime = CTime::GetCurrentTime();
								ChMap[nCic].nRecordTimes++;
								ChMap[nCic].sql = "INSERT INTO RecordLog  ( CallerNum,CalleeNum,CustomerID,StarTime,F_Path ,Flag)";
								ChMap[nCic].sql += "VALUES ( '" + ChMap[nCic].szCallerId + "','9" + ChMap[nCic].szCalleeId + "','','" + ChMap[nCic].tStartTime.Format("%Y-%m-%d %H:%M:%S.%MS") + "','" + ChMap[nCic].szFileName + "','0') ";
								m_sqlServerDB.addSql2Queue(ChMap[nCic].sql.GetBuffer());
								ChMap[nCic].sql.ReleaseBuffer();
								m_RecordingSum++;
							}
						}
						else if(m_nCallFnMode == 1)		//Call the function with channel number as its parameter
						{
							if(ChMap[nCic].wRecDirection == CALL_IN_RECORD)
							{
								if(SsmRecToFile(ChMap[nCic].nCallInCh, ChMap[nCic].szFileName.GetBuffer(), -1, 0L, -1, -1, 0) == -1)
									LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmRecToFile"));
								else{
									ChMap[nCic].tStartTime = CTime::GetCurrentTime();
									ChMap[nCic].nRecordTimes++;
								}
							}
							else if(ChMap[nCic].wRecDirection == CALL_OUT_RECORD)
							{
								if(SsmRecToFile(ChMap[nCic].nCallOutCh, ChMap[nCic].szFileName.GetBuffer(), -1, 0L, -1, -1, 0) == -1)
									LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmRecToFile"));
								else{
									ChMap[nCic].tStartTime = CTime::GetCurrentTime();
									ChMap[nCic].nRecordTimes++;
								}
							}
							else
							{
								if(SsmLinkFrom(ChMap[nCic].nCallOutCh, ChMap[nCic].nCallInCh) == -1)  //Connect the bus from outgoing channel to incoming channel
									LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmLinkFrom"));
								else{
									ChMap[nCic].tStartTime = CTime::GetCurrentTime();
									ChMap[nCic].nRecordTimes++;
								}
								if(SsmSetRecMixer(ChMap[nCic].nCallInCh, TRUE, 0) == -1)		//Turn on the record mixer
									LOG4CPLUS_ERROR(log,"Ch:" << nCic <<  _T(" Fail to call SsmSetRecMixer"));
								else{
									ChMap[nCic].tStartTime = CTime::GetCurrentTime();
									ChMap[nCic].nRecordTimes++;
								}
								if(SsmRecToFile(ChMap[nCic].nCallInCh, ChMap[nCic].szFileName.GetBuffer(), -1, 0L, -1, -1, 0) == -1)//Recording
									LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SsmRecToFile"));
								else{
									ChMap[nCic].tStartTime = CTime::GetCurrentTime();
									ChMap[nCic].nRecordTimes++;
								}
							}
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
				LOG4CPLUS_WARN(log,"nCic:"<< nCic << " Error, not exsist.");
			}
			UpdateCircuitListCtrl(nCic);
#pragma endregion E_CHG_SpyState
		}
#pragma region DTMF
		//Event generated by the driver when DTMF is received
		else if(nEventCode == E_CHG_RcvDTMF)
		{
			nCh = wParam;
			//Switching from channel number to circuit number
			if((nCic = SpyChToCic(nCh)) == -1)
			{
				LOG4CPLUS_ERROR(log, "Ch:" << nCh <<  _T(" Fail to call SpyChToCic"));
				nCic = nCh;
			}else{
				LOG4CPLUS_DEBUG(log,"nCh:" << nCh << " change to cic nCic:" << nCic);
			}


			if(nCic != -1)
			{
				cNewDtmf = (char)(0xFFFF & lParam);	//Newly received DTMF
				ChMap[nCic].szDtmf.AppendChar(cNewDtmf);
			}
			UpdateCircuitListCtrl(nCic);
		}
#pragma endregion DTMF
#pragma region Hook
		else if (nEventCode == E_CHG_HookState)
		{
			nCh = wParam;
			LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " E_CHG_HookState");
			nNewState = lParam;
			switch(nNewState)
			{
#pragma region on hook
			case S_CALL_STANDBY:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " S_CALL_STANDBY");
					ChMap[nCic].nState = CIRCUIT_IDLE;
					ChMap[nCic].szState = StateName[CIRCUIT_IDLE];
					ChMap[nCic].szDtmf.Empty();
					ChMap[nCic].szCalleeId.Empty();
					ChMap[nCic].szCallerId.Empty();
					ChMap[nCic].szFileName.Empty();
					m_RecordingSum--;

				}
				break;
#pragma endregion on hook
#pragma region off hook

			case S_CALL_PICKUPED:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " S_CALL_PICKUPED");
					ChMap[nCic].nState = STATE_PICKUP;
					ChMap[nCic].szState = StateName[STATE_PICKUP];
				}
				break;
#pragma endregion off hook
#pragma region S_CALL_TALKING
			case S_CALL_TALKING:
				{
					LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " S_CALL_TALKING");
					ChMap[nCic].nState = CIRCUIT_TALKING;
					ChMap[nCic].szState = StateName[CIRCUIT_TALKING];
				}
				break;
#pragma endregion S_CALL_TALKING
#pragma region unkown
			default:
				{
					LOG4CPLUS_WARN(log, "Ch:" << nCh << " unknown state:" << std::hex << nNewState);
				}
				break;
#pragma endregion unkown
			}
			UpdateCircuitListCtrl(nCh);
		}
#pragma endregion Hook
#pragma region E_CHG_ChState
		else if (nEventCode == E_CHG_ChState)
		{
			nCh = wParam;
			LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " E_CHG_ChState");

		}
#pragma endregion E_CHG_ChState
#pragma region E_SYS_BargeIn
		else if (nEventCode == E_SYS_BargeIn)
		{
			nCh = wParam;
			LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " E_SYS_BargeIn");
		}
#pragma endregion E_SYS_BargeIn
#pragma region E_SYS_NoSound
		else if (nEventCode == E_SYS_NoSound)
		{
			nCh = wParam;
			LOG4CPLUS_DEBUG(log, "Ch:" << nCh << " E_SYS_NoSound");
		}
#pragma endregion E_SYS_NoSound
		else
		{
			nCh = wParam;
			LOG4CPLUS_WARN(log, "Ch:" << nCh << " unresolve Event:" << std::hex << nEventCode);
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
	COLORREF freeColor = RGB(0xCC,0x33,0x99);
	COLORREF applyColor = RGB(0x00,0x33,0x99);

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
		shKey.SetStringValue(name.GetBuffer(), strValue.GetBuffer(),REG_SZ);
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
		shKey.SetDWORDValue(name.GetBuffer(), value);
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
		ULONG dwLen;
		shKey.QueryStringValue(name.GetBuffer(), strValue.GetBuffer(MAX_PATH),&dwLen);
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
		shKey.QueryDWORDValue(name.GetBuffer(), dwValue);
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
	GetDiskFreeSpaceEx(m_strFileDir,&lpuse,&lptotal,&lpfree);  

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
}
