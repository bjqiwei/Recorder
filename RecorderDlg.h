// DTP_Event_VCDlg.h : header file
//

#if !defined(AFX_DTP_EVENT_VCDLG_H__B3DCE24A_E157_4D3C_B711_FDB6CA134CD4__INCLUDED_)
#define AFX_DTP_EVENT_VCDLG_H__B3DCE24A_E157_4D3C_B711_FDB6CA134CD4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "C:\ShCti\api\Vc6.0\Inc\shpa3api.h"
#include <string>
#include <log4cplus/logger.h>
/////////////////////////////////////////////////////////////////////////////
// CDTP_Event_VCDlg dialog
#define MAX_CIC 1000		//Maximum number of the monitored circuits
//User-defined circuit status
enum CIRCUIT_STATE
{
	CIRCUIT_IDLE,			//Idle state
	CIRCUIT_RCV_PHONUM,		//State of receiving phone number
	CIRCUIT_RINGING,		//State of ringing
	CIRCUIT_TALKING			//State of talking
};


enum RECORD_DIRECTION
{
	CALL_IN_RECORD,		//Incoming call recording
	CALL_OUT_RECORD,	//Outgoing call recording
	MIX_RECORD			//Mix-record of incoming/outgoing call 
};
	

typedef struct tagCIC_STRUCT
{
	int  nCicState;				//State of monitored circuits
	CString szCallerId;		//Calling party number
	CString szCalleeId;		//Called party number
	CString szCallInDtmf;		//DTMF received on the incoming channel
	CString szCallOutDtmf;	//DTMF received on the outgoing channel
	WORD wRecDirection;			//Recording direction
	int  nCallInCh;				//Incoming channel
	int  nCallOutCh;			//Outgoing channel
	unsigned int nRecordTimes;  //Record Times 
	CTime tStartTime;// record start time
	CString szFileName; // record file name
}CIC_STRUCT;


class CRecorder_Dlg : public CDialog
{
// Construction
public:
	CRecorder_Dlg(CWnd* pParent = NULL);	// standard constructor
	~CRecorder_Dlg();

// Dialog Data
	//{{AFX_DATA(CDTP_Event_VCDlg)
	enum { IDD = IDD_RECORDER_DTP_VC_DIALOG };
	CListCtrl	m_CicList;
	CComboBox	m_cmbCic;
	int		m_nRecFormat;
	int		m_nCallFnMode;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDTP_Event_VCDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;
	log4cplus::Logger log;
	// Generated message map functions
	//{{AFX_MSG(CDTP_Event_VCDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnRadioCallIn();
	afx_msg void OnRadioCallOut();
	afx_msg void OnRadioMix();
	afx_msg void OnSelchangeComboCic();
	afx_msg void OnDestroy();
	afx_msg void OnRadioCircuit();
	afx_msg void OnRadioCh();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	CIC_STRUCT CicState[MAX_CIC];	//Monitored circuits
	int nMaxCh;					//Maximum number of the monitored circuits
	BOOL InitCtiBoard();			//Initialize board
	void InitCircuitListCtrl();		//Initialize list
	void UpdateCircuitListCtrl();	//Update list
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DTP_EVENT_VCDLG_H__B3DCE24A_E157_4D3C_B711_FDB6CA134CD4__INCLUDED_)