
// CUL_Test.h: Hauptheaderdatei für die PROJECT_NAME-Anwendung
//

#pragma once

#ifndef __AFXWIN_H__
	#error "'stdafx.h' vor dieser Datei für PCH einschließen"
#endif

#include "resource.h"		// Hauptsymbole

// Pfeuffer Start
#include "../CUL_Monitor/Win32_Serial.h"
#include "../SerialAsyncMFCTest/CulAsync.h"
// Pfeuffer Ende

// CCUL_TestApp:
// Siehe CUL_Test.cpp für die Implementierung dieser Klasse
//

class CCUL_TestApp : public CWinApp
{
public:
	CCUL_TestApp();

// Überschreibungen
public:
	virtual BOOL InitInstance();

// Implementierung
	DECLARE_MESSAGE_MAP()
// Pfeuffer Start
private:
	CCulAsync			m_CulAsync;
	CWin32_Serial		m_CulComPort;
	char				m_ConnectString[80];
	char				m_IniFileName[256];
// Pfeuffer Ende
};

extern CCUL_TestApp theApp;