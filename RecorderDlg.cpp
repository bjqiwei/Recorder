// DTP_Event_VCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Recorder.h"
#include "RecorderDlg.h"
#include <log4cplus/loggingmacros.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define  ColumnNumber 9
#define  ChannelWidth 60
#define  ChannleStatusWidth 100
#define	 CallingWidth	100
#define  CalleeWidth	100
#define  IncomingDTMFWidth 120
#define  OutDTMFWidth	120
#define  RecordTimesWidth	100
#define	 StartTimeWidth		100
#define  FileNameWidth		400

static LPTSTR ColumnNameCh[ColumnNumber] = {"通道号",		"通道状态",			"主叫号码",		"被叫号码",	 "呼入DTMF",	   "外呼DTMF",		 "录音次数",	  "开始时间",	 "录音文件名称"};
static LPTSTR ColumnName[ColumnNumber] =   {"Cic",			"CicState",			"CallerId",		"CalleeId",	 "InComingCh:DTMF","OutgoingCh:DTMF","Times",		  "StartTime",   "FileName"};
static int    ColumnWidth[ColumnNumber] =  {ChannelWidth,	ChannleStatusWidth, CallingWidth,	CalleeWidth, IncomingDTMFWidth, OutDTMFWidth,	 RecordTimesWidth,StartTimeWidth,FileNameWidth};

static LPTSTR	StateName[] = {"Idle","Receiving Phone number","Ringing","Talking",};		
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About


/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCDlg dialog

CRecorder_Dlg::CRecorder_Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecorder_Dlg::IDD, pParent)
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
	DDX_Control(pDX, IDC_LIST_DTP, m_CicList);
	DDX_Control(pDX, IDC_COMBO_CIC, m_cmbCic);
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
	char szCurPath[MAX_PATH];		//Current path
	char szShIndex[MAX_PATH];		//path to ShIndex.ini
	char szShConfig[MAX_PATH];		//path to ShConfig.ini
	CString CErrMsg;			    //error message
	GetCurrentDirectory(MAX_PATH, szCurPath);
	strcpy(szShIndex, szCurPath);
	strcpy(szShConfig, szCurPath);
	strcat(szShIndex, "\\ShIndex.ini");
	strcat(szShConfig, "\\ShConfig.ini");
	
	LOG4CPLUS_INFO(log,_T("Application start..."));
	//load configuration file and initialize system
	if(SsmStartCti(szShConfig, szShIndex) == -1)
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300));//Get error message
		LOG4CPLUS_ERROR(log, (LPCSTR)CErrMsg);
		CErrMsg.ReleaseBuffer();
		return FALSE;
	}
	
	//Judge if the number of initialized boards is the same as
	//		   that of boards specified in the configuration file
	if(SsmGetMaxUsableBoard() != SsmGetMaxCfgBoard())
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300)); //Get error message	
		LOG4CPLUS_ERROR(log, (LPCSTR)CErrMsg);
		CErrMsg.ReleaseBuffer();
		return FALSE;
	}
	//Get the maximum number of the monitored circuits
	//nMaxCic = SpyGetMaxCic();
	nMaxCh = SsmGetMaxCh();
	if(nMaxCh == -1)
	{
		LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetMaxCic"));
	}

	CString str;
	for(int i = 0; i < nMaxCh; i++)
	{
		CicState[i].nCicState = CIRCUIT_IDLE;
		CicState[i].wRecDirection = MIX_RECORD;	    //mix-record
		CicState[i].nCallInCh = -1;	
		CicState[i].nCallOutCh = -1;
		CicState[i].nRecordTimes = 0;
		str.Format("%d", i);
		m_cmbCic.InsertString(-1, str);
	}
	
	return TRUE;
}

