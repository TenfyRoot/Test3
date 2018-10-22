
// Test3Dlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Test3.h"
#include "Test3Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CTest3Dlg 对话框

#define SERVER_PORT 8888

UINT WINAPIV ServerListenThread( LPVOID pParam )
{
	CTest3Dlg * pThis = (CTest3Dlg *) pParam;
	return pThis->ServerListenThreadProc();
}

UINT WINAPIV ServerRecvThread( LPVOID pParam )
{
	CTest3Dlg * pThis = (CTest3Dlg *) pParam;
	return pThis->ServerRecvThreadProc();
}

UINT WINAPIV ClientRecvThread( LPVOID pParam )
{
	CTest3Dlg * pThis = (CTest3Dlg *) pParam;
	return pThis->ClientRecvThreadProc();
}

UINT WINAPIV ClientConnectThread(LPVOID pParam)
{
	CTest3Dlg * pThis = (CTest3Dlg *)pParam;
	return pThis->ClientConnectThreadProc();
}

CTest3Dlg::CTest3Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTest3Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_ClientSocket = INVALID_SOCKET;
	m_ServerSocket = INVALID_SOCKET;
	m_bThreadWorking = false;
	m_bConnected = false;
	m_pServerListenThread = m_pServerRecvThread = false;
	m_pClientConnectThread = m_pClientRecvThread = false;
	m_bServerListenThreadWorking = m_bServerRecvThreadWorking = false;
	m_bClientConnectThreadWorking = m_bClientRecvThreadWorking = false;
	InitializeCriticalSection(&m_CS_ClientArray);
	InitializeCriticalSection(&m_CS_ConnectStatus);
}
CTest3Dlg::~CTest3Dlg()
{
	m_bServerListenThreadWorking = false;
	WaitForSingleObject(m_pServerListenThread, 500);
	m_bServerRecvThreadWorking = false;
	WaitForSingleObject(m_pServerRecvThread, 500);

	m_bClientConnectThreadWorking = false;
	WaitForSingleObject(m_pClientConnectThread, 500);
	m_bClientRecvThreadWorking = false;
	WaitForSingleObject(m_pClientRecvThread, 500);

	DeleteCriticalSection(&m_CS_ClientArray);
	DeleteCriticalSection(&m_CS_ConnectStatus);
}

void CTest3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS1, m_ipCtrl);
	DDX_Control(pDX, IDC_LIST1, m_list);
}

BEGIN_MESSAGE_MAP(CTest3Dlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDOK, &CTest3Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CTest3Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CTest3Dlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// CTest3Dlg 消息处理程序

BOOL CTest3Dlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	SetDlgItemInt(IDC_EDIT1, SERVER_PORT);

	CString strIP = "127.0.0.1";
	DWORD dwIP = inet_addr(strIP);
	unsigned char *pIP = (unsigned char*)&dwIP;
	m_ipCtrl.SetAddress(pIP[0], pIP[1], pIP[2], pIP[3]);

	StartServer();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTest3Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTest3Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CTest3Dlg::OnBnClickedOk()
{
	m_list.ResetContent();
}

void CTest3Dlg::AddLog(CString str,...)
{
	TCHAR szBuff[4096];
	va_list list;
    va_start(list,str);

	_vsnprintf(szBuff,4096,str,list);
    va_end(list);

	m_list.AddString(szBuff);
	m_list.SetCurSel(m_list.GetCount() - 1);
}

void CTest3Dlg::OnBnClickedButton1()
{
	GetDlgItem(IDC_BUTTON1)->EnableWindow(FALSE);

	if (m_ClientSocket != INVALID_SOCKET)
		closesocket(m_ClientSocket);
	m_ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ClientSocket == INVALID_SOCKET)
	{
		AddLog("client >> create socket error-%d!", WSAGetLastError());
		SetBtnStatus(0);
		return;
	}

	int nPort = GetDlgItemInt(IDC_EDIT1);
	DWORD dwIP;
	m_ipCtrl.GetAddress(dwIP);

	m_saServer.sin_family = AF_INET; //地址家族  
	m_saServer.sin_port = htons(nPort); //注意转化为网络节序  
	m_saServer.sin_addr.S_un.S_addr = htonl(dwIP);//inet_addr(CT2A(strIp));

	m_pClientConnectThread = AfxBeginThread(ClientConnectThread, this);
	m_pClientRecvThread = AfxBeginThread(ClientRecvThread,this);
}

