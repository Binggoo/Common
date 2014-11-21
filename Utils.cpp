#include "StdAfx.h"
#include "Utils.h"
#pragma comment(lib,"version.lib")

CCriticalSection m_CSLog;

CUtils::CUtils(void)
{
}


CUtils::~CUtils(void)
{
}

char * CUtils::UTF8_TO_ANSI( const char *szUTF8 )
{
	if (szUTF8 == NULL)
	{
		return NULL;
	}

	int nLen = MultiByteToWideChar(CP_UTF8, 0, szUTF8, -1, NULL, 0) ;
	WCHAR *pWstr = new WCHAR[nLen+1] ;
	ZeroMemory(pWstr,sizeof(WCHAR) * (nLen+1)) ;
	MultiByteToWideChar(CP_UTF8, 0, szUTF8, -1, pWstr, nLen) ;
	char *pBufer = _bstr_t((wchar_t *)pWstr);
	delete[] pWstr;
	return pBufer;
}

char * CUtils::ANSI_TO_UTF8( const char *szAnsi )
{
	if (szAnsi == NULL)
	{
		return NULL;
	}

	_bstr_t bstrTmp(szAnsi);
	int nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)bstrTmp, -1, NULL, 0, NULL, NULL);

	char *pUTF8 = new char[nLen+1];
	ZeroMemory (pUTF8, nLen + 1);
	WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)bstrTmp, -1, pUTF8, nLen, NULL, NULL);

	return pUTF8;
}

void CUtils::ANSI_TO_UTF8( const char *szSrc,char *szDest,int dlen )
{
	if (!szSrc || !szDest)
	{
		return;
	}
	int nSrc = strlen(szSrc);
	DWORD nwLen = MultiByteToWideChar(CP_ACP,0,szSrc,nSrc,NULL,0); //计算字节数
	WCHAR *pwsBuf = new WCHAR[nwLen+1];
	ZeroMemory(pwsBuf,sizeof(WCHAR) * (nwLen+1));

	MultiByteToWideChar(CP_ACP,0,szSrc,nSrc,pwsBuf,nwLen); //CHAR TO WCHAR
	WideCharToMultiByte(CP_UTF8,0,pwsBuf,-1,szDest,dlen,NULL,NULL); //wchar to utf8
	delete []pwsBuf;
}

char * CUtils::ANSI_TO_UTF8( const void *pAnsi,unsigned int *len )
{
	if (!pAnsi || !len)
	{
		return NULL;
	}

	DWORD nwLen = MultiByteToWideChar(CP_ACP,0,(CHAR *)pAnsi,*len,NULL,0); //计算字节数
	WCHAR *pwsBuf = new WCHAR[nwLen+1];
	if (!pwsBuf)
	{
		return NULL;
	}

	ZeroMemory(pwsBuf,sizeof(WCHAR) * (nwLen+1));
	MultiByteToWideChar(CP_ACP,0,(CHAR *)pAnsi,*len,pwsBuf,nwLen); //CHAR TO WCHAR

	int nLen = WideCharToMultiByte(CP_UTF8,0,pwsBuf,-1,NULL,0,NULL,NULL); //
	char *pUTF8 = new char[nLen+1];
	ZeroMemory(pUTF8,nLen + 1);
	WideCharToMultiByte(CP_UTF8,0,pwsBuf,-1,pUTF8,nLen,NULL,NULL); //WCHAR TO UTF8
	*len = nLen;
	delete []pwsBuf;

	return pUTF8;
}

int CUtils::utf8_to_utf16( const BYTE *s,const USHORT ns,WCHAR *d,const USHORT nd )
{
	if (!s)
	{
		return -1;
	}

	int need = 0;
	int j = 0;
	try
	{
		for (int i = 0;i < ns;i++) //UTF8的长度编码有时是3个字节,有时2个字节,有时1个字节
		{
			if (s[i] >= 0 && s[i] <= 0x7f)
			{
			}else if (s[i] >= 0xC0 && s[i] <= 0xDF)
			{
				i += 1;
			}else if (s[i] >= 0xE0 && s[i] <= 0xEF)
			{
				i += 2;
			}
			need++;
		}

		if (!d) //如果传入的dest为NULL，返回所需要大小字节
			return need;

		for (int i = 0;i < ns;i++,j++)
		{
			if (j >= nd) 
				return j;
			if ((s[i] >= 0) && (s[i] <= 0x7f))
			{
				d[j] = s[i];
			}else if ((s[i] >= 0xC0) && (s[i] <= 0xDF))
			{
				//111 11111111
				USHORT u = 0;
				u = (s[i] - 0xC0) << 8;
				u |= (s[i+1] - 0x80);
				i += 1;
				d[j] = u;
			}else if ((s[i] >= 0xE0) && (s[i] <= 0xEF))
			{
				//11111111 11111111
				USHORT u = 0;
				u = (s[i] - 0xE0) << 12;
				u |= (s[i+1] - 0x80) << 6;
				u |= (s[i+2] - 0x80);
				i += 2;
				d[j] = u;
			}
		}
	}catch(CException *e)
	{
		e->ReportError();
	}
	return j;
}

