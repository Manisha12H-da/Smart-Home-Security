
// CUL_TestDlg.cpp: Implementierungsdatei
//
#include "stdafx.h"
#include "CUL_Test.h"
#include "CUL_TestDlg.h"
#include "afxdialogex.h"
#include "timedmsgbox.h"
#include "string"

#pragma warning( disable : 4996 )
#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CCUL_TestDlg-Dialogfeld
const unsigned char acDefaultKey[] 	= {0xA4, 0xE3, 0x75, 0xC6, 0xB0, 0x9F, 0xD1, 0x85, 0xF2, 0x7C, 0x4E, 0x96, 0xFC, 0x27, 0x3A, 0xE4 };
const unsigned char	nControlByte	= 0xa6;
const unsigned char	nMessageType = 0x40;
static gcry_cipher_hd_t hCipher = 0;
char ctestbuffer[50] = { 0 };
BOOL scanStart = FALSE;


CCUL_TestDlg::CCUL_TestDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CCUL_TestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_nAufrufZaehler = 0;
	InitializeCriticalSectionAndSpinCount(&m_LockZaehler, 0);

	m_pCulAsync = NULL;											// Zeiger auf das CUL-Objekt
//	m_TitleFont;												// Font für die Überschrift (wird nicht initialisiert)

	m_nEditMsgCount = 0;										// Message Zähler
	m_nEditCh1Count = 0;										// Zähler für Nachrichten Kanal 1
	m_nEditCh2Count = 0;										// Zähler für Nachrichten Kanal 1
	m_sEditAes = _T("");										// String des Edit-Control-Feldes
	m_bAesEnable = false;										// Checkbox "Sicherheit Ein"
	memset(m_acConnectString, 0, sizeof(m_acConnectString));	// Speicher für den Text "Verbunden mit ..."
	m_IniFileName = NULL;										// Zeiger auf einen den Dateinanmen des ini-Files
	m_nSenderAddress = 0;										// Sender-Adresse
	m_nDestinationAddress = 0;									// Ziel-Adresse
	m_hCipher = NULL;											// Handle für gcrypt 


	memset(m_acAesKey, 0, sizeof(m_acAesKey));					// Speicher für den aktuellen AES-Schlüssel
	memset(m_ac_m_Frame, 0, sizeof(m_ac_m_Frame));				// m-Frame:	Message Puffer
	memset(m_ac_c_Frame, 0, sizeof(m_ac_c_Frame));				// c-Frame:	Challenge Message Puffer
	memset(m_ac_r_Frame, 0, sizeof(m_ac_r_Frame));				// r-Frame:	Respone Message Puffer
	memset(m_ac_a_Frame, 0, sizeof(m_ac_a_Frame));				// a-Frame: ACK Message Puffer
	memset(m_acTimestamp, 0, sizeof(m_acTimestamp));			// Zufallszahl für das Response-Paket
	m_cAktuelleMessageNummer = 0;								// Nummer des gesendetes Paketes wird für den Empfangsfilter gebraucht
}

// Destruktor
CCUL_TestDlg::~CCUL_TestDlg()
{
	_CloseGcrypt();
	DeleteCriticalSection(&m_LockZaehler);
}

void CCUL_TestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_AES, m_bAesEnable);
	DDX_Text(pDX, IDC_EDIT_MSG_COUNT, m_nEditMsgCount);
	DDV_MinMaxInt(pDX, m_nEditMsgCount, 0, 255);
	DDX_Text(pDX, IDC_EDIT_CH1_COUNT, m_nEditCh1Count);
	DDV_MinMaxInt(pDX, m_nEditCh1Count, 0, 255);
	DDX_Text(pDX, IDC_EDIT_CH2_COUNT, m_nEditCh2Count);
	DDV_MinMaxInt(pDX, m_nEditCh2Count, 0, 255);
	DDX_Text(pDX, IDC_EDIT_AES, m_sEditAes);
	DDV_MaxChars(pDX, m_sEditAes, 50);
	DDX_Control(pDX, IDC_LIST_SENDER, m_ListBoxSender);
	DDX_Control(pDX, IDC_LIST_DESTINATION, m_ListBoxDestination);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_StaticMessage);
	DDX_Control(pDX, IDC_STATIC_CHALLENGE, m_StaticChallenge);
	DDX_Control(pDX, IDC_STATIC_RRESPONSE, m_StaticResponse);
	DDX_Control(pDX, IDC_STATIC_ACK, m_StaticAck);
	DDX_Control(pDX, IDC_STATIC_TIMESTAMP, m_StaticTimestamp);
}

BEGIN_MESSAGE_MAP(CCUL_TestDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CHECK_AES, &CCUL_TestDlg::OnClickedCheckAes)
	ON_EN_KILLFOCUS(IDC_EDIT_MSG_COUNT, &CCUL_TestDlg::OnKillfocusEditMsgCount)
	ON_LBN_KILLFOCUS(IDC_LIST_SENDER, &CCUL_TestDlg::OnKillfocusListSender)
	ON_LBN_KILLFOCUS(IDC_LIST_DESTINATION, &CCUL_TestDlg::OnKillfocusListDestination)
	ON_EN_KILLFOCUS(IDC_EDIT_CH1_COUNT, &CCUL_TestDlg::OnKillfocusEditCh1Count)
	ON_EN_KILLFOCUS(IDC_EDIT_CH2_COUNT, &CCUL_TestDlg::OnKillfocusEditCh2Count)
	ON_BN_CLICKED(IDC_BUTTON_SEND1, &CCUL_TestDlg::OnClickedButtonSend1)
	ON_BN_CLICKED(IDC_BUTTON_SEND2, &CCUL_TestDlg::OnClickedButtonSend2)
	ON_EN_KILLFOCUS(IDC_EDIT_AES, &CCUL_TestDlg::OnKillfocusEditAes)
	ON_MESSAGE(SerialDataAvailable, CCUL_TestDlg::OnSerialdataavailable)
	ON_MESSAGE(SerialReceived, CCUL_TestDlg::OnSerialreceived)
	ON_BN_CLICKED(IDC_BUTTON_SCAN, &CCUL_TestDlg::OnBnClickedButtonScan)
