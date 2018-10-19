
// Test3Dlg.h : ͷ�ļ�
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

struct CClientInfo
{
	SOCKET sSocket;
	CString strIp;
	u_short nPort;
};

// CTest3Dlg �Ի���
class CTest3Dlg : public CDialog
{
// ����
public:
	CTest3Dlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CTest3Dlg();

// �Ի�������
	enum { IDD = IDD_TEST3_DIALOG };
	
	UINT ServerListenThreadProc();
	UINT ServerRecvThreadProc();
	UINT ClientRecvThreadProc();
	

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

private:
	SOCKET m_ClientSocket,m_ServerSocket;
	CArray<CClientInfo> m_ClientArray;
	CRITICAL_SECTION m_CS_ClientArray;
	fd_set m_fdSets;//�׽��ּ���

	void AddLog(CString str,...);
	void StartServer();

	void LockClientArray(){ EnterCriticalSection(&m_CS_ClientArray); }
	void UnLockClientArray(){ LeaveCriticalSection(&m_CS_ClientArray); }
	
	void OnSocketConnected(SOCKET sClientSocket,sockaddr_in * saClient);
	void OnSocketDisconnect(SOCKET aClientSocket = INVALID_SOCKET);

	BOOL SocketMng(SOCKET aClientSocket = INVALID_SOCKET, int nFlag = 0);
	

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();
	CIPAddressCtrl m_ipCtrl;
	CListBox m_list;
};