//Initialize list
void CRecorder_Dlg::InitCircuitListCtrl()
{
	
	/*m_CicList.SetBkColor(RGB(0,0,0));
	m_CicList.SetTextColor(RGB(0,255,0));
	m_CicList.SetTextBkColor(RGB(0,0,0));*/

	DWORD dwExtendedStyle = m_CicList.GetExtendedStyle();
	dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	dwExtendedStyle |= LVS_EX_GRIDLINES; 
	m_CicList.SetExtendedStyle(dwExtendedStyle);

	LV_COLUMN lvc;
	lvc.mask =  LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;

	for (int i = 0; i< ColumnNumber; i++)
	{
		lvc.iSubItem = i;
		lvc.pszText = ColumnNameCh[i];
		lvc.cx = ColumnWidth[i];
		m_CicList.InsertColumn(i, &lvc);
	}
	
	CString str;
	for(int i = 0; i < nMaxCh; i++)
	{
		str.Format("%d",i);
		m_CicList.InsertItem(i, str);
	}
}

//Update list
void CRecorder_Dlg::UpdateCircuitListCtrl()
{
	CString strNewData;
	CString strOldData;

	for(int nItem = 0; nItem < m_CicList.GetItemCount(); nItem++)
	{
		//Display the state of the monitored circuit
		
		strNewData = StateName[CicState[nItem].nCicState];
		strOldData = m_CicList.GetItemText(nItem, 1);
		if(strOldData != strNewData)
		{
			m_CicList.SetItemText(nItem, 1, strNewData);
		}
		
		//Display calling party number
		strOldData = m_CicList.GetItemText(nItem, 2);
		if(strOldData != CicState[nItem].szCallerId)
		{
			m_CicList.SetItemText(nItem, 2, CicState[nItem].szCallerId);
		}		
		//Display called party number
		strOldData = m_CicList.GetItemText(nItem, 3);
		if(strOldData != CicState[nItem].szCalleeId)
		{
			m_CicList.SetItemText(nItem, 3, CicState[nItem].szCalleeId);
		}
		//CDisplay incoming channel and received DTMFs in the channel
		strNewData.Format("%d:%s", CicState[nItem].nCallInCh, CicState[nItem].szCallInDtmf);
		strOldData = m_CicList.GetItemText(nItem, 4);
		if(strOldData != strNewData)
		{
			m_CicList.SetItemText(nItem, 4, strNewData);
		}
		//Display outgoing channel and received DTMFs in the channel
		strNewData.Format("%d:%s", CicState[nItem].nCallOutCh, CicState[nItem].szCallOutDtmf);
		strOldData = m_CicList.GetItemText(nItem, 5);
		if(strOldData != strNewData)
		{
			m_CicList.SetItemText(nItem, 5, strNewData);
		}
	
		strNewData.Format("%d", CicState[nItem].nRecordTimes);
		strOldData = m_CicList.GetItemText(nItem, 6);
		if(strOldData != strNewData)
		{
			m_CicList.SetItemText(nItem, 6, strNewData);
		}

		if (CicState[nItem].nCicState == CIRCUIT_TALKING)
		{
			m_CicList.SetItemText(nItem, 7, CicState[nItem].tStartTime.Format("YYYY-MM-DD HH:MS:SS"));
		}

		if (CicState[nItem].nCicState == CIRCUIT_TALKING)
		{
			m_CicList.SetItemText(nItem, 8, CicState[nItem].szFileName);
		}
	}
}


