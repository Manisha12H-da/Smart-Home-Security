
// CUL_TestDlg.h: Headerdatei
//

#pragma once
// F�r die Verwendung von GCRYPT:
// https://www.gnupg.org/documentation/manuals/gcrypt/index.html#Top
// 
// Zus�tzliche Includeverzeichnisse:
// C:\Users\pfeuffer\Documents\Visual Studio 2010\Projects\Entwicklung\gcrypt\libgcrypt_libgcrypt-1.7.4_msvc12\include
// 
// Zus�tzliche Bibliotheksverzeichnisse:
// C:\Users\pfeuffer\Documents\Visual Studio 2010\Projects\Entwicklung\gcrypt\libgcrypt_libgcrypt-1.7.4_msvc12\lib\x86
// Das Einf�gen von #include <gcrypt.h> f�hrt zu folgender Warnung:
//
// Zus�tzliche Abh�nigkeiten: gcrypt.lib
//c:\program files (x86)\microsoft visual studio 10.0\vc\include\stdint.h(79): warning C4005: 'UINT8_MAX': Makro-Neudefinition
//          c:\program files (x86)\microsoft sdks\windows\v7.0a\include\intsafe.h(168): Siehe vorherige Definition von 'UINT8_MAX'
// Verursacht wird dies durch das Include-Statement in der Datei gpg-error.h:
// libgcrypt_libgcrypt-1.7.4_msvc12\include\gpg-error.h(956):# include <stdint.h>
//
// Folgendes hat nicht geholfen:
//#include <stdint.h>
//#include <intsafe.h>
//#define _STDINT
// 
// Erstmal die Warnung ausschalten:
#pragma warning( disable : 4005)
#include<gcrypt.h>

// CCUL_TestDlg-Dialogfeld
class CCUL_TestDlg : public CDialogEx
{
private:	// Konstanten f�r die Anwendung
	static const unsigned int	nMaxTxPuffer = 80;
	static const unsigned int	nMaxListBoxString = 30;
	static const unsigned int	nMaxConnectString = 80;

// Konstruktion
public:
	CCUL_TestDlg(CWnd* pParent = NULL);	// Standardkonstruktor
	~CCUL_TestDlg();
// Dialogfelddaten
	enum { IDD = IDD_CUL_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV-Unterst�tzung


// Implementierung
protected:
	HICON m_hIcon;

	// Generierte Funktionen f�r die Meldungstabellen
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:		// Methoden

	bool ReadIniFileDevices(LPCTSTR lpFileName, CListBox * pListBox);
	bool CreateTxData(int nChannelNr);
	void SetCul(CCulAsync* pCul);
	void SetConnectString(char * pConnectString);
	void SetIniFileName(const char * pIniFileName);
	void ClearStatusText();
	void CreateTimestamp();
	bool CreateResponsePacket();
	void _FormatHexString(char *pString, const unsigned char *pArray, int nLaenge);
	bool _InitGcrypt();
	void _CloseGcrypt();
	void payloaddecryption(struct sHommaticPacket acPuffer, unsigned char NumBytes);
	int DecryptKeyChangePacket(unsigned char* acEncrypted, unsigned char* acDecrypted);
	gcry_cipher_hd_t getcipher();


	afx_msg LRESULT OnSerialreceived(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSerialdataavailable(WPARAM wParam, LPARAM lParam);

	afx_msg void OnKillfocusEditMsgCount();
	afx_msg void OnKillfocusListSender();
	afx_msg void OnKillfocusListDestination();
	afx_msg void OnKillfocusEditCh1Count();
	afx_msg void OnKillfocusEditCh2Count();
	afx_msg void OnClickedButtonSend1();
	afx_msg void OnClickedButtonSend2();
	afx_msg void OnKillfocusEditAes();
	afx_msg void OnClickedCheckAes();

private:	
	int					m_nAufrufZaehler;
	CRITICAL_SECTION	m_LockZaehler;								// verwendet bei CCUL_TestDlg::OnSerialreceived()

private:	// Attribute
	CCulAsync*			m_pCulAsync;  
	CFont				m_TitleFont;
	int					m_nEditMsgCount;
	int					m_nEditCh1Count;
	int					m_nEditCh2Count;
	CString				m_sEditAes;
	BOOL				m_bAesEnable;
	char				m_acConnectString[nMaxConnectString];
	const char				*m_IniFileName;
	unsigned long int	m_nSenderAddress;
	unsigned long int	m_nDestinationAddress;
	gcry_cipher_hd_t	m_hCipher;									// Handle f�r gcrypt

	unsigned char		m_acAesKey[16];								// Speicher f�r den aktuellen AES-Schl�ssel
	unsigned char		m_ac_m_Frame[nMaxTxPuffer];					// m-Frame:	Message Puffer
	unsigned char		m_ac_c_Frame[nMaxTxPuffer];					// c-Frame:	Challenge Message Puffer
	unsigned char		m_ac_r_Frame[nMaxTxPuffer];					// r-Frame:	Respone Message Puffer
	unsigned char		m_ac_a_Frame[nMaxTxPuffer];					// a-Frame: ACK Message Puffer
	unsigned char		m_acTimestamp[6];							// Zufallszahl f�r das Response-Paket
	unsigned char		m_cAktuelleMessageNummer;

private:		// Control-Attribute f�r Resourcen
	CListBox	m_ListBoxSender;									// Sender Adresse
	CListBox	m_ListBoxDestination;								// Empf�nger Adresse
	CStatic		m_StaticMessage;									// Text f�r das Message Paket
	CStatic		m_StaticChallenge;									// Text f�r das Challenge Paket
	CStatic		m_StaticTimestamp;									// Text f�r den Timestamp
	CStatic		m_StaticResponse;									// Text f�r das Response Paket
	CStatic		m_StaticAck;										// Text f�r das ACK Paket
public:
	afx_msg
	void OnBnClickedButtonScan();
};