UINT CTest3Dlg::ClientRecvThreadProc()
{
	m_bClientRecvThreadWorking = true;
	while (m_bClientRecvThreadWorking)
	{
		LockConnectStatus();
		if (!m_bConnected)
		{
			UnLockConnectStatus();
			Sleep(200);
			continue;
		}
		UnLockConnectStatus();

		char szBuf[256] = { 0 };
		if (SOCKET_ERROR == recv(m_ClientSocket, szBuf, 250, 0))
		{
			int nError = WSAGetLastError();
			if (nError != 0)
			{
				SetBtnStatus(1);
				LockConnectStatus(false);
				continue;
			}
		}
		AddLog((LPCTSTR)szBuf);
	}
	return 0;
}

UINT CTest3Dlg::ClientConnectThreadProc()
{
	int nLastError = 0;
	m_bClientConnectThreadWorking = true;
	while (m_bClientConnectThreadWorking)
	{
		LockConnectStatus();
		if (m_bConnected)
		{
			UnLockConnectStatus();
			Sleep(200);
			continue;
		}
		UnLockConnectStatus();

		if (SOCKET_ERROR == connect(m_ClientSocket, (SOCKADDR *)&m_saServer, sizeof(m_saServer)))
		{
			int nError = WSAGetLastError();
			if (nLastError != nError && nError == 10056)//Socket is already connected
			{
				closesocket(m_ClientSocket);
				m_ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				nLastError = nError;
			}
			else if (nLastError != nError && nError == 10061)
			{
				AddLog(_T("Connection refused! [10061]"));
				nLastError = nError;
			}
			else if (nLastError != nError)
			{
				int nError = WSAGetLastError();
				CString strErrorMsg;
				strErrorMsg.Format(_T("Connectting [%d]...."), nError);
				AddLog(strErrorMsg);
				nLastError = nError;
			}
			Sleep(100);
			continue;
		}
		LockConnectStatus(true);
		AddLog(_T("Connected OK! %s:%d"), inet_ntoa(m_saServer.sin_addr),ntohs(m_saServer.sin_port));
		SetBtnStatus(2);
		nLastError = 0;
	}
	if (m_ClientSocket != INVALID_SOCKET)
	{
		closesocket(m_ClientSocket);
	}
	m_bConnected = false;

	return 0;
}

void CTest3Dlg::StartServer()
{
	m_ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ServerSocket == INVALID_SOCKET)
	{
		AddLog("server >> create socket error-%d!", WSAGetLastError());
		return;
	}

	sockaddr_in saServer;
	saServer.sin_family = AF_INET; //地址家族  
    saServer.sin_port = htons(SERVER_PORT); //注意转化为网络节序  
    saServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//inet_addr("127.0.0.1");
	if(SOCKET_ERROR == bind(m_ServerSocket,(SOCKADDR *)&saServer,sizeof(saServer)))
	{
		AddLog("server >> bind port %d error-%d!",ntohs(saServer.sin_port), WSAGetLastError());
		return;
	}

	if(SOCKET_ERROR == listen(m_ServerSocket,SOMAXCONN))
	{
		AddLog("server >> listen error-%d!", WSAGetLastError());
		return ;
	}

	SocketMng();

	m_pServerListenThread = AfxBeginThread(ServerListenThread,this);
	m_pServerRecvThread = AfxBeginThread(ServerRecvThread,this);
}

UINT CTest3Dlg::ServerListenThreadProc()
{
	m_bServerListenThreadWorking = true;
	while(m_bServerListenThreadWorking)
	{
		sockaddr_in saClient = {0};
		int  nClientSocketLen = sizeof(saClient);
		SOCKET sClientSocket = accept(m_ServerSocket,(sockaddr *)&saClient,&nClientSocketLen);
		if(sClientSocket ==  INVALID_SOCKET)
		{
			AddLog("server >> accept error-%d", ::WSAGetLastError());
			break;
		}
		OnSocketConnected(sClientSocket,&saClient);
	}
	if (m_ServerSocket != INVALID_SOCKET)
	{
		closesocket(m_ServerSocket);
	}
	return 0;
}

