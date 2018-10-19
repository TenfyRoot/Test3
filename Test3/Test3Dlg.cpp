
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

CTest3Dlg::CTest3Dlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTest3Dlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	InitializeCriticalSection(&m_CS_ClientArray);
	m_ClientSocket = INVALID_SOCKET;
	m_ServerSocket = INVALID_SOCKET;
}
CTest3Dlg::~CTest3Dlg()
{
	if (m_ClientSocket != INVALID_SOCKET) 
	{
		closesocket(m_ClientSocket);
	}
	if (m_ClientSocket != INVALID_SOCKET) 
	{
		OnSocketDisconnect();
		closesocket(m_ServerSocket);
	}
	DeleteCriticalSection(&m_CS_ClientArray);
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

	CString strIP = "10.177.10.163";
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
	m_ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_ClientSocket == INVALID_SOCKET)
	{
		AddLog("client >> create socket error-%d!", WSAGetLastError());
		GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
		return;
	}

	int nPort = GetDlgItemInt(IDC_EDIT1);

	DWORD dwIP;
	m_ipCtrl.GetAddress(dwIP);
	
	sockaddr_in saServer;
	saServer.sin_family = AF_INET; //地址家族  
    saServer.sin_port = htons(nPort); //注意转化为网络节序  
    saServer.sin_addr.S_un.S_addr = htonl(dwIP);//inet_addr(CT2A(strIp));
	AddLog("client >> connect to %s-%d", inet_ntoa(saServer.sin_addr), ntohs(saServer.sin_port));
	if(SOCKET_ERROR == connect(m_ClientSocket,(SOCKADDR *)&saServer,sizeof(saServer)))
	{
		AddLog("client >> connect error-%d", WSAGetLastError());
		GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
		return ;
	}

	AfxBeginThread(ClientRecvThread,this);
}

UINT CTest3Dlg::ClientRecvThreadProc()
{
	while(1)
	{
		char szBuf[256] = {0};
		if(SOCKET_ERROR == recv(m_ClientSocket,szBuf,250,0))
		{
			AddLog("client >> recv error-%d", WSAGetLastError());
			GetDlgItem(IDC_BUTTON1)->EnableWindow(TRUE);
			break;
		}
		else
		{
			AddLog((LPCTSTR)szBuf);
		}
	}
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

	AfxBeginThread(ServerListenThread,this);
	AfxBeginThread(ServerRecvThread,this);
}

UINT CTest3Dlg::ServerListenThreadProc()
{
	while(1)
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
	return 0;
}

UINT CTest3Dlg::ServerRecvThreadProc()
{
	timeval tv;
	tv.tv_sec  = 5;
	tv.tv_usec = 0;

	while(1)
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
				AddLog("server >> recv error-%d", ::WSAGetLastError());
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
	AddLog("server >> client add %s-%d",aClientInfo.strIp, aClientInfo.nPort);
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
			AddLog("server >> client mov %s-%d",aClientInfo.strIp,aClientInfo.nPort);
			if (aClientSocket != INVALID_SOCKET)
				break;
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