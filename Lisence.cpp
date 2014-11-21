#include "StdAfx.h"
#include "Lisence.h"
#include <WinIoCtl.h>
#include <Iphlpapi.h>
#pragma comment(lib,"iphlpapi.lib")

#include "MD5.h"
#include "CRC32.h"
#include "Checksum32.h"

#define IDE_ATAPI_IDENTIFY 0xA1   // 读取ATAPI设备的命令
#define IDE_ATA_IDENTIFY   0xEC   // 读取ATA设备的命

CLisence::CLisence(void)
{
	memset(m_byMac,0,sizeof(m_byMac));
	memset(m_byCPUID,0,sizeof(m_byCPUID));
	memset(m_byHDDSerialNumber,0,sizeof(m_byHDDSerialNumber));
	memset(m_byLock,0,sizeof(m_byLock));
	memset(m_byKey,0,sizeof(m_byKey));
}


CLisence::~CLisence(void)
{
}

BOOL CLisence::GetMacAddress()
{
	PIP_ADAPTER_INFO pAdapterInfo = NULL;
	DWORD AdapterInfoSize = 0;

	DWORD dwErr = GetAdaptersInfo(NULL,&AdapterInfoSize);

	if((dwErr != NO_ERROR) && (dwErr != ERROR_BUFFER_OVERFLOW))
	{
		printf("GetAdaptersInfo() failed with error code:%d\n",GetLastError());
		return FALSE;
	}
	//   分配网卡信息内存  
	pAdapterInfo = (PIP_ADAPTER_INFO)GlobalAlloc(GPTR,AdapterInfoSize);
	if(pAdapterInfo == NULL)
	{
		printf("GlobalAlloc() failed with error code:%d\n",GetLastError());
		return FALSE;
	}

	if(GetAdaptersInfo(pAdapterInfo,&AdapterInfoSize) != NO_ERROR)
	{
		printf("GetAdaptersInfo() failed with error code:%d\n",GetLastError());

		GlobalFree(pAdapterInfo);
		return FALSE;
	}

	memcpy(m_byMac,pAdapterInfo->Address,MAC_LEN);

	GlobalFree(pAdapterInfo);

	return TRUE;
}

BOOL CLisence::GetCPUID()
{
	DWORD s1 = 0,s2 = 0,s3 = 0,s4 = 0;

	__asm{
		mov eax,01h   //eax=1:取CPU序列号
		xor edx,edx
		cpuid
		mov s1,edx
		mov s2,eax
	}

	__asm{
		mov eax,03h
		xor ecx,ecx
		xor edx,edx
		cpuid
		mov s3,edx
		mov s4,ecx
	}

	memcpy(m_byCPUID,&s1,sizeof(DWORD));
	memcpy(m_byCPUID + sizeof(DWORD),&s2,sizeof(DWORD));
	memcpy(m_byCPUID + 2*sizeof(DWORD),&s3,sizeof(DWORD));
	memcpy(m_byCPUID + 3*sizeof(DWORD),&s4,sizeof(DWORD));

	return TRUE;
}

BOOL CLisence::GetHDDSerialNumber()
{
	HANDLE hDisk = CreateFile(_T("\\\\.\\PHYSICALDRIVE0"),
							  GENERIC_WRITE | GENERIC_READ,
							  FILE_SHARE_WRITE | FILE_SHARE_READ,
							  NULL,
							  OPEN_EXISTING,
							  FILE_ATTRIBUTE_NORMAL,
							  NULL);

	if (hDisk == INVALID_HANDLE_VALUE)
	{
		printf("CreateFile() failed with error code:%d\n",GetLastError());

		return FALSE;
	}

	DWORD dwBytesReturned = 0;
	GETVERSIONINPARAMS gvopVersionParams = {0};
	BOOL bRet = DeviceIoControl(hDisk,
								SMART_GET_VERSION,
								NULL,
								0,
								&gvopVersionParams,
								sizeof(gvopVersionParams),
								&dwBytesReturned,
								NULL);

	if (!bRet)
	{
		printf("DeviceIoControl - SMART_GET_VERSION failed with error code:%d\n",GetLastError());

		CloseHandle(hDisk);

		return FALSE;
	}

	if (gvopVersionParams.bIDEDeviceMap == 0)
	{
		printf("It's not IDE device\n");

		CloseHandle(hDisk);

		return FALSE;
	}

	// IDE or ATAPI IDENTIFY cmd
	BYTE btIDCmd;
	SENDCMDINPARAMS inParams;
	BYTE nDrive =0;
	btIDCmd = (gvopVersionParams.bIDEDeviceMap >> nDrive & 0x10) ? IDE_ATAPI_IDENTIFY : IDE_ATA_IDENTIFY;

	// output structure
	BYTE outParams[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];   // + 512 - 1

	//fill in the input buffer
	inParams.cBufferSize = 0;           //or IDENTIFY_BUFFER_SIZE ?
	inParams.irDriveRegs.bFeaturesReg = READ_ATTRIBUTES;
	inParams.irDriveRegs.bSectorCountReg = 1;
	inParams.irDriveRegs.bSectorNumberReg = 1;
	inParams.irDriveRegs.bCylLowReg = 0;
	inParams.irDriveRegs.bCylHighReg = 0;

	inParams.irDriveRegs.bDriveHeadReg = (nDrive & 1) ? 0xB0 : 0xA0;
	inParams.irDriveRegs.bCommandReg = btIDCmd;
	//inParams.bDriveNumber = nDrive;

	//get the attributes
	bRet = DeviceIoControl(
		hDisk, 
		SMART_RCV_DRIVE_DATA,
		&inParams,
		sizeof(SENDCMDINPARAMS) - 1,
		outParams,
		sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
		&dwBytesReturned,
		NULL);
	if (!bRet)        //fail
	{
		printf("DeviceIoControl - SMART_RCV_DRIVE_DATA failed with error code:%d\n",GetLastError());

		CloseHandle(hDisk);
		return FALSE;
	}

	WORD *pIDSector; // 对应结构IDSECTOR，见头文件
	pIDSector = (WORD *)(((SENDCMDOUTPARAMS*)outParams)->bBuffer);      //lint !e826
	
	memcpy(m_byHDDSerialNumber,pIDSector+10,HDD_SN_LEN);
	
	CloseHandle(hDisk);

	return TRUE;
	
}