END_MESSAGE_MAP()


// CCUL_TestDlg-Meldungshandler

BOOL CCUL_TestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Großes Symbol verwenden
	SetIcon(m_hIcon, FALSE);		// Kleines Symbol verwenden

	// TODO: Hier zusätzliche Initialisierung einfügen
	// Pfeuffer Neu: *********************************************************************************
	const unsigned int	nStringPufferLaenge = 100;
	char				acStringPuffer[nStringPufferLaenge];
	// ******************* IDC_STATIC_TITLE *******************
	// Versuch den Text der Überschrift größer zu machen
	CStatic*	pStaticTitle = (CStatic *) GetDlgItem(IDC_STATIC_TITLE);
	CFont *		pfont =	pStaticTitle->GetFont();
	LOGFONT		lf;
	pfont->GetLogFont(&lf);
	lf.lfHeight = 25;
	// Für den vergößerten Font sicherheitshalber eine Membervariable verwendet.
	// Es ist nicht klar ob eine lokale Lebensdauer ausreicht.
	m_TitleFont.CreateFontIndirect(&lf);
	pStaticTitle->SetFont(&m_TitleFont);

	// Gcrypt starten
	_InitGcrypt();
	
	// ******************* IDC_STATIC_CONNECT *******************
	CStatic* pStaticConnect = (CStatic *) GetDlgItem(IDC_STATIC_CONNECT);
	pStaticConnect->SetWindowTextA(m_acConnectString);

	// ******************* IDC_SPIN_MSG_COUNT *******************
	CSpinButtonCtrl* pSpinMsgCount = (CSpinButtonCtrl *) GetDlgItem(IDC_SPIN_MSG_COUNT);
	pSpinMsgCount->SetRange(0, 255);

	// ******************* IDC_STATIC_CONTROL_BYTE *******************
	CStatic* pStaticControlByte = (CStatic *) GetDlgItem(IDC_STATIC_CONTROL_BYTE);
	_snprintf(acStringPuffer, nStringPufferLaenge, "0x%02x", nControlByte);
	pStaticControlByte->SetWindowTextA(acStringPuffer);

	// ******************* IDC_STATIC_MSG_TYPE *******************
	CStatic* pStaticMsgType = (CStatic *) GetDlgItem(IDC_STATIC_MSG_TYPE);
	_snprintf(acStringPuffer, nStringPufferLaenge, "KEY_EVENT (0x%02x)", nMessageType);
	pStaticMsgType->SetWindowTextA(acStringPuffer);

	// ******************* IDC_LIST_SENDER *******************
	ReadIniFileDevices(m_IniFileName, &m_ListBoxSender);

	// ******************* IDC_LIST_DESTINATION *******************
	ReadIniFileDevices(m_IniFileName, &m_ListBoxDestination);
	
	// ******************* IDC_SPIN_CH1_COUNT *******************
	CSpinButtonCtrl* pSpinCh1Count = (CSpinButtonCtrl *) GetDlgItem(IDC_SPIN_CH1_COUNT);
	pSpinCh1Count->SetRange(0, 255);

	// ******************* IDC_SPIN_CH2_COUNT *******************
	CSpinButtonCtrl* pSpinCh2Count = (CSpinButtonCtrl *) GetDlgItem(IDC_SPIN_CH2_COUNT);
	pSpinCh2Count->SetRange(0, 255);

	// ******************* IDC_EDIT_AES *******************
	CEdit * pEditAes = (CEdit *) GetDlgItem(IDC_EDIT_AES);
	pEditAes->SetWindowTextA("No Key");
	pEditAes->EnableWindow(FALSE);

	// ******************* IDC_STATIC_MESSAGE *******************
	m_StaticMessage.SetWindowTextA("No Message");
	m_StaticMessage.EnableWindow(FALSE);
	
	// ******************* IDC_STATIC_CHALLENGE *******************
	m_StaticChallenge.SetWindowTextA("No Challenge");
	m_StaticChallenge.EnableWindow(FALSE);
	
	// ******************* IDC_STATIC_CHALLENGE *******************
	m_StaticTimestamp.SetWindowTextA("No Timestamp");
	m_StaticTimestamp.EnableWindow(FALSE);

	// ******************* IDC_STATIC_RRESPONSE *******************
	m_StaticResponse.SetWindowTextA("No Response");
	m_StaticResponse.EnableWindow(FALSE);

	// ******************* IDC_STATIC_ACK *******************
	m_StaticAck.SetWindowTextA("No Acknowledge");
	m_StaticAck.EnableWindow(FALSE);

	// ******************* Empfang starten *******************
	m_pCulAsync->StartHmReceive();
	m_pCulAsync->StartThread(GetSafeHwnd(), SerialReceived);

	// Nach StartHmReceive() ist der Versionsstring des CULs verfügbar
	strncat(m_acConnectString, " CUL-Version: ", nMaxConnectString);
	strncat(m_acConnectString, m_pCulAsync->GetVerionString(), nMaxConnectString);
	pStaticConnect->SetWindowTextA(m_acConnectString);

	return TRUE;  // TRUE zurückgeben, wenn der Fokus nicht auf ein Steuerelement gesetzt wird
}

