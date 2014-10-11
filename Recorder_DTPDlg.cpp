// DTP_Event_VCDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Recorder_DTP.h"
#include "Recorder_DTPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCDlg dialog

CRecorder_DTPDlg::CRecorder_DTPDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRecorder_DTPDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDTP_Event_VCDlg)
	m_nRecFormat = 2;
	m_nCallFnMode = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRecorder_DTPDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDTP_Event_VCDlg)
	DDX_Control(pDX, IDC_LIST_DTP, m_CicList);
	DDX_Control(pDX, IDC_COMBO_CIC, m_cmbCic);
	DDX_Radio(pDX, IDC_RADIO_CALL_IN, m_nRecFormat);
	DDX_Radio(pDX, IDC_RADIO_CIRCUIT, m_nCallFnMode);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CRecorder_DTPDlg, CDialog)
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

BOOL CRecorder_DTPDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

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
		MessageBox("Fail to call SsmSetEvent");
	if(SsmSetEvent(E_CHG_SpyState, -1, TRUE, &EventSet) == -1)
		MessageBox("Fail to call SsmSetEvent when setting E_CHG_SpyState");
	InitCircuitListCtrl();		//initialize list
	UpdateCircuitListCtrl();	//update list	
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CRecorder_DTPDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRecorder_DTPDlg::OnPaint() 
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
HCURSOR CRecorder_DTPDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

//Initialize board
BOOL CRecorder_DTPDlg::InitCtiBoard()
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
	
	//load configuration file and initialize system
	if(SsmStartCti(szShConfig, szShIndex) == -1)
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300));//Get error message
		AfxMessageBox(CErrMsg, MB_OK);
		CErrMsg.ReleaseBuffer();
		return FALSE;
	}
	
	//Judge if the number of initialized boards is the same as
	//		   that of boards specified in the configuration file
	if(SsmGetMaxUsableBoard() != SsmGetMaxCfgBoard())
	{
		SsmGetLastErrMsg(CErrMsg.GetBuffer(300)); //Get error message	
		AfxMessageBox(CErrMsg, MB_OK);
		CErrMsg.ReleaseBuffer();
		return FALSE;
	}
	//Get the maximum number of the monitored circuits
	nMaxCic = SpyGetMaxCic();	  
	if(nMaxCic == -1)
	{
		MessageBox("Fail to call SpyGetMaxCic");
	}

	CString str;
	for(int i = 0; i < nMaxCic; i++)
	{
		memset(CicState[i].szCallInDtmf, 0, sizeof(char)*100);
		memset(CicState[i].szCallOutDtmf, 0, sizeof(char)*100);
		memset(CicState[i].szCallerId, 0, sizeof(char)*20);
		memset(CicState[i].szCalleeId, 0, sizeof(char)*20);
		CicState[i].nCicState = CIRCUIT_IDLE;
		CicState[i].nCallInIndex = 0;
		CicState[i].nCallOutIndex = 0;
		CicState[i].wRecDirection = MIX_RECORD;	    //mix-record
		CicState[i].nCallInCh = -1;	
		CicState[i].nCallOutCh = -1;
		str.Format("%d", i);
		m_cmbCic.InsertString(-1, str);
	}
	
	return TRUE;
}

//Initialize list
void CRecorder_DTPDlg::InitCircuitListCtrl()
{
	static int ColumnWidth[6] = {40, 100, 100, 100, 120, 120};
 	LV_COLUMN lvc;
	lvc.mask =  LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM ;

	m_CicList.SetBkColor(RGB(0,0,0));
	m_CicList.SetTextColor(RGB(0,255,0));
	m_CicList.SetTextBkColor(RGB(0,0,0));

	DWORD dwExtendedStyle = m_CicList.GetExtendedStyle();
	dwExtendedStyle |= LVS_EX_FULLROWSELECT;
	dwExtendedStyle |= LVS_EX_GRIDLINES; 
	m_CicList.SetExtendedStyle(dwExtendedStyle);

	lvc.iSubItem = 0;
	lvc.pszText = "Cic" ;
	lvc.cx = ColumnWidth[0];
	m_CicList.InsertColumn(0, &lvc);

	lvc.iSubItem = 1;
	lvc.pszText = "CicState";
	lvc.cx = ColumnWidth[1];
	m_CicList.InsertColumn(1, &lvc);

	lvc.iSubItem = 2;
	lvc.pszText = "CallerId";
	lvc.cx = ColumnWidth[2];
	m_CicList.InsertColumn(2, &lvc);
    
	lvc.iSubItem = 3;
	lvc.pszText = "CalleeId";
	lvc.cx = ColumnWidth[3];
	m_CicList.InsertColumn(3, &lvc);   

	lvc.iSubItem = 4;
	lvc.pszText = "InComingCh:DTMF";
	lvc.cx = ColumnWidth[4];
	m_CicList.InsertColumn(4, &lvc); 

	lvc.iSubItem = 5;
	lvc.pszText = "OutgoingCh:DTMF";
	lvc.cx = ColumnWidth[5];
	m_CicList.InsertColumn(5, &lvc); 

	char dig[10];
	for(int i = 0; i < nMaxCic; i++)
	{
		m_CicList.InsertItem(i, _itoa(i, dig, 10));
	}
}