LRESULT CRecorder_Dlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	int nCic;
	int nCh;
	char cNewDtmf;
	int nEventCode;
	int nNewState;

	//Adopt windows message mechanism
	//	   windows message code：event code + 0x7000(WM_USER)
	if(message > WM_USER)  
	{		
		nEventCode = message - WM_USER;	

		//Event notifying the state change of the monitored circuit
		if(nEventCode == E_CHG_SpyState)	
		{
			nCic = wParam;
	 		nNewState = (int)lParam & 0xFFFF;
			if(nCic >= 0 && nCic < MAX_CIC)
			{
				switch(nNewState)
				{
				//Idle state
				case S_SPY_STANDBY:
					{
						if(CicState[nCic].nCicState == CIRCUIT_TALKING)
						{	
							//Call the function with circuit number as its parameter
							if(m_nCallFnMode == 0)				
							{
								//stop recording
								if(SpyStopRecToFile(nCic) == -1)
									LOG4CPLUS_ERROR(log, _T("Fail to call SpyStopRecToFile"));
							}
							//Call the function with channel number as its parameter
							else
							{
								if(CicState[nCic].wRecDirection == CALL_IN_RECORD)
								{
									if(SsmStopRecToFile(CicState[nCic].nCallInCh) == -1)
										LOG4CPLUS_ERROR(log, _T("Fail to call SsmStopRecToFile"));
								}
								else if(CicState[nCic].wRecDirection == CALL_OUT_RECORD)
								{
									if(SsmStopRecToFile(CicState[nCic].nCallOutCh) == -1)
										LOG4CPLUS_ERROR(log,_T("Fail to call SsmStopRecToFile"));
								}
								else
								{
									if(SsmSetRecMixer(CicState[nCic].nCallInCh, FALSE, 0) == -1)//Turn off the record mixer
										LOG4CPLUS_ERROR(log, _T("Fail to call SsmSetRecMixer"));
									if(SsmStopLinkFrom(CicState[nCic].nCallOutCh, CicState[nCic].nCallInCh) == -1)//Cut off the bus connect from outgoing channel to incoming channel
										LOG4CPLUS_ERROR(log, _T("Fail to call SsmStopLinkFrom"));
									if(SsmStopRecToFile(CicState[nCic].nCallInCh) == -1)		//Stop recording
										LOG4CPLUS_ERROR(log, _T("Fail to call SsmStopRecToFile"));
								}
							}
						}
						CicState[nCic].nCicState = CIRCUIT_IDLE;
						CicState[nCic].szCallInDtmf.Empty();
						CicState[nCic].szCallOutDtmf.Empty();
					}
					break;
				//Receiving phone number
				case S_SPY_RCVPHONUM:
					{
						if(CicState[nCic].nCicState == CIRCUIT_IDLE)
						{
							CicState[nCic].nCicState = CIRCUIT_RCV_PHONUM;
						}
					}
					break;			
				//Ringing
				case S_SPY_RINGING:
					{
						CicState[nCic].nCicState = CIRCUIT_RINGING;
						
						if(SpyGetCallerId(nCic, CicState[nCic].szCallerId.GetBuffer(20)) == -1)//Get calling party number
							LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetCallerId"));
						CicState[nCic].szCallerId.ReleaseBuffer();
						if(SpyGetCalleeId(nCic, CicState[nCic].szCalleeId.GetBuffer(20)) == -1)//Get called party number
							LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetCalleeId"));
						CicState[nCic].szCalleeId.ReleaseBuffer();
					}
					break;			
				//Talking
				case S_SPY_TALKING:
					{
						if(CicState[nCic].nCicState == CIRCUIT_RCV_PHONUM)
						{
							if(SpyGetCallerId(nCic, CicState[nCic].szCallerId.GetBuffer(20)) == -1)
								LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetCallerId"));
							if(SpyGetCalleeId(nCic, CicState[nCic].szCalleeId.GetBuffer(20)) == -1)
								LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetCalleeId"));
						}
						if((CicState[nCic].nCallInCh = SpyGetCallInCh(nCic)) == -1)	//Get the number of incoming channel
							LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetCallInCh"));
						if((CicState[nCic].nCallOutCh = SpyGetCallOutCh(nCic)) == -1)//Get the number of outgoing channel
							LOG4CPLUS_ERROR(log, _T("Fail to call SpyGetCallOutCh"));
						CicState[nCic].nCicState = CIRCUIT_TALKING;

						//Start recording
						//Record file name + Monitored circuit number + Time(hour-minute-second)
						CString strRecFile;
						SYSTEMTIME st;
						char szTemp[50];
						GetLocalTime(&st);
						strRecFile.Format("Rec%d-%d-%d-%d.wav", nCic, st.wHour, st.wMinute, st.wSecond);
						strcpy(szTemp, strRecFile.GetBuffer(strRecFile.GetLength()));
						strRecFile.ReleaseBuffer();
						if(m_nCallFnMode == 0)	//Call the function with circuit number as its parameter
						{
							if(SpyRecToFile(nCic, CicState[nCic].wRecDirection, szTemp, -1, 0L, -1, -1, 0) == -1)
								LOG4CPLUS_ERROR(log,_T("Fail to call SpyRecToFile"));
						}
						else if(m_nCallFnMode == 1)		//Call the function with channel number as its parameter
						{
							if(CicState[nCic].wRecDirection == CALL_IN_RECORD)
							{
								if(SsmRecToFile(CicState[nCic].nCallInCh, szTemp, -1, 0L, -1, -1, 0) == -1)
									LOG4CPLUS_ERROR(log,_T("Fail to call SsmRecToFile"));
							}
							else if(CicState[nCic].wRecDirection == CALL_OUT_RECORD)
							{
								if(SsmRecToFile(CicState[nCic].nCallOutCh, szTemp, -1, 0L, -1, -1, 0) == -1)
									LOG4CPLUS_ERROR(log, _T("Fail to call SsmRecToFile"));
							}
							else
							{
								if(SsmLinkFrom(CicState[nCic].nCallOutCh, CicState[nCic].nCallInCh) == -1)  //Connect the bus from outgoing channel to incoming channel
									LOG4CPLUS_ERROR(log,_T("Fail to call SsmLinkFrom"));
								if(SsmSetRecMixer(CicState[nCic].nCallInCh, TRUE, 0) == -1)		//Turn on the record mixer
									LOG4CPLUS_ERROR(log,_T("Fail to call SsmSetRecMixer"));
								if(SsmRecToFile(CicState[nCic].nCallInCh, szTemp, -1, 0L, -1, -1, 0) == -1)//Recording
									LOG4CPLUS_ERROR(log,_T("Fail to call SsmRecToFile"));
							}
						}
					}
					break;
				default:
					break;
				}
			}		
		}
		//Event generated by the driver when DTMF is received
		else if(nEventCode == E_CHG_RcvDTMF)
		{
			nCh = wParam;
			//Switching from channel number to circuit number
 			if((nCic = SpyChToCic(nCh)) == -1)
			{
				LOG4CPLUS_ERROR(log,_T("Fail to call SpyChToCic"));
			}
			if(nCic != -1)
			{
				if(CicState[nCic].nCicState == CIRCUIT_TALKING)
				{
					cNewDtmf = (char)(0xFFFF & lParam);	//Newly received DTMF
					if(nCh == CicState[nCic].nCallInCh)
					{
						CicState[nCic].szCallInDtmf += cNewDtmf;
					}
					else if(nCh == CicState[nCic].nCallOutCh)
					{
						CicState[nCic].szCallOutDtmf += cNewDtmf; 
					}
				}
			}
		}
		
		UpdateCircuitListCtrl();
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
	
	m_cmbCic.GetLBText(m_cmbCic.GetCurSel(), sz);
	nCurLine = atoi(sz);
	CicState[nCurLine].wRecDirection = CALL_IN_RECORD;
}
//Outgoing call recording
void CRecorder_Dlg::OnRadioCallOut() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];

	UpdateData(TRUE);
	
	m_cmbCic.GetLBText(m_cmbCic.GetCurSel(), sz);
	nCurLine = atoi(sz);
	CicState[nCurLine].wRecDirection = CALL_OUT_RECORD;	
}
//Mix-record
void CRecorder_Dlg::OnRadioMix() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];

	UpdateData(TRUE);
	
	m_cmbCic.GetLBText(m_cmbCic.GetCurSel(), sz);
	nCurLine = atoi(sz);
	CicState[nCurLine].wRecDirection = MIX_RECORD;		
}


void CRecorder_Dlg::OnSelchangeComboCic() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];
	
	m_cmbCic.GetLBText(m_cmbCic.GetCurSel(), sz);
	nCurLine = atoi(sz);

	switch(CicState[nCurLine].wRecDirection)
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