void CCUL_TestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // Gerätekontext zum Zeichnen

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Symbol in Clientrechteck zentrieren
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Symbol zeichnen
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// Die System ruft diese Funktion auf, um den Cursor abzufragen, der angezeigt wird, während der Benutzer
//  das minimierte Fenster mit der Maus zieht.
HCURSOR CCUL_TestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCUL_TestDlg::ClearStatusText()
{
	m_StaticMessage.SetWindowTextA("Not Initialized");
	m_StaticChallenge.SetWindowTextA("Not Initialized");
	m_StaticTimestamp.SetWindowTextA("Not Initialized");
	m_StaticResponse.SetWindowTextA("Not Initialized");
	m_StaticAck.SetWindowTextA("Not Initialized");
}

void CCUL_TestDlg::OnClickedCheckAes()
{
	// TODO: Fügen Sie hier Ihren Kontrollbehandlungscode für die Benachrichtigung ein.
	// Dialogfeld "AES Key" aktivieren/deaktivieren
	CEdit * pEditAes = (CEdit *) GetDlgItem(IDC_EDIT_AES);
	UpdateData(TRUE);		// TRUE = Daten von dem Dialogfeld lesen,

	pEditAes->EnableWindow(m_bAesEnable);

	// ******************* IDC_STATIC_MESSAGE *******************
	ClearStatusText();
	m_StaticMessage.EnableWindow(m_bAesEnable);
	m_StaticChallenge.EnableWindow(m_bAesEnable);
	m_StaticTimestamp.EnableWindow(m_bAesEnable);
	m_StaticResponse.EnableWindow(m_bAesEnable);
	m_StaticAck.EnableWindow(m_bAesEnable);

	if (m_bAesEnable)
		pEditAes->SetFocus();
}

void CCUL_TestDlg::OnKillfocusEditMsgCount()
{
	// TODO: Fügen Sie hier Ihren Kontrollbehandlungscode für die Benachrichtigung ein.
	UpdateData(TRUE);		// TRUE = Daten von dem Dialogfeld lesen,
}

void CCUL_TestDlg::OnKillfocusEditCh1Count()
{
	UpdateData(TRUE);		// TRUE = Daten von dem Dialogfeld lesen,
}

void CCUL_TestDlg::OnKillfocusEditCh2Count()
{
	UpdateData(TRUE);		// TRUE = Daten von dem Dialogfeld lesen,
}

void CCUL_TestDlg::OnClickedButtonSend1()
{
	char	Meldung[256] = {0};			// Puffer für Meldungstexte

	UpdateData(TRUE);
	CreateTxData(1);
	ClearStatusText();
	_FormatHexString(Meldung, m_ac_m_Frame, 13);
	m_StaticMessage.SetWindowTextA(Meldung);
	
	m_pCulAsync->SendHmPacket(m_ac_m_Frame,13);

	m_nEditMsgCount++;
	if (m_nEditMsgCount > 255)
		m_nEditMsgCount = 0;

	m_nEditCh1Count++;
	if (m_nEditCh1Count > 255)
		m_nEditCh1Count = 0;

	UpdateData(FALSE);
}

void CCUL_TestDlg::OnClickedButtonSend2()
{
	char	Meldung[256] = {0};			// Puffer für Meldungstexte

	UpdateData(TRUE);
	ClearStatusText();
	CreateTxData(2);
	_FormatHexString(Meldung, m_ac_m_Frame, 13);
	m_StaticMessage.SetWindowTextA(Meldung);
	
	m_pCulAsync->SendHmPacket(m_ac_m_Frame,13);

	m_nEditMsgCount++;
	if (m_nEditMsgCount > 255)
		m_nEditMsgCount = 0;

	m_nEditCh2Count++;
	if (m_nEditCh2Count > 255)
		m_nEditCh2Count = 0;

	UpdateData(FALSE); 
}

void CCUL_TestDlg::OnKillfocusEditAes()
{
	UpdateData(TRUE);

	int		nAesChar		= m_sEditAes.GetLength();
	char	*pBufferPointer	= m_sEditAes.GetBuffer();
	int		nPufferIndex	= 0;
	char	*pConvPointer;
	int		nHexWert;
	int		nAnzahlHexZahlen = 0;
	char	acStringPuffer[16*3+1];			// sprintf() schreibt noch eine '\0' als Abschluß in den String
	int		nMessageBoxReturn = 0;
	CEdit	*pEditAes = NULL;

	pEditAes = (CEdit *) GetDlgItem(IDC_EDIT_AES);
	while ((nPufferIndex < nAesChar) && (16 > nAnzahlHexZahlen))
	{
		pConvPointer = pBufferPointer + nPufferIndex;
		if (isspace(*pConvPointer))
		{
			nPufferIndex++;
			continue;
		}
		if (1 != sscanf(pConvPointer,"%2x", &nHexWert))
		{
			// Formatfehler
			nMessageBoxReturn = MessageBox("Format error in the AES string.\nShould the standard key be used?", NULL, MB_YESNO | MB_ICONWARNING);
			if (IDYES == nMessageBoxReturn)
			{
				memcpy(m_acAesKey, acDefaultKey, sizeof(acDefaultKey));
				nAnzahlHexZahlen = 16;
				break;
			}
			pEditAes->SetFocus();
			return;
		}
		nPufferIndex += 2;
		m_acAesKey[nAnzahlHexZahlen] = (unsigned char) nHexWert;
		nAnzahlHexZahlen++;
	}
	_FormatHexString(acStringPuffer,  m_acAesKey, nAnzahlHexZahlen);
	m_sEditAes=acStringPuffer;
	UpdateData(FALSE);

	if (16 != nAnzahlHexZahlen)
	{
		// Formatfehler
		nMessageBoxReturn = MessageBox("AES key must consist of 16 bytes.\nShould the standard key be used?", NULL, MB_YESNO | MB_ICONWARNING);
		if (IDYES == nMessageBoxReturn)
		{
			memcpy(m_acAesKey, acDefaultKey, sizeof(acDefaultKey));
			nAnzahlHexZahlen = 16;
			_FormatHexString(acStringPuffer,  m_acAesKey, nAnzahlHexZahlen);
			m_sEditAes=acStringPuffer;
			UpdateData(FALSE);
		}
		pEditAes->SetFocus();
		return;
	}
}

