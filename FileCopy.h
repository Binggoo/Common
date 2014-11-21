/**************************************************************************
文件名：CopyFile.h
文件说明：类FileCopy头文件
简要说明：封装CopyFileEx函数，实现文件复制过程的暂停，控速，异步与同步。创建新的
	线程，并在其中调用CopyFileEx函数，利用CopyFileEx的回调函数实现暂停，控制速度，
	获取进度等功能。
完成日期：21:14 2011/10/4
备注：代码不够完善，没能做好线程同步工作，有时间再去改进！
**************************************************************************/

#pragma once
#include <windows.h>
#include <tchar.h>

#define  WM_COPYFILE_NOTIFY WM_USER + 118+2  //自定义的windows消息，用来通知窗口

class FileCopy
{
private:
	LPCTSTR m_lpExistingPathName;			//源文件
	LPCTSTR m_lpNewPathName;				//目标文件
	int m_iSpeedControl;					//速度控制的变量
	BOOL m_bCancel;						//取消标志，用来传入CopyFileEx的回调函数
	HANDLE  m_hEventPause;				//“复制暂停”事件
	float m_fCopyProgress;				//复制进度
	HWND m_hNotifyWnd;					//接受通知消息的窗口

	HANDLE m_hEventEnd;					//“复制结束”事件

	HANDLE m_hThreadCopy;				//线程句柄

	LARGE_INTEGER m_liTotalFileSize;		//总的文件长度	
	LARGE_INTEGER m_liTotalBytesTransferred;	//已经复制的字节数

	int m_iRetPGR;						//作为CopyProgressRoutine的返回值，此参数未用

	BOOL m_bSuccess;
	DWORD m_dwErrorCode;


	void Initialize();   //初始化内部数据：各种句柄和变量;

	//线程函数，在线程中调用CopyFileEx实现文件复制
	static DWORD WINAPI CopyThreadProc(LPVOID lpParam);


	//CopyFileEx的回调函数，在此函数中实现文件复制过程的控制。
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

	//可以在构造函数中初始化数据
	FileCopy(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName,HWND hNotifyWnd=NULL);
	~FileCopy(void);

	//初始化必要的参数，源文件和目标文件
	BOOL Init(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName,HWND hNotifyWnd=NULL);
	BOOL Init(HWND hNotifyWnd=NULL);

	BOOL SetCopyFileName(LPCTSTR lpExistingPathName,LPCTSTR lpNewPathName);

	//开始拷贝过程
	BOOL Begin();
	//暂停复制
	void Pause();
	//恢复复制
	void Resume();
	//取消复制
	void Cancel();
	//停止复制
	//void Stop();     //Stop();结束复制过程，但保存已经复制的内容，Cancel();会删除已复制的内容。

	//等待复制结束，用来实现“同步”效果,调用此函数后线程会阻塞，直到复制完成或取消。
	void WaitForEnd();
	
	//获取复制进度
	float GetProgress();
	//获取文件总大小，函数返回方式模仿 API GetFileSize();   一般情况下超过4GB的文件不多
	//，lpFileSizeHigh直接忽视就行了
	DWORD GetTotalFileSize(DWORD* lpFileSizeHigh=NULL);
	//获取已经复制的字节数;
	DWORD GetBytesTransferred(DWORD* lpTransferredHigh=NULL);

	//设置复制速度
	void SetSpeed(int iSpeed);

	void SetSuccess(BOOL bSuccess);
	BOOL IsSuccess();

	void SetErrorCode(DWORD dwErrorCode);
	DWORD GetErrorCode();
};

