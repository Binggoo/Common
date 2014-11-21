/**************************************************************************
文件名：CopyFile.cpp
文件说明：类FileCopy实现文件，详细信息见FileCopy.h文件
完成日期：21:14 2011/10/4
**************************************************************************/

#include "StdAfx.h"
#include "FileCopy.h"

FileCopy::FileCopy(void)
{
	m_hEventPause = NULL;
	m_hEventEnd = NULL;
	Initialize();
}

FileCopy::FileCopy(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName,HWND hNotifyWnd)
{
	Init(lpExistingPathName,lpNewPathName,hNotifyWnd);
}

FileCopy::~FileCopy(void)
{
	//这里貌似做的不够好。。。。。-_-
	CloseHandle(m_hEventEnd);
	CloseHandle(m_hEventPause);
	CloseHandle(m_hThreadCopy);
}

void FileCopy::Initialize()
{
	m_bCancel = FALSE;
	m_hNotifyWnd = NULL;
	m_fCopyProgress = 0;
	m_iSpeedControl = -1;
	m_liTotalFileSize.HighPart = 0;
	m_liTotalFileSize.LowPart = 0;
	m_liTotalBytesTransferred.HighPart = 0;
	m_liTotalBytesTransferred.LowPart = 0;
	m_hThreadCopy = NULL;

	m_iRetPGR = PROGRESS_CONTINUE;
	m_bSuccess = FALSE;
	m_dwErrorCode = 0;

	//初始化 “复制结束”事件        手动重置  无信号
	if(m_hEventEnd != NULL)
	{
		CloseHandle(m_hEventEnd);
	}
	m_hEventEnd = CreateEvent(NULL,TRUE,FALSE,NULL);

	//初始化 “复制暂停”事件，       手动重置  有信号状态
	if(m_hEventPause != NULL)
	{
		CloseHandle(m_hEventPause);
	}
	m_hEventPause = CreateEvent(NULL,TRUE,TRUE,NULL);
}

BOOL FileCopy::Init(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName,HWND hNotifyWnd/* =NULL */)
{
	Initialize();
	this->m_lpExistingPathName = lpExistingPathName;
	this->m_lpNewPathName = lpNewPathName;
	this->m_hNotifyWnd = hNotifyWnd;

	HANDLE hFile = CreateFile(lpExistingPathName,GENERIC_READ,
		FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,NULL);

	if(INVALID_HANDLE_VALUE == hFile)
	{
		return FALSE;
	}

	if(!GetFileSizeEx(hFile,&m_liTotalFileSize))
	{
		CloseHandle(hFile);
		return FALSE;
	}
	CloseHandle(hFile);
	return TRUE;
}

BOOL FileCopy::Init( HWND hNotifyWnd/*=NULL*/ )
{
	this->m_hNotifyWnd = hNotifyWnd;
	return TRUE;
}

BOOL FileCopy::Begin()
{
	//在线程中调用CopyFileEx函数,为了保持类的封装性，
	//线程函数被写成类的静态成员函数，此处传入this指针为了访问成员变量
	//CopyFileEx的回调函数也是类似于这样实现的。
	m_hThreadCopy = CreateThread(NULL,0,CopyThreadProc,this,0,NULL);
	if(NULL == m_hThreadCopy)
	{
		return FALSE;
	}

	return TRUE;
}