void CCUL_TestDlg::_FormatHexString(char *pString, const unsigned char *pArray, int nLaenge)
{
	for (int i = 0; i < nLaenge; i++)
	{
		sprintf(pString, "%02x ", pArray[i]);
		pString+=3;
	}
}

bool CCUL_TestDlg::ReadIniFileDevices(LPCTSTR lpFileName, CListBox * pListBox)
{
	const unsigned int	nTempArraySize = 4096;
	
	char				acTempArray[nTempArraySize], *pcZeiger;
	char				acDevieName[80];
	char				acListBox[nMaxListBoxString];
	unsigned long		nAdress;
	unsigned long		nPosition = 0;
	DWORD				nNumCharRead;

	nNumCharRead = GetPrivateProfileSectionNames(acTempArray, nTempArraySize, lpFileName);
	if (0 == nNumCharRead)
	{
		_snprintf(acTempArray, nTempArraySize, "Error reading .INI file %s", lpFileName);
		MessageBox(acTempArray, NULL, MB_OK | MB_ICONWARNING);
		return false;
	}

	nNumCharRead = GetPrivateProfileSection("DEVICES", acTempArray, nTempArraySize, lpFileName);
	while (nPosition < nNumCharRead)
	{
		pcZeiger = acTempArray+nPosition;
		sscanf(pcZeiger, "%lx=%80[0-9a-zA-Z_. ]s", &nAdress, acDevieName);
		nPosition += strlen(pcZeiger)+1;

		// String für die Listbox zusammenbauen:
		_snprintf(acListBox, nMaxListBoxString, "0x%06x %s", nAdress, acDevieName);
		acListBox[nMaxListBoxString-1] = '\0';	// Falls zu lange: Stringende einbauen
		// nIndex:	Contains the zero-based index to the position in the list box that will receive the string. If this parameter is –1, 
		//			the string is added to the end of the list.
		pListBox->InsertString(-1, acListBox);
		pListBox->SetCurSel(0);					// Erstes Element auswählen
	}
	return true;
}

bool CCUL_TestDlg::CreateTxData(int nChannelNr)
{	// Version Fensterkontakt Test für Signatur
	UpdateData(TRUE);

	int					nListBoxIndex;
	char				acListBoxPuffer[nMaxListBoxString];

	// Sender Adresse aus der List Box lesen
	nListBoxIndex = m_ListBoxSender.GetCurSel();
	m_ListBoxSender.GetText(nListBoxIndex,acListBoxPuffer);
	sscanf(acListBoxPuffer, "%lx", &m_nSenderAddress);

	// Destination Adresse aus der List Box lesen
	nListBoxIndex = m_ListBoxDestination.GetCurSel();
	m_ListBoxSender.GetText(nListBoxIndex,acListBoxPuffer);
	sscanf(acListBoxPuffer, "%lx", &m_nDestinationAddress);

	m_cAktuelleMessageNummer	= m_nEditMsgCount;				// Message-Count für den Empfang merken

	m_ac_m_Frame[0]		= 12;									// Paketlänge
	m_ac_m_Frame[1]		= m_nEditMsgCount;						// Message-Count
	m_ac_m_Frame[2]		= nControlByte;							// Control-Byte
	m_ac_m_Frame[3]		= 0x40;									// Message-Type
	m_ac_m_Frame[4]		= 0xff & (m_nSenderAddress >> 16);		// Sender-Adresse[2]
	m_ac_m_Frame[5]		= 0xff & (m_nSenderAddress >>  8);		// Sender-Adresse[1]
	m_ac_m_Frame[6]		= 0xff & (m_nSenderAddress >>  0);		// Sender-Adresse[0]
	m_ac_m_Frame[7]		= 0xff & (m_nDestinationAddress >> 16);	// Ziel-Adresse[2]
	m_ac_m_Frame[8]		= 0xff & (m_nDestinationAddress >>  8);	// Ziel-Adresse[1]
	m_ac_m_Frame[9]		= 0xff & (m_nDestinationAddress >>  0);	// Ziel-Adresse[0]
	m_ac_m_Frame[10]	= nChannelNr;							// Kanal Index
	m_ac_m_Frame[11]	= m_nEditCh1Count;							// Kanal Zähler
	m_ac_m_Frame[12]	= 200*(nChannelNr == 2);

	if(m_ac_m_Frame[4] == 0x24 && m_ac_m_Frame[5] == 0xc5 && m_ac_m_Frame[6] == 0x59)
		m_ac_m_Frame[3] = 0x41;
	
	return true;
}

void CCUL_TestDlg::SetCul(CCulAsync* pCulAsync)
{
	m_pCulAsync = pCulAsync;
}

void CCUL_TestDlg::SetConnectString(char * pConnectString)
{
	if (NULL != pConnectString)
	{
		strncpy(m_acConnectString, pConnectString, nMaxConnectString);
		m_acConnectString[nMaxConnectString -1] = '\0';
	}
}

void CCUL_TestDlg::SetIniFileName(const char * pIniFileName)
{
	m_IniFileName = pIniFileName;
}


