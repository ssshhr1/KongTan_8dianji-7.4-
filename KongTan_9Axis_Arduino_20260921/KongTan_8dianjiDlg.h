#pragma once
#include "afxwin.h"
#include "ArduinoMotorPort.h"
#include "LinkedMotion.h"
#include <array>

class CKongTan8dianjiDlg : public CDialogEx {
public:
    CKongTan8dianjiDlg(CWnd* parent=nullptr);
    enum { IDD = IDD_KONGTAN_8DIANJI_DIALOG };
protected:
    HICON m_hIcon;
    CmlMotor m_motorCtrl;
    ArduinoMotorPort m_port{m_motorCtrl};
    machine::NineAxisController m_nine{m_port};
    machine::LinkedMotion m_link{m_port};
    machine::FeedConfig m_feed;
    machine::LinkSettings m_linkSettings;
    std::array<machine::AxisConfig,9> m_linkLimits{};
    std::array<int,8> m_axisSigns{};
    CString m_configPath;
    double motor_V12=1000,motor_V34=1000,motor_V56=1000,motor_V78=1000;
    double motor_S12=100,motor_S34=100,motor_S56=100,motor_S78=100;
    bool LoadFeedConfig();
    bool StartLinked(const std::array<double,9>& delta, double velocity=0);
    void SetSystemStatus(const CString& text);
    void RefreshControls();
    void MoveFeed(bool backward,bool origin);
    virtual void DoDataExchange(CDataExchange* dx);
    virtual BOOL OnInitDialog();
    virtual BOOL OnCommand(WPARAM wp,LPARAM lp);
    virtual void OnCancel();
    virtual void OnOK() {}
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnConnectNine();
    afx_msg void OnStopNine();
    afx_msg void OnFeedForward();
    afx_msg void OnFeedBackward();
    afx_msg void OnFeedOrigin();
    afx_msg void OnDemoSequence();
    DECLARE_MESSAGE_MAP()
};