//Update list
void CRecorder_DTPDlg::UpdateCircuitListCtrl()
{
	char szNewStat[100];
	char szOldStat[100];
	int nIndex;

	nIndex = 0;
	for(int i = 0; i < nMaxCic; i++)
	{
		//Display the state of the monitored circuit
		switch(CicState[i].nCicState)
		{
		case CIRCUIT_IDLE:				strcpy(szNewStat, "Idle");					  break;	
		case CIRCUIT_RCV_PHONUM:		strcpy(szNewStat, "Receiving Phone number");  break;
		case CIRCUIT_RINGING:			strcpy(szNewStat, "Ringing");				  break;
		case CIRCUIT_TALKING:			strcpy(szNewStat, "Talking");			   	  break;
		}
		m_CicList.GetItemText(nIndex, 1, szOldStat, 10);
		if(strcmp(szOldStat, szNewStat) != 0)
		{
			m_CicList.SetItemText(nIndex, 1, szNewStat);
		}
		//Display calling party number
		m_CicList.GetItemText(nIndex, 2, szOldStat, 20);
		if(strcmp(szOldStat, CicState[i].szCallerId) != 0)
		{
			m_CicList.SetItemText(nIndex, 2, CicState[i].szCallerId);
		}		
		//Display called party number
		m_CicList.GetItemText(nIndex, 3, szOldStat, 20);
		if(strcmp(szOldStat, CicState[i].szCalleeId) != 0)
		{
			m_CicList.SetItemText(nIndex, 3, CicState[i].szCalleeId);
		}
		//CDisplay incoming channel and received DTMFs in the channel
		sprintf(szNewStat, "%d:%s", CicState[i].nCallInCh, CicState[i].szCallInDtmf);
		m_CicList.GetItemText(nIndex, 4, szOldStat, 99);
		if(strcmp(szOldStat, szNewStat) != 0)
		{
			m_CicList.SetItemText(nIndex, 4, szNewStat);
		}
		//Display outgoing channel and received DTMFs in the channel
		sprintf(szNewStat, "%d:%s", CicState[i].nCallOutCh, CicState[i].szCallOutDtmf);
		m_CicList.GetItemText(nIndex, 5, szOldStat, 99);
		if(strcmp(szOldStat, szNewStat) != 0)
		{
			m_CicList.SetItemText(nIndex, 5, szNewStat);
		}

		nIndex++;		
	}
}