afx_msg LRESULT CCUL_TestDlg::OnSerialreceived(WPARAM wParam, LPARAM lParam)
{
	EnterCriticalSection(&m_LockZaehler);
	m_nAufrufZaehler++;
	LeaveCriticalSection(&m_LockZaehler);

	if (m_nAufrufZaehler > 1)
		return 1;
	do 
	{
		SendMessage(SerialDataAvailable, m_nAufrufZaehler, 0L);
		EnterCriticalSection(&m_LockZaehler);
		m_nAufrufZaehler--;
		LeaveCriticalSection(&m_LockZaehler);
	} while (m_nAufrufZaehler > 0);
	return 1;
}

afx_msg LRESULT CCUL_TestDlg::OnSerialdataavailable(WPARAM wParam, LPARAM lParam)
{

// Eventuell m_bAesEnable abfragen ?????
	char			Meldung[256] = {0};						// Puffer für Meldungstexte
	unsigned char	acPuffer[PUFFERLAENGE];					// Puffer für die Empfangsdaten
	int				NumBytes;								// Anzahl Bytes der Empfangsdaten
	sHommaticPacket	shPacket;								// Datenstruktur Paket
	bool			bSendRetValue = false;					// Rückgabewert für SendHmPacket()

	m_pCulAsync->GetPacket(acPuffer, PUFFERLAENGE);			// 1 Paket mit den Empfangsdaten abholen
	NumBytes = acPuffer[0] + 1;								// Paketlänge ermitteln
	if (60 < NumBytes)										// Paket zu lange? 
		return 0;

	_FormatHexString(Meldung, acPuffer, NumBytes);			// Als Hex-String vorbereiten
	m_pCulAsync->ParsePacket(&shPacket, acPuffer);			// Empfangspaket auswerten

	if (scanStart && 0x04 == shPacket.Type)
	{
		//if(button is one then execute else skip)
		payloaddecryption(shPacket, NumBytes);
	}
	// Wir interessieren uns nur für Pakete zur "Schaltersimulation
	if ((m_nSenderAddress			== shPacket.DestinationAddress) && 
		(m_nDestinationAddress		== shPacket.SenderAddress) &&
		(m_cAktuelleMessageNummer	== shPacket.MessageCount))
	{
		// MessageType = ACK ?
		if (0x02 == shPacket.Type)
		{
			switch (shPacket.Payload[0])	// Acknowledge Subtype
			{
			case 0x00:						// OK
				strcat(Meldung, "  (OK)");
				m_StaticAck.SetWindowTextA(Meldung);
				break;
			case 0x01:						// ACK_STATUS 
				strcat(Meldung, "  (ACK Status)");
				m_StaticAck.SetWindowTextA(Meldung);
				break;
			case 0x04:						// Signature request
				memcpy(m_ac_c_Frame, acPuffer, 18);
				CreateResponsePacket();
				bSendRetValue = m_pCulAsync->SendHmPacket(m_ac_r_Frame,26);
				m_StaticChallenge.SetWindowTextA(Meldung);
				break;
			default:
				m_StaticChallenge.SetWindowTextA("ACK message not expected");
				;
			}
		}
	}

	return 0;
}


void CCUL_TestDlg::OnKillfocusListSender()
{
	UpdateData(TRUE);

	int		nListBoxIndex;
	char	acListBoxPuffer[nMaxListBoxString];

	// Sender Adresse aus der List Box lesen
	nListBoxIndex = m_ListBoxSender.GetCurSel();
	m_ListBoxSender.GetText(nListBoxIndex,acListBoxPuffer);
	sscanf(acListBoxPuffer, "%lx", &m_nSenderAddress);

	// Sender Adresse aus der List Box lesen
	nListBoxIndex = m_ListBoxDestination.GetCurSel();
	m_ListBoxSender.GetText(nListBoxIndex,acListBoxPuffer);
	sscanf(acListBoxPuffer, "%lx", &m_nDestinationAddress);
}

void CCUL_TestDlg::OnKillfocusListDestination()
{
	UpdateData(TRUE);

	int		nListBoxIndex;
	char	acListBoxPuffer[nMaxListBoxString];

	// Destination Adresse aus der List Box lesen
	nListBoxIndex = m_ListBoxDestination.GetCurSel();
	m_ListBoxSender.GetText(nListBoxIndex,acListBoxPuffer);
	sscanf(acListBoxPuffer, "%lx", &m_nDestinationAddress);
}

