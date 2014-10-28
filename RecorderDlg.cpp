// DTP_Event_VCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Recorder.h"
#include "RecorderDlg.h"
#include <log4cplus/loggingmacros.h>
#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
	ChDTMF,
	ChTimes,
	ChStartTime,
	ChFileName,
};
#define  ColumnNumber (ChFileName + 1)
static LPTSTR ColumnNameCh[ColumnNumber] = {"通道号",		"通道状态",	"主叫号码",		"被叫号码",	 "DTMF",	"录音次数",		"开始时间",	 "录音文件名称"};
static LPTSTR ColumnName[ColumnNumber] =   {"Ch",			"CicState",	"CallerId",		"CalleeId",	 "DTMF",    "Times",		"StartTime",   "FileName"};
static int    ColumnWidth[ColumnNumber] =  {ChannelWidth,	StatusWidth, CallingWidth,	CalleeWidth,  DTMFWidth,RecordTimesWidth,StartTimeWidth,FileNameWidth};

static LPTSTR	StateName[] = {"空闲","收号","振铃","通话","摘机"};		
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About


/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCDlg dialog

CRecorder_Dlg::CRecorder_Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecorder_Dlg::IDD, pParent),nMaxCh(0)
{
	//{{AFX_DATA_INIT(CDTP_Event_VCDlg)
	m_nRecFormat = 2;
	m_nCallFnMode = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	this->log = log4cplus::Logger::getInstance(_T("Recorder"));
}
CRecorder_Dlg::~CRecorder_Dlg()
{
}

void CRecorder_Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDTP_Event_VCDlg)
	DDX_Control(pDX, IDC_LIST_DTP, m_ChList);
	DDX_Control(pDX, IDC_COMBO_CIC, m_cmbCh);
	DDX_Radio(pDX, IDC_RADIO_CALL_IN, m_nRecFormat);
	DDX_Radio(pDX, IDC_RADIO_CIRCUIT, m_nCallFnMode);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRecorder_Dlg, CDialog)
	//{{AFX_MSG_MAP(CDTP_Event_VCDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_RADIO_CALL_IN, OnRadioCallIn)
	ON_BN_CLICKED(IDC_RADIO_CALL_OUT, OnRadioCallOut)
	ON_BN_CLICKED(IDC_RADIO_MIX, OnRadioMix)
	ON_CBN_SELCHANGE(IDC_COMBO_CIC, OnSelchangeComboCic)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_RADIO_CIRCUIT, OnRadioCircuit)
	ON_BN_CLICKED(IDC_RADIO_CH, OnRadioCh)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCDlg message handlers

BOOL CRecorder_Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	if(!InitCtiBoard())		
	{
		PostQuitMessage(0);
		return FALSE;
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
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRecorder_Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialog::OnSysCommand(nID, lParam);
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRecorder_Dlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRecorder_Dlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//Initialize board
BOOL CRecorder_Dlg::InitCtiBoard()
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
	CString str;
	for(int i = 0; i < nMaxCh; i++)
	{
		ChMap[i].nState = CIRCUIT_IDLE;
		ChMap[i].szState = StateName[CIRCUIT_IDLE];
		ChMap[i].wRecDirection = MIX_RECORD;	    //mix-record
		ChMap[i].nCallInCh = -1;	
		ChMap[i].nCallOutCh = -1;
		ChMap[i].nRecordTimes = 0;
		ChMap[i].tStartTime = CTime::GetCurrentTime();
		str.Format("%d", i);
		m_cmbCh.InsertString(-1, str);
	}
	
	return TRUE;
}

//Initialize list
void CRecorder_Dlg::InitCircuitListCtrl()
{
	
	//m_CicList.SetBkColor(RGB(0,0,0));
	//m_CicList.SetTextColor(RGB(0,255,0));
	m_ChList.SetTextBkColor(RGB(255,255,204));
	m_ChList.SetOutlineColor(RGB(0,0,0));

	DWORD dwExtendedStyle = m_ChList.GetExtendedStyle();
	dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	dwExtendedStyle |= LVS_EX_GRIDLINES; 
	m_ChList.SetExtendedStyle(dwExtendedStyle);

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
		m_ChList.InsertItem(i, str);
	}
}

