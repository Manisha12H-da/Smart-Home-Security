#include <stdafx.h>
#include"afximpl.h"
#include "timedmsgbox.h"

/////////////////////////////////////////////////////////////////////////
//
// CDlgTimedMessageBox
//
/////////////////////////////////////////////////////////////////////////

CMapPtrToPtr		CDlgTimedMessageBox::m_mapTimerIdToClassMe;
CCriticalSection	CDlgTimedMessageBox::m_sectMap;

// the static one to call the messagebox with one line
UINT CDlgTimedMessageBox::TimedMessageBox(UINT flags, LPCTSTR ptszMessage, LPCTSTR ptszTitle,
	DWORD dwTimeout, UINT dDefaultReturn,
	LPCTSTR ptszMessageTimer, HWND hwndParent, BOOL* pbStoppedByUser)
{
	CDlgTimedMessageBox		msgBox(flags, ptszMessage, ptszTitle,
		dwTimeout, dDefaultReturn,
		ptszMessageTimer, hwndParent);

	return msgBox.ShowMessageBox(pbStoppedByUser);
}

CDlgTimedMessageBox::CDlgTimedMessageBox(UINT flags,
	LPCTSTR ptszMessage, LPCTSTR ptszTitle,
	DWORD dwTimeout, UINT dDefaultReturn,
	LPCTSTR ptszMessageTimer,
	HWND hwndParent)
{
	m_hParent = hwndParent;
	m_Message = ptszMessage;
	m_Title = ptszTitle;
	m_flags = flags;
	m_dwTimeout = dwTimeout - 1;
	m_MessageTimer = ptszMessageTimer;
	m_DefaultReturn = dDefaultReturn;

	m_hMsgBox = NULL;
	m_hStaticText = NULL;
	m_hDefaultButton = NULL;
	m_bRunning = FALSE;
	m_bStoppedByTimer = FALSE;

	if (!m_hParent)
	{
		CWnd* m_pParent = AfxGetApp()->GetMainWnd();
		m_hParent = m_pParent->m_hWnd;
	}
}

CDlgTimedMessageBox::~CDlgTimedMessageBox()
{
}

UINT CDlgTimedMessageBox::ShowMessageBox(BOOL* pbStoppedByUser)
{
	// start timer 
	CDlgTimedMessageBox::m_sectMap.Lock();
	{
		m_idTimer = ::SetTimer(NULL, 0, 1000, (TIMERPROC)CDlgTimedMessageBox::GlobalTimerProc);
		CDlgTimedMessageBox::m_mapTimerIdToClassMe.SetAt((void*)m_idTimer, this);
	}
	CDlgTimedMessageBox::m_sectMap.Unlock();

	// show MessageBox
	m_bRunning = TRUE;
	m_dwStarted = ::GetTickCount();

	m_CurrentMessage = m_Message;
	if (!m_MessageTimer.IsEmpty())
	{
		CString	second;
		second.Format(m_MessageTimer, (m_dwTimeout + 1) / 1000);
		m_CurrentMessage.Format("%s%s", m_Message, second);
	}
	UINT erg = ::MessageBox(m_hParent, m_CurrentMessage, m_Title, m_flags);
	m_bRunning = FALSE;

	CDlgTimedMessageBox::m_sectMap.Lock();
	{
		::KillTimer(NULL, m_idTimer);
		m_idTimer = 0;
		CDlgTimedMessageBox::m_mapTimerIdToClassMe.RemoveKey((void*)m_idTimer);
	}
	CDlgTimedMessageBox::m_sectMap.Unlock();

	if (pbStoppedByUser)
		*pbStoppedByUser = !m_bStoppedByTimer;

	return erg;
}