bool CCUL_TestDlg::_InitGcrypt()
{
	const char		*pVersion;
	gcry_error_t	gcRetValue;

	CStatic* pStaticGcrypt = (CStatic *) GetDlgItem(IDC_STATIC_GCRYPT);
	pStaticGcrypt->SetWindowTextA("Gcrypt has not yet started");

	//	Aus der Dokumentation von gcrypt:
	//
	//	Before the library can be used, it must initialize itself. This is achieved by invoking the
	//	function gcry_check_version described below.
	//	Also, it is often desirable to check that the version of Libgcrypt used is indeed one
	//	which fits all requirements. Even with binary compatibility, new features may have been
	//	introduced, but due to problem with the dynamic linker an old version may actually be
	//	used. So you may want to check that the version is okay right after program startup.
	pVersion = gcry_check_version (NULL);

	gcRetValue = gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
	gcRetValue = gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
	gcRetValue = gcry_control (GCRYCTL_INITIALIZATION_FINISHED_P);
	if (!gcRetValue)
	{
		MessageBox( "Error initializing gcrypt!", NULL,MB_ICONWARNING);
		return false;
	}

	//	To use a cipher algorithm, you must first allocate an according handle. This is to be done
	//	using the open function:
	//	gcry_error_t gcry_cipher_open (gcry cipher hd t *hd, int algo, int mode, unsigned int flags)
	//
	//	This function creates the context handle required for most of the other cipher functions
	//	and returns a handle to it in ‘hd’. In case of an error, an according error code is
	//	returned.
	m_hCipher = NULL;
	// Optionen gewählt entsprechnend der Vorlage in:
	//	.../Homegear-HomeMaticBidCoS-master/src/PhysicalInterfaces/AesHandshake.cpp
	//	.../Homegear-HomeMaticBidCoS-master/src/PhysicalInterfaces/HM-CFG-LAN.cpp
	//	.../Homegear-HomeMaticBidCoS-master/src/PhysicalInterfaces/HM-LGW.cpp
	//	GCRY_CIPHER_AES128:	Verschlüsselungsverfahren
	//	GCRY_CIPHER_MODE_ECB:	"Electronic Codebook mode"	"mode" in "AesHandshake.cpp"
	//	GCRY_CIPHER_MODE_CFB:	"Cipher Feedback mode"		"mode" in "HM-CFG-LAN.cpp" und "HM-LGW.cpp"
	//	GCRY_CIPHER_SECURE:		Make sure that all operations are allocated in secure memory. This is
	//							useful when the key material is highly confidential.sizeof(acHmBsp1)
	//							Für das Beispiel nicht nötig.
	gcRetValue = gcry_cipher_open (&m_hCipher, GCRY_CIPHER_AES128,GCRY_CIPHER_MODE_ECB,0);
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		MessageBox( "Gcry_cipher_open () failed!", NULL,MB_ICONWARNING);
		return false;
	}
	char	acMessagePuffer[256];
	_snprintf(acMessagePuffer, 256, "Gcrypt Version: %s", pVersion);
	pStaticGcrypt->SetWindowTextA(acMessagePuffer);

	return true;
}

void CCUL_TestDlg::_CloseGcrypt()
{
	if (NULL != m_hCipher)
	{
		gcry_cipher_close(m_hCipher);
		m_hCipher = NULL;
	}
}

void CCUL_TestDlg::CreateTimestamp()
{
	for (int i = 0; i < 6 ; i++)
		m_acTimestamp[i] = rand() % 256;

}