//Update list
void CRecorder_Dlg::UpdateCircuitListCtrl(unsigned int nIndex)
{
	CString strNewData;
	m_ChList.SetItemText(nIndex, ChState, ChMap[nIndex].szState);
	m_ChList.SetItemText(nIndex, ChCaller, ChMap[nIndex].szCallerId);
	m_ChList.SetItemText(nIndex, ChCallee, ChMap[nIndex].szCalleeId);
	m_ChList.SetItemText(nIndex, ChDTMF, ChMap[nIndex].szDtmf);
	//m_CicList.SetItemText(nIndex, ChOutDTMF, CicState[nIndex].szCallOutDtmf);
	strNewData.Format("%d", ChMap[nIndex].nRecordTimes);
	m_ChList.SetItemText(nIndex, ChTimes, strNewData);
	m_ChList.SetItemText(nIndex, ChStartTime , ChMap[nIndex].tStartTime.Format("%Y-%m-%d %H:%M:%S"));
	m_ChList.SetItemText(nIndex, ChFileName, ChMap[nIndex].szFileName);
	
}


LRESULT CRecorder_Dlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
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
						ChMap[nCic].szFileName.Format("d:\\test\\Rec_%d-%d-%d-%d.wav", nCic, st.wHour, st.wMinute, st.wSecond);

						if(m_nCallFnMode == 0)	//Call the function with circuit number as its parameter
						{
							if(SpyRecToFile(nCic, ChMap[nCic].wRecDirection, ChMap[nCic].szFileName.GetBuffer(), -1, 0L, -1, -1, 0) == -1)
								LOG4CPLUS_ERROR(log, "Ch:" << nCic <<  _T(" Fail to call SpyRecToFile"));
							else{
								ChMap[nCic].tStartTime = CTime::GetCurrentTime();
								ChMap[nCic].nRecordTimes++;
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

	return CDialog::WindowProc(message, wParam, lParam);
}
//Incoming call recording
void CRecorder_Dlg::OnRadioCallIn() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];

	UpdateData(TRUE);
	
	m_cmbCh.GetLBText(m_cmbCh.GetCurSel(), sz);
	nCurLine = atoi(sz);
	ChMap[nCurLine].wRecDirection = CALL_IN_RECORD;
}
//Outgoing call recording
void CRecorder_Dlg::OnRadioCallOut() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];

	UpdateData(TRUE);
	
	m_cmbCh.GetLBText(m_cmbCh.GetCurSel(), sz);
	nCurLine = atoi(sz);
	ChMap[nCurLine].wRecDirection = CALL_OUT_RECORD;	
}
//Mix-record
void CRecorder_Dlg::OnRadioMix() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];

	UpdateData(TRUE);
	
	m_cmbCh.GetLBText(m_cmbCh.GetCurSel(), sz);
	nCurLine = atoi(sz);
	ChMap[nCurLine].wRecDirection = MIX_RECORD;		
}


void CRecorder_Dlg::OnSelchangeComboCic() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];
	
	m_cmbCh.GetLBText(m_cmbCh.GetCurSel(), sz);
	nCurLine = atoi(sz);

	switch(ChMap[nCurLine].wRecDirection)
	{
	case CALL_IN_RECORD:		m_nRecFormat = 0; break;
	case CALL_OUT_RECORD:		m_nRecFormat = 1; break;
	case MIX_RECORD:			m_nRecFormat = 2; break;
	}

	UpdateData(FALSE);		
}


void CRecorder_Dlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	//Close board driver
	LOG4CPLUS_INFO(log,_T("Application exit..."));
	if(SsmCloseCti() == -1)
		LOG4CPLUS_ERROR(log,_T("Fail to call SsmCloseCti"));
}


void CRecorder_Dlg::OnRadioCircuit() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);	
}


void CRecorder_Dlg::OnRadioCh() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
}