UINT CTest3Dlg::ServerRecvThreadProc()
{
	timeval tv;
	tv.tv_sec  = 5;
	tv.tv_usec = 0;

	m_bServerRecvThreadWorking = true;
	while(m_bServerRecvThreadWorking)
	{
		if(m_ClientArray.GetCount() == 0 ||
			select(0, &m_fdSets, NULL, NULL, &tv) <= 0)
		{
			Sleep(200);
			continue;
		}

		LockClientArray();
		CArray<CClientInfo> RecvArray;
		for(int i = 0;i < m_ClientArray.GetCount();i++)
		{
			CClientInfo aClientInfo = m_ClientArray.GetAt(i);
			if(SocketMng(aClientInfo.sSocket))
				RecvArray.Add(aClientInfo);
		}
		UnLockClientArray();

		for(int i = 0;i < RecvArray.GetCount();i++)
		{
			CClientInfo aClientInfo = RecvArray.GetAt(i);
			char szRecvBuf[1024] = {0};
			int nRecvRet = recv(aClientInfo.sSocket,szRecvBuf,1024,0);
			if(nRecvRet == SOCKET_ERROR)
			{
				int nErr = ::WSAGetLastError();
				if (nErr != 0)
				{
					AddLog("server >> [%d] client %s-%d", nErr, aClientInfo.strIp, aClientInfo.nPort);
				}
				OnSocketDisconnect(aClientInfo.sSocket);
				continue;
			}
			AddLog(szRecvBuf);
		}
	}
	return 0;
}

void CTest3Dlg::OnSocketConnected(SOCKET sClientSocket,sockaddr_in * saClient)
{
	u_short uPort =  ntohs(((sockaddr_in *)&saClient)->sin_port);
	CString strIp = CA2T(inet_ntoa(((sockaddr_in *)&saClient)->sin_addr));
	CClientInfo aClientInfo;
	aClientInfo.nPort = uPort;
	aClientInfo.strIp = strIp;
	aClientInfo.sSocket = sClientSocket;

	LockClientArray();
	m_ClientArray.Add(aClientInfo);
	SocketMng(sClientSocket,1);
	AddLog("server >> add client %s-%d",aClientInfo.strIp, aClientInfo.nPort);
	UnLockClientArray();
}

void CTest3Dlg::OnSocketDisconnect(SOCKET aClientSocket)
{
	LockClientArray();
	for(int i = 0;i<m_ClientArray.GetCount();i++)
	{
		CClientInfo aClientInfo = m_ClientArray.GetAt(i);
		if( aClientSocket == INVALID_SOCKET ||
			aClientSocket == aClientInfo.sSocket )	
		{
			SocketMng(aClientInfo.sSocket,-1);
			closesocket(aClientInfo.sSocket);
			m_ClientArray.RemoveAt(i);
			AddLog("server >> mov client %s-%d", aClientInfo.strIp,aClientInfo.nPort);
			if (aClientSocket != INVALID_SOCKET) break;
		}
	}
	UnLockClientArray();
}

BOOL CTest3Dlg::SocketMng(SOCKET aClientSocket, int nFlag)
{
	if (aClientSocket == INVALID_SOCKET)
	{
		FD_ZERO(&m_fdSets);//清空套接字集合
		return TRUE;
	}
	switch(nFlag)
	{
	case 1:
		FD_SET(aClientSocket,&m_fdSets);//加入你感兴趣的套节字到集合
		break;
	case -1:
		if (FD_ISSET(aClientSocket,&m_fdSets))
		{
			FD_CLR(aClientSocket,&m_fdSets);//从集合中删除指定的套节字
			return TRUE;
		}
		return FALSE;
	}
	return FD_ISSET(aClientSocket,&m_fdSets);//检查集合中指定的套节字是否可以读写
}

void CTest3Dlg::SetBtnStatus(int nFlag)
{
	if (nFlag == 0)
	{
		GetDlgItem(IDC_BUTTON1)->EnableWindow(true);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(false);
		//GetDlgItem(IDC_BUTTON3)->EnableWindow(false);
	}
	else if (nFlag == 1)
	{
		GetDlgItem(IDC_BUTTON1)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(false);
		//GetDlgItem(IDC_BUTTON3)->EnableWindow(false);
	}
	else
	{
		GetDlgItem(IDC_BUTTON1)->EnableWindow(false);
		GetDlgItem(IDC_BUTTON2)->EnableWindow(true);
		//GetDlgItem(IDC_BUTTON3)->EnableWindow(true);
	}

}

void CTest3Dlg::OnBnClickedButton2()
{
	m_bClientConnectThreadWorking = false;
	WaitForSingleObject(m_pClientConnectThread, 500);
	m_bClientRecvThreadWorking = false;
	WaitForSingleObject(m_pClientRecvThread, 500);

	SetBtnStatus(0);
}