bool CCUL_TestDlg::CreateResponsePacket()
{
	// Die Signatur baut auf dem "Dissecting HomeMatic AES" auf.
	// https://git.zerfleddert.de/hmcfgusb/AES/
	//
	// https://blog.ploetzli.ch/2015/on-the-security-of-aes-in-homematic/
	//
	// Packets:
	//	+---+------------------------------------------------------------------------------------
	//	| m |	Original message to be authenticated. Note: m = D0 // D1, if the 
	//	|	|	length is considered not to be part of the packet.
	//	| c	|	Challenge message
	//	| r	|	Response message
	//  | a	|	ACK message
	//	+---+------------------------------------------------------------------------------------
	//
	// Data items:
	//	+-------+---------------------------------------------------+---------------+------------
	//	| Name	| Description										| Length/bytes	| Packet
	//	| D0	| Metadata (counter, flags, type, sender, receiver)	| 10			| m
	//	| D1	| Parameters										| varies		| m
	//	| C		| Challenge											|  6			| c
	//	| P		| AES response										| 16			| r
	//	| A		| ACK authentication								|  4			| a
	//	| T		| Timestamp or counter								|  6			| 	
	//	+-------+---------------------------------------------------+---------------+------------
	//
	// Under these definitions the calculation of the authentication messages happens as follows:

	// HomeMatic AES signature verification algorithm
	// ==============================================
	// Based on these findings, the algorithm to verify the signature is:
	// 1. A temporary key is built by XORing the first 6 bytes of the key with the challenge (bytes 12-17 of the c-frame).
	// 
	// 2. The parameters of the m-frame are used as an IV, padded with 0x00 at the end to fill 16 bytes.
	//
	// 3. The AES-payload P of the r-frame is decrypted with the temporary key, producing Pd.
	//
	// 4. The IV is XORed to the decrypted Pd, resulting in Pd^. The first 4 bytes of Pd^ will be sent in the ACK-message.
	//
	// 5. The resulting Pd^ is decrypted (again) with the temporary key, producing Pd^d.

	gcry_error_t	gcRetValue;
	unsigned char	acTempKey[16]	= {0};			// Exor aus dem AES-Schlüssel und der Challenge
	unsigned char	acPexD[16]		= {0};			// zusammengefügt aus dem Timestamp und den Metadaten des m-Frames
	unsigned char	acPex[16]		= {0};			// AES-verschlüsselung von PexD
	unsigned char	acPd[16]		= {0};			// Exor aus Pex und der Payload des m-Frames
	unsigned char	acAesP[16]		= {0};			// Response Daten
	unsigned char	acIV[16]		= {0};			// Anfangswert 

	CreateTimestamp();								// Neuen Timestamp erzeugen

	//	Erzeugen der Response-Daten
	//  ***************************
	//
	//	+-----------+           +-----------+           +-----------+           +------------------+
	//	| AES - Key |           | c - Frame |           | Timestamp |           |    m - Frame     |
	//	+-----+-----+           +-----+-----+           +-----+-----+           +--+------------+--+
	//        |                       | 11 ... 16             |                    | 1.. 10     | 11 .. n
	//        |                       | Challenge             |                    | Metadaten  | Payload
	//        |                       |                       |                    |            |
	//        | m_acAesKey            | m_ac_c_Frame          | acTimestamp        |            | m_ac_m_Frame
	//        | (K)                   | (C)                   | (T)                | (D0)       | (D1)
	//        |                       |                       |                    |            |
	//        |     +-----------+     |                       |    +-----------+   |			|
	//        +---->+   EXOR    +<----+                       +--->+ verbinden +<--+            |
	//              +-----+-----+                                  +-----+-----+                |
	//                    |                                              |                      |
	//                    | acTempKey             +----------------------+ acPexD               |
	//                    | (K')                  |                                             |
	//                    |                       |                                             |
	//                    |                      \|/                                            |
	//                    |            +----------+----------+                                  |
	//                    +----------->+ AES-Verschlüsselung |                                  |
	//                    |            +---------------------+                                  |
	//                    |                       |                                             |
	//                    |                       | acPex                                       |
	//                    |                       | (Pd')                                       |
	//                    |                      \|/                                            |
	//                    |                 +-----------+                                       |
	//                    |                 |   EXOR    +<--------------------------------------+
	//                    |                 +-----+-----+     
	//                    |                       |
	//                    |                       | acPd
	//                    |                       |
	//                    |                      \|/
	//                    |            +----------+----------+
	//                    +----------->+ AES-Verschlüsselung |
	//                                 +----------+----------+
	//                                            |
	//                                            | acAesP
	//                                            | (P)
	//                                            |
	//
	//
	//	
	//
	// 1. Einen temporären Schlüssel durch EXOR-Verknüpfung der ersten 6 Byte des AES-Schlüssels mit 
	//	der "challenge" (bytes 12-17 of the c-frame) erzeugen
	memcpy(acTempKey, m_acAesKey, 16);					// AES-Schlüssel nach acTempKey kopieren
	for (int i = 0; i<6; i++)
		acTempKey[i] ^= m_ac_c_Frame[11+i];				// EXOR-Verknüpfung mit der Challenge

	// Den temporären Schlüssel setzen
	gcRetValue = gcry_cipher_setkey (m_hCipher, acTempKey, sizeof(acTempKey));
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		MessageBox("Gcry_cipher_setkey () failed!", NULL,MB_ICONWARNING);
		return false;
	}

	// 2. IV (Initial Value) für die erste Verschlüsselung setzen
	gcRetValue = gcry_cipher_setiv(m_hCipher, acIV, sizeof(acIV));
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		MessageBox("Gcry_cipher_setiv () failed!", NULL,MB_ICONWARNING);
		return false;
	}

	// 3. Pd^D durch Ergänzen des Timestamps mit den Metadaten des m-Frames (Byte 1 bis 11)
	//	(Length, MessageCount, ControlByte, Type, SenderAddress, DestinationAddress) erzeugen.
	memcpy(acPexD, m_acTimestamp, sizeof(m_acTimestamp));
	memcpy(acPexD + sizeof(m_acTimestamp), m_ac_m_Frame+1, sizeof(acPexD)-sizeof(m_acTimestamp));
	//acPexD[7] &= 0xBF;	// xxx-Bit löschen (AESHandshake.cpp Zeile 419 (funktioniert aber nicht!!)

	// 4. Pd^D mit dem temporären Schlüssel verschlüsseln. Ergebnis Pd^ (acPex)
	gcRetValue =  gcry_cipher_encrypt (m_hCipher, acPex, sizeof(acPex), acPexD, sizeof(acPexD));
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		MessageBox("Gcry_cipher_encrypt (Pd ^ D) failed!", NULL,MB_ICONWARNING);
		return false;
	}

	// 5. Pd durch EXOR-Verknüpfung von acPex und den Payload Daten des m-Frames erzeugen
	int nNumBytes;						// Anzahl der zu verknüpfenden Bytes
	nNumBytes = m_ac_m_Frame[0]-10;		// Gesamtlänge des m-Frames - Länge der Metadaten
	if (16 < nNumBytes)					// auf maximal 16 Byte begrenzen
		nNumBytes = 16;
	memcpy(acPd, acPex, sizeof(acPd));
	for (int i = 0; i<nNumBytes; i++)
		acPd[i] ^= m_ac_m_Frame[i+11];

	// 6. Pd nochmal mit dem temporären Schlüssel verschlüsseln. Ergebnis sind die Response Daten.
		gcRetValue =  gcry_cipher_encrypt (m_hCipher, acAesP, sizeof(acAesP), acPd, sizeof(acPd));
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		MessageBox("Gcry_cipher_encrypt (Pd) error!", NULL,MB_ICONWARNING);
		return false;
	}

	// 7. Response Paket erzeugen:
	// Metadaten aus dem m-Frame kopieren
	memcpy(m_ac_r_Frame, m_ac_m_Frame, 10);
	// Response Daten anhängen
	memcpy(m_ac_r_Frame+10, acAesP, 16);
	m_ac_r_Frame[0]	= 25;						// Länge = 25 Byte 
	m_ac_r_Frame[2]	= nControlByte & 0xBF;		// Control Byte 2^6-Bit (0x40) löschen (AESHandshake.cpp Zeile 419 
												// HOMEMATIC_RPTED_FLAG (Removed in response)
	m_ac_r_Frame[3]	= 0x03;						// Message Type: Challenge response (0x03)

	// Meldung aktualisieren
	char	Meldung[256];		// Puffer für Meldungstexte
	_FormatHexString(Meldung, m_ac_r_Frame, 26);
	m_StaticResponse.SetWindowTextA(Meldung);
	_FormatHexString(Meldung, m_acTimestamp, 6);
	m_StaticTimestamp.SetWindowTextA(Meldung);

	return true;
}

