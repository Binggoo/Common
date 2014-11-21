#include "StdAfx.h"
#include "SerialPort.h"
#include <assert.h>


CSerialPort::CSerialPort( bool bAutoBeginThread /*= true*/, DWORD dwIOMode /*= FILE_FLAG_OVERLAPPED*/ )
	: m_dwIOMode(dwIOMode), m_bAutoBeginThread(bAutoBeginThread)
{
	Init(); 
}


CSerialPort::~CSerialPort(void)
{
	Close(); 
	UnInit();
}

bool CSerialPort::Open( DWORD dwPort )
{
	return Open(dwPort, 9600);
}

bool CSerialPort::Open( DWORD dwPort, DWORD dwBaudRate )
{
	if(dwPort < 1 || dwPort > 1024)
	{
		return false;
	}

	BindCommPort(dwPort);

	if(!OpenCommPort())
	{
		return false;
	}

	if(!SetupPort())
	{
		return false;
	}

	return SetState(dwBaudRate);
}

bool CSerialPort::Open( DWORD dwPort, TCHAR *szSetStr )
{
	if(dwPort < 1 || dwPort > 1024)
	{
		return false;
	}

	BindCommPort(dwPort);

	if(!OpenCommPort())
	{
		return false;
	}

	if(!SetupPort())
		return false;

	return SetState(szSetStr);
}

bool CSerialPort::IsOpen()
{
	return m_hCommHandle != INVALID_HANDLE_VALUE;
}

HANDLE CSerialPort::GetHandle()
{
	return m_hCommHandle;
}

bool CSerialPort::SetState( TCHAR *szSetStr )
{
	if(IsOpen())
	{
		if(!::GetCommState(m_hCommHandle, &m_DCB))
		{
			m_dwErrorCode = GetLastError();
			return false;
		}
		if(!::BuildCommDCB(szSetStr, &m_DCB))
		{
			m_dwErrorCode = GetLastError();
			return false;
		}
		return ::SetCommState(m_hCommHandle, &m_DCB) == TRUE;
	}
	return false;
}

bool CSerialPort::SetState( DWORD dwBaudRate, BYTE byByteSize /*= 8*/, BYTE byParity /*= NOPARITY*/, BYTE byStopBits /*= ONESTOPBIT*/ )
{
	if(IsOpen())
	{
		if(!::GetCommState(m_hCommHandle, &m_DCB))
		{
			m_dwErrorCode = GetLastError();
			return false;
		}
		m_DCB.BaudRate = dwBaudRate;
		m_DCB.ByteSize = byByteSize;
		m_DCB.Parity   = byParity;
		m_DCB.StopBits = byStopBits;

		if (::SetCommState(m_hCommHandle, &m_DCB))
		{
			return true;
		}
		else
		{
			m_dwErrorCode = GetLastError();
			return false;
		}
	}
	return false;
}

bool CSerialPort::SetBufferSize( DWORD dwInputSize, DWORD dwOutputSize )
{
	if(IsOpen())
	{
		if (::SetupComm(m_hCommHandle, dwInputSize, dwOutputSize))
		{
			return true;
		}
		else
		{
			m_dwErrorCode = GetLastError();
			return false;
		}
	}
	return false;
}

DWORD CSerialPort::Read( char *szBuffer, DWORD dwBufferLength, DWORD dwWaitTime /*= 20*/ )
{
	if(!IsOpen())
	{
		return 0;
	}

	szBuffer[0] = '\0';

	COMSTAT  Stat;
	DWORD dwError;

	if(::ClearCommError(m_hCommHandle, &dwError, &Stat) && dwError > 0) //清除错误
	{
		::PurgeComm(m_hCommHandle, PURGE_RXABORT | PURGE_RXCLEAR); /*清除输入缓冲区*/
		return 0;
	}

	if(!Stat.cbInQue)// 缓冲区无数据
	{
		return 0;
	}

	unsigned long uReadLength = 0;

	dwBufferLength = dwBufferLength - 1 > Stat.cbInQue ? Stat.cbInQue : dwBufferLength - 1;

	if(!::ReadFile(m_hCommHandle, szBuffer, dwBufferLength, &uReadLength, &m_ReadOverlapped)) //2000 下 ReadFile 始终返回 True
	{
		if(::GetLastError() == ERROR_IO_PENDING) // 结束异步I/O
		{
			WaitForSingleObject(m_ReadOverlapped.hEvent, dwWaitTime); //等待20ms
			if(!::GetOverlappedResult(m_hCommHandle, &m_ReadOverlapped, &uReadLength, false))
			{
				if(::GetLastError() != ERROR_IO_INCOMPLETE)//其他错误
				{
					uReadLength = 0;
				}
			}
		}
		else
		{
			uReadLength = 0;
		}
	}

	szBuffer[uReadLength] = '\0';
	return uReadLength;
}

