///////////////////////////////////////////////////
//CsystemTray的实现文件
#include "stdafx.h"
#include "CSystemTray.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSystemTray, CObject)

/////////////////////////////////////////////////
// CSystemTray construction/creation/destruction

CSystemTray::CSystemTray()
{
	memset(&m_tnd, 0, sizeof(m_tnd));
	m_bEnabled = FALSE;
	m_bHidden = FALSE;
}

CSystemTray::CSystemTray(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szToolTip, HICON icon, UINT uID)
{
	Create(pWnd, uCallbackMessage, szToolTip, icon, uID);
	m_bHidden = FALSE;
}

BOOL CSystemTray::Create(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szToolTip, HICON icon, UINT uID)
{
// this is only for Windows 95 (or higher)
	VERIFY(m_bEnabled = ( GetVersion() & 0xff ) >= 4);
	if (!m_bEnabled) return FALSE;

//Make sure Notification window is valid
	VERIFY(m_bEnabled = (pWnd && ::IsWindow(pWnd->GetSafeHwnd())));
	if (!m_bEnabled) return FALSE;

//Make sure we avoid conflict with other messages
	ASSERT(uCallbackMessage >= WM_USER);

//Tray only supports tooltip text up to 64 characters
	ASSERT(_tcslen(szToolTip) <= 64);

// load up the NOTIFYICONDATA structure
	m_tnd.cbSize = sizeof(NOTIFYICONDATA);
	m_tnd.hWnd = pWnd->GetSafeHwnd();
	m_tnd.uID = uID;
	m_tnd.hIcon = icon;
	m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	m_tnd.uCallbackMessage = uCallbackMessage;
	strcpy (m_tnd.szTip, szToolTip);

// Set the tray icon
	m_Wnd = pWnd;
	VERIFY(m_bEnabled = Shell_NotifyIcon(NIM_ADD, &m_tnd));
	return m_bEnabled;
}

CSystemTray::~CSystemTray()
{
	RemoveIcon();
}


/////////////////////////////////////////////
// CSystemTray icon manipulation

void CSystemTray::MoveToRight()
{
	HideIcon();
	ShowIcon();
}

void CSystemTray::RemoveIcon()
{
	if(!m_bEnabled) return;
	m_tnd.uFlags = 0;
	Shell_NotifyIcon(NIM_DELETE, &m_tnd);
	m_bEnabled = FALSE;
}

void CSystemTray::HideIcon()
{
	if(m_bEnabled && !m_bHidden) {
		m_tnd.uFlags = NIF_ICON;
		Shell_NotifyIcon (NIM_DELETE, &m_tnd);
		m_bHidden = TRUE;
	}
}

void CSystemTray::ShowIcon()
{
	if (m_bEnabled && m_bHidden) {
		m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
		Shell_NotifyIcon(NIM_ADD, &m_tnd);
		m_bHidden = FALSE;
	}
}

BOOL CSystemTray::SetIcon(HICON hIcon)
{
	if (!m_bEnabled) return FALSE;

	m_tnd.uFlags = NIF_ICON;
	m_tnd.hIcon = hIcon;

	return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CSystemTray::SetIcon(LPCTSTR lpszIconName)
{
	HICON hIcon = AfxGetApp()->LoadIcon(lpszIconName);
	return SetIcon(hIcon);
}

BOOL CSystemTray::SetIcon(UINT nIDResource)
{
	HICON hIcon = AfxGetApp()->LoadIcon(nIDResource);
	return SetIcon(hIcon);
}

BOOL CSystemTray::SetStandardIcon(LPCTSTR lpIconName)
{
	HICON hIcon = LoadIcon(NULL, lpIconName);
	return SetIcon(hIcon);
}

BOOL CSystemTray::SetStandardIcon(UINT nIDResource)
{
	HICON hIcon = ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(nIDResource));
	return SetIcon(hIcon);
}

HICON CSystemTray::GetIcon() const
{
	HICON hIcon = NULL;
	if(m_bEnabled)
		hIcon = m_tnd.hIcon;

	return hIcon;
}

//////////////////////////////////////////////////
// CSystemTray tooltip text manipulation

BOOL CSystemTray::SetTooltipText(LPCTSTR pszTip)
{
	if (!m_bEnabled) return FALSE;

	m_tnd.uFlags = NIF_TIP;
	_tcscpy(m_tnd.szTip, pszTip);
	return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

BOOL CSystemTray::SetTooltipText(UINT nID)
{
	CString strText;
	VERIFY(strText.LoadString(nID));
	return SetTooltipText(strText);
}

CString CSystemTray::GetTooltipText() const
{
	CString strText;
	if (m_bEnabled)
	strText = m_tnd.szTip;
	return strText;
}

////////////////////////////////////////////////
// CSystemTray notification window stuff

BOOL CSystemTray::SetNotificationWnd(CWnd* pWnd)
{
	if (!m_bEnabled) return FALSE;

	//Make sure Notification window is valid
	ASSERT(pWnd && ::IsWindow(pWnd->GetSafeHwnd()));

	m_tnd.hWnd = pWnd->GetSafeHwnd();
	m_tnd.uFlags = 0;

	return Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}

CWnd* CSystemTray::GetNotificationWnd() const
{
	return CWnd::FromHandle(m_tnd.hWnd);
}