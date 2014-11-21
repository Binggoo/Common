#pragma once

//当接受到数据送到窗口的消息
#define ON_COM_RECEIVE WM_USER + 618  //  WPARAM 端口号

class CSerialPort
{
public:
	CSerialPort(bool bAutoBeginThread = true, DWORD dwIOMode = FILE_FLAG_OVERLAPPED);
	virtual ~CSerialPort(void);

protected:
	volatile DWORD m_dwPort;  //串口号
	volatile HANDLE m_hCommHandle;//串口句柄
	TCHAR m_szCommStr[20];

	DWORD m_dwErrorCode;

	DCB m_DCB;     //波特率，停止位，等
	COMMTIMEOUTS m_CO;     //超时结构

	DWORD m_dwIOMode; // 0 同步  默认 FILE_FLAG_OVERLAPPED 重叠I/O 异步
	OVERLAPPED m_ReadOverlapped, m_WriteOverlapped; // 重叠I/O

	//线程用
	volatile HANDLE m_hThreadHandle; //辅助线程
	volatile HWND m_hNotifyWnd; // 通知窗口
	volatile DWORD m_dwNotifyNum;//接受多少字节(>_dwNotifyNum)发送通知消息
	volatile bool m_bRunFlag; //线程运行循环标志
	bool m_bAutoBeginThread;//Open() 自动 BeginThread();
	OVERLAPPED m_WaitOverlapped; //WaitCommEvent use


public:
	//打开串口 缺省 9600, 8, n, 1
	bool Open(DWORD dwPort);

	//打开串口 缺省 baud_rate, 8, n, 1
	bool Open(DWORD dwPort, DWORD dwBaudRate);

	//打开串口, 使用类似"9600, 8, n, 1"的设置字符串设置串口
	bool Open(DWORD dwPort, TCHAR *szSetStr);

	//判断串口是或打开
	bool IsOpen();

	//获得串口句炳
	HANDLE GetHandle();

	//设置串口参数：波特率，停止位，等 支持设置字符串 "9600, 8, n, 1"
	bool SetState(TCHAR *szSetStr);

	//设置串口参数：波特率，停止位
	bool SetState(DWORD dwBaudRate, BYTE byByteSize = 8, BYTE byParity = NOPARITY, BYTE byStopBits = ONESTOPBIT);

	//设置串口的I/O缓冲区大小
	bool SetBufferSize(DWORD dwInputSize, DWORD dwOutputSize);

	//读取串口 dwBufferLength - 1 个字符到 szBuffer 返回实际读到的字符数
	DWORD Read(char *szBuffer, DWORD dwBufferLength, DWORD dwWaitTime = 20);

	//写串口 szBuffer
	DWORD Write(char *szBuffer, DWORD dwBufferLength);

	//写串口 szBuffer
	DWORD Write(char *szBuffer);

	//强制同步写
	DWORD WriteSync(char *szBuffer, DWORD dwBufferLength);

	//写串口 szBuffer 可以输出格式字符串
	DWORD Write(char *szBuffer, DWORD dwBufferLength, char * szFormat, ...);

	//获取接收缓存区中数据大小
	DWORD GetInBufferLength();

	//关闭串口
	virtual void Close();

	//设定发送通知, 接受字符最小值
	void SetNotifyNum(int iNum);

	//送消息的窗口句柄
	void SetWnd(HWND hWnd);

	// 获取ErrorCode
	DWORD GetErrorCode();

	//辅助线程控制 建监视线程
	bool BeginThread();

	//线程是否运行
	bool IsThreadRunning();

	//获得线程句柄
	HANDLE GetThread();

	//暂停监视线程
	bool SuspendThread();

	//恢复监视线程
	bool ResumeThread();

	//终止线程
	bool EndThread(DWORD dwWaitTime = 100);

protected:
	//线程收到消息自动调用, 如窗口句柄有效, 送出消息, 包含窗口编号
	virtual void OnReceive();

	//初始化
	void Init();

	void UnInit();

	//绑定串口
	void BindCommPort(DWORD dwPort);

	//打开串口
	virtual bool OpenCommPort();

	//设置串口
	virtual bool SetupPort();

private:
	//监视线程
	static DWORD WINAPI CommThreadProc(LPVOID lpPara);
};