DWORD CSerialPort::Write( char *szBuffer, DWORD dwBufferLength )
{
	if(!IsOpen())
	{
		return 0;
	}

	DWORD dwError;

	if(::ClearCommError(m_hCommHandle, &dwError, NULL) && dwError > 0) //清除错误
	{
		::PurgeComm(m_hCommHandle, PURGE_TXABORT | PURGE_TXCLEAR); 
	}

	unsigned long uWriteLength = 0;

	if(!::WriteFile(m_hCommHandle, szBuffer, dwBufferLength, &uWriteLength, &m_WriteOverlapped))
	{
		if(::GetLastError() != ERROR_IO_PENDING)
		{
			uWriteLength = 0;
		}
	}

	return uWriteLength;
}

DWORD CSerialPort::Write( char *szBuffer )
{
	assert(szBuffer);

	return Write(szBuffer, strlen(szBuffer));
}

DWORD CSerialPort::Write( char *szBuffer, DWORD dwBufferLength, char * szFormat, ... )
{
	if(!IsOpen())
	{
		return 0;
	}

	DWORD dwError;

	if(::ClearCommError(m_hCommHandle, &dwError, NULL) && dwError > 0) //清除错误
	{
		::PurgeComm(m_hCommHandle, PURGE_TXABORT | PURGE_TXCLEAR); 
	}

	va_list va;
	va_start(va, szFormat);
	_vsnprintf(szBuffer, dwBufferLength, szFormat, va);
	va_end(va);

	unsigned long uWriteLength = 0;

	if(!::WriteFile(m_hCommHandle, szBuffer, dwBufferLength, &uWriteLength, &m_WriteOverlapped))
	{
		if(::GetLastError() != ERROR_IO_PENDING)
		{
			uWriteLength = 0;
		}
	}

	return uWriteLength;
}

DWORD CSerialPort::WriteSync( char *szBuffer, DWORD dwBufferLength )
{
	if(!IsOpen())
	{
		return 0;
	}

	DWORD dwError;

	if(::ClearCommError(m_hCommHandle, &dwError, NULL) && dwError > 0) //清除错误
	{
		::PurgeComm(m_hCommHandle, PURGE_TXABORT | PURGE_TXCLEAR); 
	}

	unsigned long uWriteLength = 0;

	if(!::WriteFile(m_hCommHandle, szBuffer, dwBufferLength, &uWriteLength, NULL))
	{
		if(::GetLastError() != ERROR_IO_PENDING)
		{
			uWriteLength = 0;
		}
	}

	return uWriteLength;
}

DWORD CSerialPort::GetInBufferLength()
{
	if(!IsOpen())
	{
		return 0;
	}

	COMSTAT  Stat;
	DWORD dwError;

	if(::ClearCommError(m_hCommHandle, &dwError, &Stat) && dwError > 0) //清除错误
	{
		::PurgeComm(m_hCommHandle, PURGE_RXABORT | PURGE_RXCLEAR); /*清除输入缓冲区*/
		return 0;
	}

	return Stat.cbInQue;
}

void CSerialPort::Close()
{
	if(IsOpen())  
	{
		EndThread();
		::CloseHandle(m_hCommHandle);

		m_hCommHandle = INVALID_HANDLE_VALUE;
	}
}

void CSerialPort::SetWnd( HWND hWnd )
{
	m_hNotifyWnd = hWnd;
}

bool CSerialPort::BeginThread()
{
	if(!IsThreadRunning()) 
	{
		m_bRunFlag = true;
		m_hThreadHandle = NULL;

		DWORD id;

		m_hThreadHandle = ::CreateThread(NULL, 0, CommThreadProc, this, 0, &id); //兼容98 故使用&ID

		return (m_hThreadHandle != NULL); //辅助线程
	}
	return false;
}

bool CSerialPort::IsThreadRunning()
{
	return m_hThreadHandle != NULL;
}

HANDLE CSerialPort::GetThread()
{
	return m_hThreadHandle;
}

bool CSerialPort::SuspendThread()
{
	return IsThreadRunning() ? ::SuspendThread(m_hThreadHandle) != 0xFFFFFFFF : false;
}

bool CSerialPort::ResumeThread()
{
	return IsThreadRunning() ? ::ResumeThread(m_hThreadHandle) != 0xFFFFFFFF : false;
}

bool CSerialPort::EndThread( DWORD dwWaitTime /*= 100*/ )
{
	if(IsThreadRunning()) 
	{
		m_bRunFlag = false;
		::SetCommMask(m_hCommHandle, 0);
		::SetEvent(m_WaitOverlapped.hEvent);
		if(::WaitForSingleObject(m_hThreadHandle, dwWaitTime) != WAIT_OBJECT_0)
		{
			if(!::TerminateThread(m_hThreadHandle, 0))
			{
				return false;
			}
		}

		::CloseHandle(m_hThreadHandle);
		::ResetEvent(m_WaitOverlapped.hEvent);

		m_hThreadHandle = NULL;

		return true;
	}
	return false;
}

void CSerialPort::OnReceive()
{
	if(m_hNotifyWnd)
	{
		::PostMessage(m_hNotifyWnd, ON_COM_RECEIVE, WPARAM(m_dwPort), LPARAM(0));
	}
}

