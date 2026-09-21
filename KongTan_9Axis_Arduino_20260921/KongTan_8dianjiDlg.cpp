#include "pch.h"
#include "framework.h"
#include "KongTan_8dianji.h"
#include "KongTan_8dianjiDlg.h"
#include "afxdialogex.h"
std::set<CML::uint> MotorIds={1,2,3,4,5,6,7,8};
CKongTan8dianjiDlg::CKongTan8dianjiDlg(CWnd* parent):CDialogEx(IDD,parent) {
    m_hIcon=AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}
void CKongTan8dianjiDlg::DoDataExchange(CDataExchange* dx) {
    CDialogEx::DoDataExchange(dx);
    DDX_Text(dx,IDC_EDIT1,motor_V12); DDX_Text(dx,IDC_EDIT2,motor_V34);
    DDX_Text(dx,IDC_EDIT3,motor_V56); DDX_Text(dx,IDC_EDIT4,motor_V78);
    DDX_Text(dx,IDC_EDIT5,motor_S12); DDX_Text(dx,IDC_EDIT6,motor_S34);
    DDX_Text(dx,IDC_EDIT7,motor_S56); DDX_Text(dx,IDC_EDIT8,motor_S78);
}
BEGIN_MESSAGE_MAP(CKongTan8dianjiDlg,CDialogEx)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_CONNECT_NINE,&CKongTan8dianjiDlg::OnConnectNine)
    ON_BN_CLICKED(IDC_STOP_NINE,&CKongTan8dianjiDlg::OnStopNine)
    ON_BN_CLICKED(IDC_FEED_FORWARD,&CKongTan8dianjiDlg::OnFeedForward)
    ON_BN_CLICKED(IDC_FEED_BACKWARD,&CKongTan8dianjiDlg::OnFeedBackward)
    ON_BN_CLICKED(IDC_FEED_ORIGIN,&CKongTan8dianjiDlg::OnFeedOrigin)
    ON_BN_CLICKED(IDC_DEMO_SEQUENCE,&CKongTan8dianjiDlg::OnDemoSequence)
END_MESSAGE_MAP()
BOOL CKongTan8dianjiDlg::OnInitDialog() {
    CDialogEx::OnInitDialog(); SetIcon(m_hIcon,TRUE); SetIcon(m_hIcon,FALSE);
    wchar_t file[32768]={}; GetModuleFileNameW(nullptr,file,_countof(file));
    m_configPath=file;
    m_configPath=m_configPath.Left(m_configPath.ReverseFind(L'\\')+1)+L"machine.ini";
    for (int id:{IDC_LINK_UP,IDC_LINK_LEFT,IDC_LINK_MIDDLE,IDC_LINK_BASE,IDC_LINK_FEED}) SetDlgItemText(id,L"0");
    UpdateData(FALSE); RefreshControls();
    SetSystemStatus(L"未连接。请先核实接线、脉冲换算、方向与限位，再配置 machine.ini。");
    return TRUE;
}