int CUtils::utf16_to_utf8( const WCHAR *s,const USHORT ns,BYTE *d,const USHORT nd )
{
	if (!s)
	{
		return -1;
	}

	int j = 0;
	int need = 0;
	try
	{
		for (int i = 0;i < ns;i++)
		{
			if (s[i] >= 0 && s[i] <= 0x7f)
			{
				need += 1;
			}else if (s[i] >= 0x80 && s[i] <= 0x7FF)	
			{
				need += 2;
			}else if (s[i] >= 0x800 && s[i] <= 0xFFFF)
			{
				need += 3;
			}
		}

		if (!d)
		{
			return need;
		}

		for (int i = 0;i < ns;i++,j++)
		{
			if (j >= nd)
			{
				return j;
			}

			if (s[i] >= 0 && s[i] <= 0x7f)
			{
				d[j] = s[i] & 0x7F; //0x7F 111 1111 获取低7位
			}else if (s[i] >= 0x80 && s[i] <= 0x7FF)	
			{
				//111 11111111
				unsigned char u = 0,k = 0;
				u = s[i] << 8;
				u |= s[i+1]; //先搞成一个整体,因为 110xxxxx 10xxxxxx
				k = u >> 6; //获取高5位
				k |= 0xC0;
				d[j] = k;
				k = u & 0x3F; //111 11111111 获取低6位
				if (j < nd)
					d[j+1] = k;
				else
					break;
				j += 1;
			}else if (s[i] >= 0x800 && s[i] <= 0xFFFF)
			{
				//11111111 11111111
				unsigned char u = 0;
				u = s[i] >> 12;
				u |= 0xE0;
				d[j] = u;
				u = (s[i] >> 6) & 0x3F;//0x3F 111111
				u |= 0x80;
				if (j < nd - 1)
					d[j+1] = u;
				else
					break;
				u = s[i] & 0x3F;
				u |= 0x80;
				d[j+2] = u;
				j += 2;
			}
		}
	}catch(CException *e)
	{
		e->ReportError();
	}
	return j;
}

CString CUtils::GetAppPath()
{
	CString strPath;
	GetModuleFileName(NULL,strPath.GetBuffer(MAX_PATH),MAX_PATH);
	strPath.ReleaseBuffer();

	return strPath;
}

CString CUtils::GetAppVersion( const CString strPath )
{
	DWORD dwVerInfoSize = 0;
	DWORD dwVerHnd;
	VS_FIXEDFILEINFO * pFileInfo = NULL;
	CString strVersion ;
	dwVerInfoSize = GetFileVersionInfoSize(strPath, &dwVerHnd);
	if (dwVerInfoSize)
	{
		// If we were able to get the information, process it:
		HANDLE	 hMem;
		LPVOID	 lpvMem;
		unsigned int	uInfoSize = 0;
		hMem = GlobalAlloc(GMEM_MOVEABLE, dwVerInfoSize);
		lpvMem = GlobalLock(hMem);
		GetFileVersionInfo(strPath, dwVerHnd, dwVerInfoSize, lpvMem);
		::VerQueryValue(lpvMem, (LPTSTR)_T("\\"), (void**)&pFileInfo, &uInfoSize);
		WORD wProdVersion[4];
		// Product version from the FILEVERSION of the version info resource
		wProdVersion[0] = HIWORD(pFileInfo->dwProductVersionMS);
		wProdVersion[1] = LOWORD(pFileInfo->dwProductVersionMS);
		wProdVersion[2] = HIWORD(pFileInfo->dwProductVersionLS);
		wProdVersion[3] = LOWORD(pFileInfo->dwProductVersionLS);
		strVersion.Format(_T("%d.%d.%d.%d"),wProdVersion[0],
			wProdVersion[1],wProdVersion[2],wProdVersion[3]);
		GlobalUnlock(hMem);
		GlobalFree(hMem);
	}
	return strVersion;
}