void CSerialPort::Init()
{
	memset(m_szCommStr, 0, 20);
	memset(&m_DCB, 0, sizeof(m_DCB));
	m_DCB.DCBlength = sizeof(m_DCB);
	m_hCommHandle = INVALID_HANDLE_VALUE;

	memset(&m_ReadOverlapped, 0, sizeof(m_ReadOverlapped));
	memset(&m_WriteOverlapped, 0, sizeof(m_WriteOverlapped));

	m_ReadOverlapped.hEvent = ::CreateEvent(NULL, true, false, NULL);
	assert(m_ReadOverlapped.hEvent != INVALID_HANDLE_VALUE); 

	m_WriteOverlapped.hEvent = ::CreateEvent(NULL, true, false, NULL);
	assert(m_WriteOverlapped.hEvent != INVALID_HANDLE_VALUE);

	m_dwNotifyNum = 0;
	m_hNotifyWnd = NULL;
	m_hThreadHandle = NULL;

	memset(&m_WaitOverlapped, 0, sizeof(m_WaitOverlapped));
	m_WaitOverlapped.hEvent = ::CreateEvent(NULL, true, false, NULL);
	assert(m_WaitOverlapped.hEvent != INVALID_HANDLE_VALUE); 
}

void CSerialPort::UnInit()
{
	if(m_ReadOverlapped.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_ReadOverlapped.hEvent);
	}

	if(m_WriteOverlapped.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_WriteOverlapped.hEvent);
	}

	if(m_WaitOverlapped.hEvent != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_WaitOverlapped.hEvent);
	}
}

void CSerialPort::BindCommPort( DWORD dwPort )
{
	assert(dwPort >= 1 && dwPort <= 1024);

	m_dwPort = dwPort;
	_stprintf(m_szCommStr, _T("\\\\.\\COM%d"),dwPort); 
}

bool CSerialPort::OpenCommPort()
{
	if(IsOpen())
	{
		Close();
	}

	m_hCommHandle = ::CreateFile(
		m_szCommStr,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | m_dwIOMode, 
		NULL
		);

	if (m_hCommHandle == INVALID_HANDLE_VALUE)
	{
		m_dwErrorCode = GetLastError();
		return false;
	}

	if(m_bAutoBeginThread)
	{
		if(BeginThread())
		{
			return true;
		}
		else
		{
			m_dwErrorCode = GetLastError();
			Close(); //创建线程失败
			return false;
		}
	}
	return IsOpen();//检测串口是否成功打开
}

bool CSerialPort::SetupPort()
{
	if(!IsOpen())
	{
		return false;
	}

	//设置推荐缓冲区
	if(!::SetupComm(m_hCommHandle, 4096, 4096))
	{
		m_dwErrorCode = GetLastError();
		return false; 
	}

	//设置超时时间
	if(!::GetCommTimeouts(m_hCommHandle, &m_CO))
	{
		m_dwErrorCode = GetLastError();
		return false;
	}
	m_CO.ReadIntervalTimeout = 0xFFFFFFFF;
	m_CO.ReadTotalTimeoutMultiplier = 0;
	m_CO.ReadTotalTimeoutConstant = 0;
	m_CO.WriteTotalTimeoutMultiplier = 0;
	m_CO.WriteTotalTimeoutConstant = 2000;
	if(!::SetCommTimeouts(m_hCommHandle, &m_CO))
	{
		m_dwErrorCode = GetLastError();
		return false; 
	}

	//清空串口缓冲区
	if(!::PurgeComm(m_hCommHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR ))
	{
		m_dwErrorCode = GetLastError();
		return false; 
	}

	return true;
}

DWORD WINAPI CSerialPort::CommThreadProc( LPVOID lpPara )
{
	CSerialPort *pComm = (CSerialPort *)lpPara; 


	if(!::SetCommMask(pComm->m_hCommHandle, EV_RXCHAR | EV_ERR))
	{
		return 1;
	}

	COMSTAT Stat;
	DWORD dwError;

	for(DWORD dwLength, dwMask = 0; pComm->m_bRunFlag && pComm->IsOpen(); dwMask = 0)
	{
		if(!::WaitCommEvent(pComm->m_hCommHandle, &dwMask, &pComm->m_WaitOverlapped))
		{
			if(::GetLastError() == ERROR_IO_PENDING)
			{
				::GetOverlappedResult(pComm->m_hCommHandle, &pComm->m_WaitOverlapped, &dwLength, TRUE);
			}
		}

		if(dwMask & EV_ERR) // == EV_ERR
			::ClearCommError(pComm->m_hCommHandle, &dwError, &Stat);

		if(dwMask & EV_RXCHAR) // == EV_RXCHAR
		{
			::ClearCommError(pComm->m_hCommHandle, &dwError, &Stat);
			if(Stat.cbInQue > pComm->m_dwNotifyNum)
			{
				pComm->OnReceive();
			}
		}
	}
	return 0;
}

void CSerialPort::SetNotifyNum( int iNum )
{
	m_dwNotifyNum = iNum;
}

DWORD CSerialPort::GetErrorCode()
{
	return m_dwErrorCode;
}