void CALLBACK CDlgTimedMessageBox::GlobalTimerProc(HWND hwnd, UINT uiMsg, UINT_PTR idEvent, DWORD dwTime)
{
	//TRACE("Global timer with id=%u\n", idEvent);

	CDlgTimedMessageBox* pMe = NULL;

	// Find the corresponding class by the timer-id
	CDlgTimedMessageBox::m_sectMap.Lock();
	{
		CDlgTimedMessageBox::m_mapTimerIdToClassMe.Lookup((void*)idEvent, (void*&)pMe);
	}
	CDlgTimedMessageBox::m_sectMap.Unlock();

	if (pMe != NULL)
		pMe->LocalTimerProc();
}

void CDlgTimedMessageBox::LocalTimerProc(void)
{
	//TRACE("Local timer with id=%u (%s)\n", m_idTimer, m_Title);

	if (!m_bRunning)
		return;

	// lookup the handles 
	GetWindowHandles();


	if (!m_hStaticText || !m_hMsgBox)
		return;

	DWORD now = GetTickCount() - m_dwStarted;

	if (now >= (m_dwTimeout))
	{
		// done with the box
		m_bStoppedByTimer = TRUE;
		::PostMessage(m_hMsgBox, WM_COMMAND, (WPARAM)m_DefaultReturn, (LPARAM)m_hDefaultButton);
	}
	else
	{
		m_CurrentMessage = m_Message;

		// not done: set text again
		if (!m_MessageTimer.IsEmpty())
		{
			CString	second;
			second.Format(m_MessageTimer, (100 + m_dwTimeout - now) / 1000);
			m_CurrentMessage.Format("%s%s", m_Message, second);
		}
		::SetWindowText(m_hStaticText, m_CurrentMessage);
	}
}

void CDlgTimedMessageBox::GetWindowHandles(void)
{
	HWND		hWnd;
	CWnd* pWnd;
	CString		title;
	CPtrList	allButtons;

	//
	// Handle of the messageBox
	//
	if (!m_hMsgBox)
	{
		hWnd = ::GetWindow(::GetDesktopWindow(), GW_CHILD);
		while ((hWnd != NULL) && (m_hMsgBox == NULL))
		{
			pWnd = CWnd::FromHandle(hWnd);
			pWnd->GetWindowText(title);

			if (AfxIsDescendant(m_hParent, hWnd) && ::IsWindowVisible(hWnd) && (m_Title.CompareNoCase(title) == 0))
			{
				m_hMsgBox = hWnd;
				break;
			}

			hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
		}
	}

	//
	// Handle of the static text
	// TODO only if text-replace is needed
	//
	if (m_hMsgBox && !m_hStaticText)
	{
		char		className[_MAX_PATH];
		CString		classNameOk("STATIC");
		LONG		id;

		hWnd = ::GetWindow(m_hMsgBox, GW_CHILD);
		while ((hWnd != NULL) && (m_hStaticText == NULL))
		{
			id = ::GetWindowLong(hWnd, GWL_ID);

			// small ids only for buttons
			if (id > IDHELP)
			{
				if (::GetClassName(hWnd, className, _MAX_PATH))
				{

					// looking only for a static 
					if (classNameOk.CompareNoCase(className) == 0)
					{
						// not check the text
						pWnd = CWnd::FromHandle(hWnd);
						pWnd->GetWindowText(title);

						if (m_CurrentMessage.CompareNoCase(title) == 0)
						{
							m_hStaticText = hWnd;
							break;
						}

					}
				}
			}
			else
			{
				allButtons.AddTail(hWnd);
			}

			hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
		}

	}

	//
	// Handle of the default button
	//
	if (m_hMsgBox && !m_hDefaultButton)
	{
		m_hDefaultButton = ::GetDlgItem(m_hMsgBox, m_DefaultReturn);

		while (allButtons.GetCount() > 0 && !m_hDefaultButton)
		{
			m_hDefaultButton = (HWND)allButtons.GetHead();
			allButtons.RemoveHead();

			if (m_hDefaultButton)
				m_DefaultReturn = ::GetWindowLong(m_hDefaultButton, GWL_ID);
		}
	}
}