CString CUtils::GetFilePathWithoutName( const CString strFilePath )
{
	return strFilePath.Left(strFilePath.ReverseFind(_T('\\')));
}

CString CUtils::GetFileName( const CString strFilePath )
{
	return strFilePath.Right(strFilePath.GetLength() - strFilePath.ReverseFind(_T('\\')) - 1);
}

CString CUtils::GetFileTitle( const CString strFilePath )
{
	CString strFileName = GetFileName(strFilePath);

	int nPos = strFileName.ReverseFind(_T('.'));
	if (nPos == -1)
	{
		nPos = strFileName.GetLength();
	}

	return strFileName.Left(nPos);
}

CString CUtils::GetErrorMsg( DWORD dwErrorCode )
{
	CString strErrorMsg;
	LPTSTR lpMsgBuffer;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dwErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuffer,
		0, NULL );
	strErrorMsg = CString(lpMsgBuffer);
	LocalFree(lpMsgBuffer);
	strErrorMsg.Trim();
	return strErrorMsg;
}

void CUtils::WriteLogFile( HANDLE hFile,BOOL bTimestamp,LPCTSTR lpszFormat,... )
{
	TCHAR szBuffer[1024] = {NULL};
	va_list va;
	va_start(va, lpszFormat);
	_vsntprintf_s(szBuffer, 1024, lpszFormat, va);
	va_end(va);

	ASSERT(hFile != INVALID_HANDLE_VALUE);

	m_CSLog.Lock();
	CString strMsg(szBuffer);

	strMsg.Trim();

	if (bTimestamp)
	{
		CString strTimestamp;
		strTimestamp.Format(_T("<%04X|%s>"),GetCurrentThreadId(),CTime::GetCurrentTime().Format(_T("%Y-%m-%d %H:%M:%S")));
		strMsg = strTimestamp + strMsg;
	}

	//去除中间的换行
	strMsg.Replace(_T("\r\n"),_T(""));

	strMsg += _T("\r\n");

	SetFilePointer(hFile,0,NULL,FILE_END);

	DWORD dwWrite = 0;
#ifdef _UNICODE
	USES_CONVERSION;
	char *pBuf = W2A(strMsg);
	WriteFile(hFile,pBuf,strlen(pBuf),&dwWrite,NULL);
#else
	WriteFile(hFile,strMsg,strMsg.GetLength(),&dwWrite,NULL);
#endif
	
	m_CSLog.Unlock();
}

CString CUtils::GetDiskFreeAndTotalSize( CString strPath,PULARGE_INTEGER puiTotalSize,PULARGE_INTEGER puiUseSize )
{
	ULARGE_INTEGER   uiFreeBytesAvailableToCaller = {0}; 
	ULARGE_INTEGER   uiTotalNumberOfBytes = {0}; 
	ULARGE_INTEGER   uiTotalNumberOfFreeBytes = {0};
	CString strReturn;

	BOOL bOK = GetDiskFreeSpaceEx(strPath,&uiFreeBytesAvailableToCaller,&uiTotalNumberOfBytes,&uiTotalNumberOfFreeBytes);

	if (bOK)
	{
		CString strTotal,strUsed;
		ULARGE_INTEGER uiUsed;
		uiUsed.QuadPart = uiTotalNumberOfBytes.QuadPart - uiFreeBytesAvailableToCaller.QuadPart;

		*puiTotalSize = uiTotalNumberOfBytes;
		*puiUseSize = uiUsed;

		strTotal = AdjustFileSize(uiTotalNumberOfBytes.QuadPart);
		strUsed = AdjustFileSize(uiUsed.QuadPart);

		strReturn.Format(_T("Used %s / Total %s"),strUsed,strTotal);
	}
	else
	{
		DWORD dwLastError = GetLastError();
		strReturn = CUtils::GetErrorMsg(dwLastError);
	}

	return strReturn;
}

