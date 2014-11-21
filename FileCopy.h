/**************************************************************************
�ļ�����CopyFile.h
�ļ�˵������FileCopyͷ�ļ�
��Ҫ˵������װCopyFileEx������ʵ���ļ����ƹ��̵���ͣ�����٣��첽��ͬ���������µ�
	�̣߳��������е���CopyFileEx����������CopyFileEx�Ļص�����ʵ����ͣ�������ٶȣ�
	��ȡ���ȵȹ��ܡ�
������ڣ�21:14 2011/10/4
��ע�����벻�����ƣ�û�������߳�ͬ����������ʱ����ȥ�Ľ���
**************************************************************************/

#pragma once
#include <windows.h>
#include <tchar.h>

#define  WM_COPYFILE_NOTIFY WM_USER + 118+2  //�Զ����windows��Ϣ������֪ͨ����

class FileCopy
{
private:
	LPCTSTR m_lpExistingPathName;			//Դ�ļ�
	LPCTSTR m_lpNewPathName;				//Ŀ���ļ�
	int m_iSpeedControl;					//�ٶȿ��Ƶı���
	BOOL m_bCancel;						//ȡ����־����������CopyFileEx�Ļص�����
	HANDLE  m_hEventPause;				//��������ͣ���¼�
	float m_fCopyProgress;				//���ƽ���
	HWND m_hNotifyWnd;					//����֪ͨ��Ϣ�Ĵ���

	HANDLE m_hEventEnd;					//�����ƽ������¼�

	HANDLE m_hThreadCopy;				//�߳̾��

	LARGE_INTEGER m_liTotalFileSize;		//�ܵ��ļ�����	
	LARGE_INTEGER m_liTotalBytesTransferred;	//�Ѿ����Ƶ��ֽ���

	int m_iRetPGR;						//��ΪCopyProgressRoutine�ķ���ֵ���˲���δ��

	BOOL m_bSuccess;
	DWORD m_dwErrorCode;


	void Initialize();   //��ʼ���ڲ����ݣ����־���ͱ���;

	//�̺߳��������߳��е���CopyFileExʵ���ļ�����
	static DWORD WINAPI CopyThreadProc(LPVOID lpParam);


	//CopyFileEx�Ļص��������ڴ˺�����ʵ���ļ����ƹ��̵Ŀ��ơ�
	static DWORD CALLBACK CopyProgressRoutine(
		LARGE_INTEGER TotalFileSize,
		LARGE_INTEGER TotalBytesTransferred,
		LARGE_INTEGER StreamSize,
		LARGE_INTEGER StreamBytesTransferred,
		DWORD dwStreamNumber,
		DWORD dwCallbackReason,
		HANDLE hSourceFile,
		HANDLE hDestinationFile,
		LPVOID lpData
		);

public:
	FileCopy(void);

	//�����ڹ��캯���г�ʼ������
	FileCopy(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName,HWND hNotifyWnd=NULL);
	~FileCopy(void);

	//��ʼ����Ҫ�Ĳ�����Դ�ļ���Ŀ���ļ�
	BOOL Init(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName,HWND hNotifyWnd=NULL);
	BOOL Init(HWND hNotifyWnd=NULL);

	BOOL SetCopyFileName(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName);

	//��ʼ��������
	BOOL Begin();
	//��ͣ����
	void Pause();
	//�ָ�����
	void Resume();
	//ȡ������
	void Cancel();
	//ֹͣ����
	//void Stop();     //Stop();�������ƹ��̣��������Ѿ����Ƶ����ݣ�Cancel();��ɾ���Ѹ��Ƶ����ݡ�

	//�ȴ����ƽ���������ʵ�֡�ͬ����Ч��,���ô˺������̻߳�������ֱ��������ɻ�ȡ����
	void WaitForEnd();
	
	//��ȡ���ƽ���
	float GetProgress();
	//��ȡ�ļ��ܴ�С���������ط�ʽģ�� API GetFileSize();   һ������³���4GB���ļ�����
	//��lpFileSizeHighֱ�Ӻ��Ӿ�����
	DWORD GetTotalFileSize(DWORD* lpFileSizeHigh=NULL);
	//��ȡ�Ѿ����Ƶ��ֽ���;
	DWORD GetBytesTransferred(DWORD* lpTransferredHigh=NULL);

	//���ø����ٶ�
	void SetSpeed(int iSpeed);

	void SetSuccess(BOOL bSuccess);
	BOOL IsSuccess();

	void SetErrorCode(DWORD dwErrorCode);
	DWORD GetErrorCode();
};