LRESULT CRecorder_DTPDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	// TODO: Add your specialized code here and/or call the base class
	int nCic;
	int nCh;
	char cNewDtmf;
	int nEventCode;
	int nNewState;

	//Adopt windows message mechanism
	//	   windows message code£ºevent code + 0x7000(WM_USER)
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
									MessageBox("Fail to call SpyStopRecToFile");
							}
							//Call the function with channel number as its parameter
							else
							{
								if(CicState[nCic].wRecDirection == CALL_IN_RECORD)
								{
									if(SsmStopRecToFile(CicState[nCic].nCallInCh) == -1)
										MessageBox("Fail to call SsmStopRecToFile");
								}
								else if(CicState[nCic].wRecDirection == CALL_OUT_RECORD)
								{
									if(SsmStopRecToFile(CicState[nCic].nCallOutCh) == -1)
										MessageBox("Fail to call SsmStopRecToFile");
								}
								else
								{
									if(SsmSetRecMixer(CicState[nCic].nCallInCh, FALSE, 0) == -1)//Turn off the record mixer
										MessageBox("Fail to call SsmSetRecMixer");
									if(SsmStopLinkFrom(CicState[nCic].nCallOutCh, CicState[nCic].nCallInCh) == -1)//Cut off the bus connect from outgoing channel to incoming channel
										MessageBox("Fail to call SsmStopLinkFrom");
									if(SsmStopRecToFile(CicState[nCic].nCallInCh) == -1)		//Stop recording
										MessageBox("Fail to call SsmStopRecToFile");
								}
							}
						}
						CicState[nCic].nCicState = CIRCUIT_IDLE;
						CicState[nCic].nCallInIndex = 0;
						CicState[nCic].nCallOutIndex = 0;
						memset(CicState[nCic].szCallInDtmf, 0, sizeof(char)*100);
						memset(CicState[nCic].szCallOutDtmf, 0, sizeof(char)*100);
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
						memset(CicState[nCic].szCallerId, 0, sizeof(char)*20);
						memset(CicState[nCic].szCalleeId, 0, sizeof(char)*20);
						if(SpyGetCallerId(nCic, CicState[nCic].szCallerId) == -1)//Get calling party number
							MessageBox("Fail to call SpyGetCallerId");
						if(SpyGetCalleeId(nCic, CicState[nCic].szCalleeId) == -1)//Get called party number
							MessageBox("Fail to call SpyGetCalleeId");
					}
					break;			
				//Talking
				case S_SPY_TALKING:
					{
						if(CicState[nCic].nCicState == CIRCUIT_RCV_PHONUM)
						{
							memset(CicState[nCic].szCallerId, 0, sizeof(char)*20);
							memset(CicState[nCic].szCalleeId, 0, sizeof(char)*20);
							if(SpyGetCallerId(nCic, CicState[nCic].szCallerId) == -1)
								MessageBox("Fail to call SpyGetCallerId");
							if(SpyGetCalleeId(nCic, CicState[nCic].szCalleeId) == -1)
								MessageBox("Fail to call SpyGetCalleeId");
						}
						if((CicState[nCic].nCallInCh = SpyGetCallInCh(nCic)) == -1)	//Get the number of incoming channel
							MessageBox("Fail to call SpyGetCallInCh");
						if((CicState[nCic].nCallOutCh = SpyGetCallOutCh(nCic)) == -1)//Get the number of outgoing channel
							MessageBox("Fail to call SpyGetCallOutCh");
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
								MessageBox("Fail to call SpyRecToFile");
						}
						else if(m_nCallFnMode == 1)		//Call the function with channel number as its parameter
						{
							if(CicState[nCic].wRecDirection == CALL_IN_RECORD)
							{
								if(SsmRecToFile(CicState[nCic].nCallInCh, szTemp, -1, 0L, -1, -1, 0) == -1)
									MessageBox("Fail to call SsmRecToFile");
							}
							else if(CicState[nCic].wRecDirection == CALL_OUT_RECORD)
							{
								if(SsmRecToFile(CicState[nCic].nCallOutCh, szTemp, -1, 0L, -1, -1, 0) == -1)
									MessageBox("Fail to call SsmRecToFile");
							}
							else
							{
								if(SsmLinkFrom(CicState[nCic].nCallOutCh, CicState[nCic].nCallInCh) == -1)  //Connect the bus from outgoing channel to incoming channel
									MessageBox("Fail to call SsmLinkFrom");
								if(SsmSetRecMixer(CicState[nCic].nCallInCh, TRUE, 0) == -1)		//Turn on the record mixer
									MessageBox("Fail to call SsmSetRecMixer");
								if(SsmRecToFile(CicState[nCic].nCallInCh, szTemp, -1, 0L, -1, -1, 0) == -1)//Recording
									MessageBox("Fail to call SsmRecToFile");
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
				MessageBox("Fail to call SpyChToCic");
			}
			if(nCic != -1)
			{
				if(CicState[nCic].nCicState == CIRCUIT_TALKING)
				{
					cNewDtmf = (char)(0xFFFF & lParam);	//Newly received DTMF
					if(nCh == CicState[nCic].nCallInCh)
					{
						CicState[nCic].szCallInDtmf[CicState[nCic].nCallInIndex] = cNewDtmf;
						CicState[nCic].nCallInIndex++;
					}
					else if(nCh == CicState[nCic].nCallOutCh)
					{
						CicState[nCic].szCallOutDtmf[CicState[nCic].nCallOutIndex] = cNewDtmf;
						CicState[nCic].nCallOutIndex++; 
					}
				}
			}
		}
		
		UpdateCircuitListCtrl();
	}

	return CDialog::WindowProc(message, wParam, lParam);
}
//Incoming call recording
void CRecorder_DTPDlg::OnRadioCallIn() 
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
void CRecorder_DTPDlg::OnRadioCallOut() 
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
void CRecorder_DTPDlg::OnRadioMix() 
{
	// TODO: Add your control notification handler code here
	int nCurLine;
	char sz[100];

	UpdateData(TRUE);
	
	m_cmbCic.GetLBText(m_cmbCic.GetCurSel(), sz);
	nCurLine = atoi(sz);
	CicState[nCurLine].wRecDirection = MIX_RECORD;		
}


void CRecorder_DTPDlg::OnSelchangeComboCic() 
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


void CRecorder_DTPDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	// TODO: Add your message handler code here
	//Close board driver
	if(SsmCloseCti() == -1)
		MessageBox("Fail to call SsmCloseCti");
}


void CRecorder_DTPDlg::OnRadioCircuit() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);	
}


void CRecorder_DTPDlg::OnRadioCh() 
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
}

