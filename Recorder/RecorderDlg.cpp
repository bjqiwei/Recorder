
// RecorderDlg.cpp : ʵ���ļ�
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
	ChDTMF,
	ChTimes,
	ChStartTime,
	ChFileName,
};
#define  ColumnNumber (ChFileName + 1)
static LPTSTR ColumnNameCh[ColumnNumber] = {"ͨ����",		"ͨ��״̬",	"���к���",		"���к���",	 "DTMF",	"¼������",		"��ʼʱ��",	 "¼���ļ�����"};
static LPTSTR ColumnName[ColumnNumber] =   {"Ch",			"CicState",	"CallerId",		"CalleeId",	 "DTMF",    "Times",		"StartTime",   "FileName"};
static int    ColumnWidth[ColumnNumber] =  {ChannelWidth,	StatusWidth, CallingWidth,	CalleeWidth,  DTMFWidth,RecordTimesWidth,StartTimeWidth,FileNameWidth};

static LPTSTR	StateName[] = {"����","�պ�","����","ͨ��","ժ��"};		
// CRecorderDlg �Ի���




CRecorderDlg::CRecorderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRecorderDlg::IDD, pParent),nMaxCh(0),freeCapacity(100),applyCapacity(50)
	, m_strFileDir(_T(""))
	, m_strDataBase(_T(""))
	, m_KeepDays(0)
{
	//m_nRecFormat = 2;
	m_nCallFnMode = 0;

	this->log = log4cplus::Logger::getInstance(_T("Recorder"));
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRecorderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DTP, m_ChList);
	DDX_Control(pDX, IDC_RICHEDIT21, m_ctrCapacityView);
	DDX_Text(pDX, IDC_EDIT1, m_strFileDir);
	DDX_Text(pDX, IDC_EDIT_DATABASE, m_strDataBase);
	DDX_Text(pDX, IDC_EDIT2, m_KeepDays);
}

BEGIN_MESSAGE_MAP(CRecorderDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON1, &CRecorderDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CRecorderDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CRecorderDlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CRecorderDlg ��Ϣ��������

BOOL CRecorderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ����Ӷ���ĳ�ʼ������
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
	m_KeepDays = ReadRegKeyDWORD("KeepDays");
	UpdateData(FALSE);
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի���������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CRecorderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		DrawCapacityView();
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
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


LRESULT CRecorderDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	// TODO: Add your specialized code here and/or call the base class
	static int nCic;
	static int nCh;
	static char cNewDtmf;
	static ULONG nEventCode;
	static UINT nNewState;

	//Adopt windows message mechanism
	//	   windows message code��event code + 0x7000(WM_USER)
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
#pragma region ����
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
#pragma endregion ����
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
#pragma region ����
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
#pragma endregion ����
#pragma region ͨ��
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
						ChMap[nCic].szFileName.Format("%s\\Rec_%d-%d-%d-%d.wav", m_strFileDir, nCic, st.wHour, st.wMinute, st.wSecond);

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
#pragma endregion ͨ��
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
	pen.CreatePen(PS_NULL,0,penColor);
	pOldPen = dc->SelectObject(&pen);  

	brush.CreateSolidBrush(applyColor);
	pOldBrush = dc->SelectObject(&brush);
	unsigned long sum = applyCapacity + freeCapacity;
	sum = max(sum,1);
	unsigned int nXRadial1 = radius*2;
	unsigned int nYRadial1 = radius;

	unsigned int nXRadial2 = radius + radius * cos(applyCapacity*360/sum*pi/180);
	unsigned int nYRadial2 = radius - radius * sin(applyCapacity*360/sum*pi/180);

	dc->Pie(0, 0, radius*2, radius*2, nXRadial1, nYRadial1, nXRadial2, nYRadial2); 
	nXRadial1 = nXRadial2;
	nYRadial1 = nYRadial2;
	nXRadial2 = radius * 2;
	nYRadial2 = radius;

	brush.DeleteObject();
	brush.CreateSolidBrush(freeColor);
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
	bi.lpszTitle = "��ѡ��Ŀ¼";
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