DWORD WINAPI FileCopy::CopyThreadProc(LPVOID lpParam)
{
	//获得当前类的实例中的相关数据
	HWND hNotifyWnd = ((FileCopy*)lpParam)->m_hNotifyWnd;
	LPCTSTR lpExistingPathName = ((FileCopy*)lpParam)->m_lpExistingPathName;
	LPCTSTR lpNewPathName = ((FileCopy*)lpParam)->m_lpNewPathName;

	//调用核心API函数CopyFileEx来复制文件
	BOOL bSucceed = CopyFileEx(lpExistingPathName,lpNewPathName,
							   CopyProgressRoutine,
							   lpParam,
							   &(((FileCopy*)lpParam)->m_bCancel),
							   COPY_FILE_ALLOW_DECRYPTED_DESTINATION|COPY_FILE_COPY_SYMLINK);

	DWORD dwErrorCode = GetLastError();
	((FileCopy*)lpParam)->SetSuccess(bSucceed);
	((FileCopy*)lpParam)->SetErrorCode(dwErrorCode);

	//拷贝结束，向窗口发送通知消息；
	if(hNotifyWnd != NULL)
	{
		if(bSucceed)
		{
			PostMessage(hNotifyWnd,WM_COPYFILE_NOTIFY,1,(LPARAM)lpExistingPathName);
		}
		else
		{
			PostMessage(hNotifyWnd,WM_COPYFILE_NOTIFY,0,(LPARAM)lpExistingPathName);
		}
	}

	//将“拷贝结束”事件设置成信号状态
	SetEvent(((FileCopy*)lpParam)->m_hEventEnd);
	return 0;
}

DWORD CALLBACK FileCopy::CopyProgressRoutine(
	LARGE_INTEGER TotalFileSize,
	LARGE_INTEGER TotalBytesTransferred,
	LARGE_INTEGER StreamSize,
	LARGE_INTEGER StreamBytesTransferred,
	DWORD dwStreamNumber,
	DWORD dwCallbackReason,
	HANDLE hSourceFile,
	HANDLE hDestinationFile,
	LPVOID lpData
	)
{
	//保存文件长度和已经复制的数据量
	((FileCopy*)lpData)->m_liTotalFileSize = TotalFileSize;
	((FileCopy*)lpData)->m_liTotalBytesTransferred = TotalBytesTransferred;

	//计算复制进度
	((FileCopy*)lpData)->m_fCopyProgress = TotalBytesTransferred.QuadPart*1.0/TotalFileSize.QuadPart;

	//通过事件对象实现暂停；
	WaitForSingleObject(((FileCopy*)lpData)->m_hEventPause,INFINITE);

	//通过Sleep()来控制复制速度
	int iSpeed = ((FileCopy*)lpData)->m_iSpeedControl;
	if(iSpeed >= 0)
	{
		Sleep(iSpeed);
	}

	//返回0，继续复制，以通过bCancel控制复制结束，此返回值暂时未用
	return PROGRESS_CONTINUE;
}

void FileCopy::Pause()
{
	ResetEvent(m_hEventPause);
}

void FileCopy::Resume()
{
	SetEvent(m_hEventPause);
}

void FileCopy::Cancel()
{
	m_bCancel = TRUE;
	Resume();		//恢复暂停状态，让线程自然结束！
}

void FileCopy::WaitForEnd()
{
	WaitForSingleObject(m_hEventEnd,INFINITE);
}

float FileCopy::GetProgress()
{
	return m_fCopyProgress;
}

DWORD FileCopy::GetTotalFileSize(DWORD* lpFileSizeHigh)
{
	if(lpFileSizeHigh)
	{
		*lpFileSizeHigh = m_liTotalFileSize.HighPart;
	}
	return m_liTotalFileSize.LowPart;
}

DWORD FileCopy::GetBytesTransferred(DWORD* lpTransferredHigh)
{
	if(lpTransferredHigh)
	{
		*lpTransferredHigh=m_liTotalBytesTransferred.HighPart;
	}
	return m_liTotalBytesTransferred.LowPart;
}

void FileCopy::SetSpeed(int iSpeed)
{
	m_iSpeedControl = iSpeed;
	//每次线程Sleep()的时间不超过1000ms
	if(m_iSpeedControl > 1000)
	{
		m_iSpeedControl = 1000;
	}
}

BOOL FileCopy::SetCopyFileName( LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName )
{
	return Init(lpExistingPathName,lpNewPathName,m_hNotifyWnd);
}

void FileCopy::SetSuccess( BOOL bSuccess )
{
	m_bSuccess = bSuccess;
}

BOOL FileCopy::IsSuccess()
{
	return m_bSuccess;
}

void FileCopy::SetErrorCode( DWORD dwErrorCode )
{
	m_dwErrorCode = dwErrorCode;
}

DWORD FileCopy::GetErrorCode()
{
	return m_dwErrorCode;
}