CString CUtils::AdjustFileSize( ULONGLONG ullFileSize )
{
	double dbSize = 0.0;
	CString strSize;

	if (ullFileSize/1024/1024/1024)
	{
		dbSize = ullFileSize/1024/1024/1024.0;
		strSize.Format(_T("%.1fGB"),dbSize);
	}
	else if (ullFileSize/1024/1024)
	{
		dbSize = ullFileSize/1024/1024.0;
		strSize.Format(_T("%.1fMB"),dbSize);
	}
	else if (ullFileSize/1024)
	{
		dbSize = ullFileSize/1024.;
		strSize.Format(_T("%.1fKB"),dbSize);
	}
	else
	{
		dbSize = ullFileSize/1.;
		strSize.Format(_T("%.1fB"),dbSize);
	}

	return strSize;
}

CString CUtils::AdjustSpeed( ULONGLONG ullSpeed )
{
	int speed = 0;
	CString strSpeed;

	if ((speed = int(ullSpeed/1024/1024/1024)))
	{
		strSpeed.Format(_T("%d GB/s"),speed);
	}
	else if ((speed = int(ullSpeed/1024/1024)))
	{
		strSpeed.Format(_T("%d MB/s"),speed);
	}
	else if ((speed = int(ullSpeed/1024)))
	{
		strSpeed.Format(_T("%d KB/s"),speed);
	}
	else
	{
		speed = (int)ullSpeed;
		strSpeed.Format(_T("%d B/s"),speed);
	}

	return strSpeed;
}


ULONGLONG CUtils::GetFolderSize( CString strPath )
{
	ULONGLONG ullFolderSize = 0,ullFileSize = 0;

	CString strFindFile = strPath + _T("\\*");
	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	while (bFind)
	{
		bFind = ff.FindNextFile();

		if (ff.GetFileName() == _T(".") || ff.GetFileName() == _T(".."))
		{
			continue;
		}

		if (ff.IsDirectory())
		{
			// 文件夹List
			ullFolderSize = GetFolderSize(ff.GetFilePath());
			ullFileSize += ullFolderSize;
		}
		else
		{
			ullFileSize += ff.GetLength();
		}

	}
	ff.Close();

	return ullFileSize;
}

BOOL CUtils::CreateAllDirectories( CString strDir )
{
	if(strDir.GetLength() <= 0)
	{
		return FALSE;
	}

	// remove ending / if exists
	if(strDir.Right(1) == _T("\\"))
	{
		strDir = strDir.Left(strDir.GetLength() - 1);
	}

	// base case . . .if directory exists
	//    if(PathFileExists(strDir)) return TRUE;
	DWORD dwRet = GetFileAttributes(strDir);
	if(dwRet != 0xFFFFFFFF)
	{
		return TRUE;
	}

	// recursive call, one less directory
	int nFound = strDir.ReverseFind('\\');

	CreateAllDirectories(strDir.Left(nFound));

	// actual work
	return CreateDirectory(strDir,NULL);
}

void CUtils::DeleteDirectory( LPCTSTR lpszPath )
{
	CFileFind ff;
	CString strTmpPath (lpszPath);
	strTmpPath += _T("\\*");
	BOOL bFound = ff.FindFile(strTmpPath, 0);
	while(bFound)
	{
		bFound = ff.FindNextFile();
		if ((ff.GetFileName() == _T(".")) || (ff.GetFileName() == _T("..")))
		{
			continue;
		}
		//
		SetFileAttributes(ff.GetFilePath(), FILE_ATTRIBUTE_NORMAL);
		if (ff.IsDirectory())
		{ //
			DeleteDirectory(ff.GetFilePath());
		}
		else
		{
			DeleteFile(ff.GetFilePath()); //
		}
	}
	ff.Close();
	RemoveDirectory(lpszPath);
}

void _DbgPrintA( char *szFormat,... )
{
	char szBuffer[1024] = {NULL};
	va_list va;
	va_start(va, szFormat);
	_vsnprintf(szBuffer, 1024, szFormat, va);
	va_end(va);

	OutputDebugStringA(szBuffer);
}

void _DbgPrintW( wchar_t *szFormat,... )
{
	TCHAR szBuffer[1024] = {NULL};
	va_list va;
	va_start(va, szFormat);
	_vsnwprintf(szBuffer, 1024, szFormat, va);
	va_end(va);

	OutputDebugStringW(szBuffer);
}