TCHAR * CLisence::ConvertSENDCMDOUTPARAMSBufferToString( const DWORD *dwDiskData, DWORD nFirstIndex, DWORD nLastIndex )
{
	static TCHAR szResBuf[IDENTIFY_BUFFER_SIZE];     //512
	DWORD nIndex = 0;
	DWORD nPosition = 0;

	for (nIndex = nFirstIndex; nIndex <= nLastIndex; nIndex++)
	{
		// get high byte
		szResBuf[nPosition] = (TCHAR)(dwDiskData[nIndex] >> 8);
		nPosition++;

		// get low byte
		szResBuf[nPosition] = (TCHAR)(dwDiskData[nIndex] & 0xff);
		nPosition++;
	}

	// End the string
	szResBuf[nPosition] = _T('\0');

	return szResBuf;
}

BYTE * CLisence::GetLock()
{
	
	GetMacAddress();
	GetCPUID();
	GetHDDSerialNumber();

	MD5 md5;
	md5.update((void *)m_byMac,MAC_LEN);
	md5.update((void *)m_byCPUID,CPUID_LEN);
	md5.update((void *)m_byHDDSerialNumber,HDD_SN_LEN);

	memcpy(m_byLock,md5.digest(),LOCK_LEN);

	return m_byLock;
}

BYTE * CLisence::GetKey()
{
	CCRC32 crc;
	crc.update((void *)m_byLock,LOCK_LEN);

	CChecksum32 checksum;
	checksum.update((void *)m_byLock,LOCK_LEN);

	MD5 md5;
	md5.update((void *)m_byLock,LOCK_LEN);

	memcpy(m_byKey,crc.digest(),4);
	memcpy(m_byKey+4,checksum.digest(),4);
	memcpy(m_byKey + 8,md5.digest(),16);

	int loop = KEY_LEN / 24; //21
	int remain = KEY_LEN % 24; //8

	for (int i = 1;i < loop;i++)
	{
		crc.update((void *)m_byKey,KEY_LEN);
		checksum.update((void *)m_byKey,KEY_LEN);
		md5.update((void *)m_byKey,KEY_LEN);

		memcpy(m_byKey + i*24,crc.digest(),4);
		memcpy(m_byKey + i*24 + 4,checksum.digest(),4);
		memcpy(m_byKey + i*24 + 8,md5.digest(),16);
	}
	time_t t = time(NULL);
	memcpy(m_byKey + loop * 24,&t,8);

	return m_byKey;
}

BYTE * CLisence::GetKey( BYTE *byLock,size_t len )
{
	CCRC32 crc;
	crc.update((void *)byLock,len);

	CChecksum32 checksum;
	checksum.update((void *)byLock,len);

	MD5 md5;
	md5.update((void *)byLock,len);

	memcpy(m_byKey,crc.digest(),4);
	memcpy(m_byKey+4,checksum.digest(),4);
	memcpy(m_byKey + 8,md5.digest(),16);

	int loop = KEY_LEN / 24; //21
	int remain = KEY_LEN % 24; //8

	for (int i = 1;i < loop;i++)
	{
		crc.update((void *)m_byKey,KEY_LEN);
		checksum.update((void *)m_byKey,KEY_LEN);
		md5.update((void *)m_byKey,KEY_LEN);

		memcpy(m_byKey + i*24,crc.digest(),4);
		memcpy(m_byKey + i*24 + 4,checksum.digest(),4);
		memcpy(m_byKey + i*24 + 8,md5.digest(),16);
	}
	time_t t = time(NULL);
	memcpy(m_byKey + loop * 24,&t,8);

	return m_byKey;
}

BYTE * CLisence::GetKeyFromFile( char *sFileName )
{
	CCRC32 crc;
	crc.update(sFileName);

	CChecksum32 checksum;
	checksum.update(sFileName);

	MD5 md5;
	md5.update(sFileName);

	memcpy(m_byKey,crc.digest(),4);
	memcpy(m_byKey+4,checksum.digest(),4);
	memcpy(m_byKey + 8,md5.digest(),16);

	int loop = KEY_LEN / 24; //21
	int remain = KEY_LEN % 24; //8

	for (int i = 1;i < loop;i++)
	{
		crc.update((void *)m_byKey,KEY_LEN);
		checksum.update((void *)m_byKey,KEY_LEN);
		md5.update((void *)m_byKey,KEY_LEN);

		memcpy(m_byKey + i*24,crc.digest(),4);
		memcpy(m_byKey + i*24 + 4,checksum.digest(),4);
		memcpy(m_byKey + i*24 + 8,md5.digest(),16);
	}
	time_t t = time(NULL);
	memcpy(m_byKey + loop * 24,&t,8);

	return m_byKey;
}

