#pragma once
#include <Windows.h>

#define MAC_LEN    12
#define CPUID_LEN  32
#define HDD_SN_LEN 20
#define LOCK_LEN   16
#define KEY_LEN    512

class CLisence
{
public:
	CLisence(void);
	~CLisence(void);

	BYTE *GetLock();
	BYTE *GetKey();
	BYTE *GetKey(BYTE *byLock,size_t len);
	BYTE *GetKeyFromFile(char *sFileName);

	BOOL GetMacAddress();
	BOOL GetCPUID();
	BOOL GetHDDSerialNumber();

private:
	BYTE m_byMac[MAC_LEN];
	BYTE m_byCPUID[CPUID_LEN];
	BYTE m_byHDDSerialNumber[HDD_SN_LEN];
	BYTE m_byLock[LOCK_LEN];
	BYTE m_byKey[KEY_LEN];

	TCHAR *ConvertSENDCMDOUTPARAMSBufferToString( const DWORD *dwDiskData, DWORD nFirstIndex, DWORD nLastIndex );

};

