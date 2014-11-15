#pragma once
#include <afx.h>
#include <shellapi.h>

/////////////////////////////////////////////////////////////////////////////
// CSystemTray window

class CSystemTray : public CObject
{
// Construction/destruction
public:
	CSystemTray();
	CSystemTray(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID);
	virtual ~CSystemTray();

// Operations
public:
	CWnd * m_Wnd;
	BOOL Enabled() { return m_bEnabled; }
	BOOL Visible() { return !m_bHidden; }

//Create the tray icon
	BOOL Create(CWnd* pWnd, UINT uCallbackMessage, LPCTSTR szTip, HICON icon, UINT uID);

//Change or retrieve the Tooltip text
	BOOL SetTooltipText(LPCTSTR pszTooltipText);
	BOOL SetTooltipText(UINT nID);
	CString GetTooltipText() const;

//Change or retrieve the icon displayed
	BOOL SetIcon(HICON hIcon);
	BOOL SetIcon(LPCTSTR lpIconName);
	BOOL SetIcon(UINT nIDResource);
	BOOL SetStandardIcon(LPCTSTR lpIconName);
	BOOL SetStandardIcon(UINT nIDResource);
	HICON GetIcon() const;
	void HideIcon();
	void ShowIcon();
	void RemoveIcon();
	void MoveToRight();

//Change or retrieve the window to send notification messages to
	BOOL SetNotificationWnd(CWnd* pNotifyWnd);
	CWnd* GetNotificationWnd() const;

//Default handler for tray notification message

// Overrides
// ClassWizard generated virtual function overrides
//{{AFX_VIRTUAL(CSystemTray)
//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL m_bEnabled; // does O/S support tray icon?
	BOOL m_bHidden; // Has the icon been hidden?
	NOTIFYICONDATA m_tnd;

DECLARE_DYNAMIC(CSystemTray)
};