void ShowAMsgBox(char* szTitle, UINT time, HWND hWnd)
{
	BOOL		stoppedByUser = FALSE;
	UINT		erg;
	CString		msg("Scanning for the key"),
		ret;

	if (time > 0)
	{
		erg = CDlgTimedMessageBox::TimedMessageBox(MB_OKCANCEL | MB_ICONASTERISK | MB_DEFBUTTON2, msg, szTitle,
			time, IDCANCEL , /*NULL*/"\nfor the next %lu sec !", 
			hWnd, &stoppedByUser);
		if (scanStart == FALSE && erg == 2) 
		{			
			HWND hWnd = FindWindow(0, "CUL_Simulation");
			LPCTSTR Info="Information";
			MessageBox(hWnd,"Key has been captured.", Info, MB_OK | MB_ICONASTERISK);
		}
		if (scanStart == TRUE && erg == 2) {
				HWND hWnd = FindWindow(0, "CUL_Simulation");
			MessageBox(hWnd, "Scanning is stopped", NULL, MB_OK | MB_ICONSTOP);
			scanStart = FALSE;
		}
	}
}

UINT ThreadMesssageBox(LPVOID pParam)
{
	CCUL_TestDlg* pThis = (CCUL_TestDlg*)pParam;
	ShowAMsgBox("Scan for the Key", 120000, NULL);
	return 0;
}
UINT changeOKofMsgBox(LPVOID pParam) {
	Sleep(105);
	HWND hWnd;
	hWnd = FindWindow(0, "Scan for the Key");
	ShowWindow(GetDlgItem(hWnd, IDOK), SW_HIDE);
	return 0;
}
UINT CloseTimerPopupThread(LPVOID pParam)
{
	HWND hWnd;
	hWnd = FindWindow(0, "Scan for the Key");
	scanStart = FALSE;
	if (hWnd) ::PostMessage(hWnd, WM_CLOSE, 0, 0); ;
	return 0;
}

void CCUL_TestDlg::OnBnClickedButtonScan()
{
	// Created by Chittimilla Saikiran

	UpdateData(TRUE);
	AfxBeginThread(ThreadMesssageBox, this, THREAD_PRIORITY_NORMAL, 0, 0);
	AfxBeginThread(changeOKofMsgBox, this, THREAD_PRIORITY_NORMAL, 0, 0);	
	scanStart = TRUE;
	memset(ctestbuffer,'\0',sizeof(ctestbuffer));	
} 



gcry_cipher_hd_t CCUL_TestDlg::getcipher()
{
	return m_hCipher;
}

int CCUL_TestDlg::DecryptKeyChangePacket(unsigned char* acEncrypted, unsigned char* acDecrypted)
{
	gcry_error_t gcRetValue;
	unsigned char acIV[16] = { 0 };
	hCipher = getcipher();
	gcRetValue = gcry_cipher_setkey(hCipher, acDefaultKey, sizeof(acDefaultKey));
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		gcry_cipher_close(hCipher);
		return 0;
	}

	gcRetValue = gcry_cipher_setiv(hCipher, acIV, sizeof(acIV));
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		gcry_cipher_close(hCipher);
		hCipher = 0;
		return 0;
	}
	gcRetValue = gcry_cipher_decrypt(hCipher, acDecrypted, 16, acEncrypted, 16);
	if (GPG_ERR_NO_ERROR != gcRetValue)
	{
		gcry_cipher_close(hCipher);
		hCipher = 0;
		return 0;
	}
	return 1;
}

void CCUL_TestDlg::payloaddecryption(struct sHommaticPacket shPacket, unsigned char NumBytes)
{
	unsigned char acEncrypted[16] = { 0 };
	unsigned char acDecrypted[16] = { 0 };
	unsigned char nPruefzahl[32];
	int		nRetValue;
	unsigned char bcPuffer[100];

	if ((16 + 9 + 1) == NumBytes)
	{
		memcpy(acEncrypted, shPacket.Payload, 16);

		nRetValue = DecryptKeyChangePacket(acEncrypted, acDecrypted);

		if ((0x7E == acDecrypted[12]) &&
			(0x29 == acDecrypted[13]) &&
			(0x6F == acDecrypted[14]) &&
			(0xA5 == acDecrypted[15]) &&
			(1 == nRetValue))
		{
			char cPrintBuffer[(8 * 3 + 1)] = { 0 };
			int i;
			for (i = 0; i < 8; i++)
			{
				sprintf(cPrintBuffer + (i * 3), "%02x ", acDecrypted[i + 2]);
			}

			unsigned char nPruefzahl = 0;
			for (i = 0; i < 4; i++)
			{
				nPruefzahl *= 256u;
				nPruefzahl += (unsigned char)acDecrypted[12 + i];
			}
			char ctestbuffer1[50] = {};
			if(ctestbuffer[0]==0)
			{
				memcpy(ctestbuffer, cPrintBuffer, sizeof(cPrintBuffer));
			}
			else
			{
				memcpy(ctestbuffer1, cPrintBuffer, sizeof(cPrintBuffer));
				std::copy(
					&ctestbuffer1[0],
					&ctestbuffer1[24],
					&ctestbuffer[strlen(ctestbuffer)]					
				);
				
				AfxBeginThread(CloseTimerPopupThread, this, THREAD_PRIORITY_NORMAL, 0, 0);
				CButton* pCheckAes = (CButton*)GetDlgItem(IDC_CHECK_AES);
				pCheckAes->SetCheck(1);
				OnClickedCheckAes();
				CEdit* pEditAes = (CEdit*)GetDlgItem(IDC_EDIT_AES);
				UpdateData(TRUE);
				pEditAes->EnableWindow(m_bAesEnable);
				pEditAes->SetFocus();
				if (m_bAesEnable) {
					pEditAes->SetFocus();
					m_sEditAes = ctestbuffer;
					pEditAes->SetWindowText(TEXT(ctestbuffer));
				}
			}
		}
	}
}