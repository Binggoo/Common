/**************************************************************************
�ļ�����CopyFile.cpp
�ļ�˵������FileCopyʵ���ļ�����ϸ��Ϣ��FileCopy.h�ļ�
������ڣ�21:14 2011/10/4
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
	//����ò�����Ĳ����á���������-_-
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

	//��ʼ�� �����ƽ������¼�        �ֶ�����  ���ź�
	if(m_hEventEnd != NULL)
	{
		CloseHandle(m_hEventEnd);
	}
	m_hEventEnd = CreateEvent(NULL,TRUE,FALSE,NULL);

	//��ʼ�� ��������ͣ���¼���       �ֶ�����  ���ź�״̬
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
	//���߳��е���CopyFileEx����,Ϊ�˱�����ķ�װ�ԣ�
	//�̺߳�����д����ľ�̬��Ա�������˴�����thisָ��Ϊ�˷��ʳ�Ա����
	//CopyFileEx�Ļص�����Ҳ������������ʵ�ֵġ�
	m_hThreadCopy = CreateThread(NULL,0,CopyThreadProc,this,0,NULL);
	if(NULL == m_hThreadCopy)
	{
		return FALSE;
	}

	return TRUE;
}


DWORD WINAPI FileCopy::CopyThreadProc(LPVOID lpParam)
{
	//��õ�ǰ���ʵ���е��������
	HWND hNotifyWnd = ((FileCopy*)lpParam)->m_hNotifyWnd;
	LPCTSTR lpExistingPathName = ((FileCopy*)lpParam)->m_lpExistingPathName;
	LPCTSTR lpNewPathName = ((FileCopy*)lpParam)->m_lpNewPathName;

	//���ú���API����CopyFileEx�������ļ�
	BOOL bSucceed = CopyFileEx(lpExistingPathName,lpNewPathName,
							   CopyProgressRoutine,
							   lpParam,
							   &(((FileCopy*)lpParam)->m_bCancel),
							   COPY_FILE_ALLOW_DECRYPTED_DESTINATION|COPY_FILE_COPY_SYMLINK);

	DWORD dwErrorCode = GetLastError();
	((FileCopy*)lpParam)->SetSuccess(bSucceed);
	((FileCopy*)lpParam)->SetErrorCode(dwErrorCode);

	//�����������򴰿ڷ���֪ͨ��Ϣ��
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

	//���������������¼����ó��ź�״̬
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
	//�����ļ����Ⱥ��Ѿ����Ƶ�������
	((FileCopy*)lpData)->m_liTotalFileSize = TotalFileSize;
	((FileCopy*)lpData)->m_liTotalBytesTransferred = TotalBytesTransferred;

	//���㸴�ƽ���
	((FileCopy*)lpData)->m_fCopyProgress = TotalBytesTransferred.QuadPart*1.0/TotalFileSize.QuadPart;

	//ͨ���¼�����ʵ����ͣ��
	WaitForSingleObject(((FileCopy*)lpData)->m_hEventPause,INFINITE);

	//ͨ��Sleep()�����Ƹ����ٶ�
	int iSpeed = ((FileCopy*)lpData)->m_iSpeedControl;
	if(iSpeed >= 0)
	{
		Sleep(iSpeed);
	}

	//����0���������ƣ���ͨ��bCancel���Ƹ��ƽ������˷���ֵ��ʱδ��
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
	Resume();		//�ָ���ͣ״̬�����߳���Ȼ������
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
	//ÿ���߳�Sleep()��ʱ�䲻����1000ms
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
