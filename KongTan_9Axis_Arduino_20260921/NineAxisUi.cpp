#include "pch.h"
#include "KongTan_8dianji.h"
#include "KongTan_8dianjiDlg.h"
#include <cerrno>
#include <cwctype>
#include <limits>

namespace {
double Number(const wchar_t* text) {
    wchar_t* end=nullptr; errno=0; const double result=wcstod(text,&end);
    if(end==text || errno==ERANGE) return std::numeric_limits<double>::quiet_NaN();
    while(iswspace(*end)) ++end;
    return *end==0 && std::isfinite(result) ? result : std::numeric_limits<double>::quiet_NaN();
}
double Ini(const CString& path,const wchar_t* section,const wchar_t* key) {
    wchar_t text[128]={}; GetPrivateProfileStringW(section,key,L"",text,_countof(text),path); return Number(text);
}
bool Integer(double value,double low,double high) { return std::isfinite(value) && value>=low && value<=high && value==std::floor(value); }
const UINT buttons[]={IDC_BUTTON1,IDC_BUTTON2,IDC_BUTTON3,IDC_BUTTON4,IDC_BUTTON5,IDC_BUTTON6,
    IDC_BUTTON7,IDC_BUTTON8,IDC_BUTTON9,IDC_BUTTON10,IDC_BUTTON11,IDC_BUTTON12,IDC_BUTTON13};
}
void CKongTan8dianjiDlg::SetSystemStatus(const CString& text) { SetDlgItemText(IDC_NINE_STATUS,text); }
bool CKongTan8dianjiDlg::LoadFeedConfig() {
    ArduinoConfig c;
    c.confirmed=Ini(m_configPath,L"Machine",L"Confirmed")==1;
    wchar_t port[32]={}; GetPrivateProfileStringW(L"Arduino",L"Port",L"",port,_countof(port),m_configPath); c.port=port;
    c.stepsPerMm=Ini(m_configPath,L"Feed",L"StepsPerMm");
    c.maxMmPerSec=Ini(m_configPath,L"Feed",L"MaxMmPerSec");
    c.accelerationMmPerSec2=Ini(m_configPath,L"Feed",L"AccelerationMmPerSec2");
    c.negativeMm=Ini(m_configPath,L"Feed",L"NegativeMmFromConnection");
    c.positiveMm=Ini(m_configPath,L"Feed",L"PositiveMmFromConnection");
    double rate=Ini(m_configPath,L"Feed",L"MaxPulseRate"),acc=Ini(m_configPath,L"Feed",L"MaxPulseAcceleration");
    if(!Integer(rate,1,1000) || !Integer(acc,1,10000)) return false;
    c.maxPulseRate=(int)rate; c.maxPulseAcceleration=(int)acc;
    if(!c.Valid()) return false;
    machine::FeedConfig f; f.confirmed=true; f.node=9;
    f.negative=std::ceil(c.negativeMm*c.stepsPerMm); f.positive=std::floor(c.positiveMm*c.stepsPerMm);
    f.velocity=(std::min)(c.maxMmPerSec*c.stepsPerMm,(double)c.maxPulseRate);
    f.acceleration=(std::min)(c.accelerationMmPerSec2*c.stepsPerMm,(double)c.maxPulseAcceleration);
    f.step=std::round(Ini(m_configPath,L"Feed",L"JogMm")*c.stepsPerMm); f.tolerance=0.25;
    const double direction=Ini(m_configPath,L"Feed",L"Direction");
    if(direction!=1 && direction!=-1) return false;
    f.direction=(int)direction;
    const double timeout=Ini(m_configPath,L"Link",L"SegmentTimeoutMs");
    if(!Integer(timeout,1000,600000)) return false;
    f.timeoutMs=(std::uint64_t)timeout;
    if(!f.Valid()) return false;
    machine::LinkSettings cfg;
    cfg.robotVelocity=Ini(m_configPath,L"Robot",L"MaxCountsPerSec");
    cfg.robotAcceleration=Ini(m_configPath,L"Robot",L"AccelerationCountsPerSec2");
    cfg.robotTolerance=Ini(m_configPath,L"Robot",L"ToleranceCounts");
    cfg.robotSegment=Ini(m_configPath,L"Link",L"MaxRobotCountsPerSegment");
    const double feedSegmentMm=Ini(m_configPath,L"Link",L"MaxFeedMmPerSegment");
    cfg.feedSegment=std::floor(feedSegmentMm*c.stepsPerMm);
    cfg.feedVelocity=f.velocity; cfg.feedAcceleration=f.acceleration; cfg.segmentTimeoutMs=f.timeoutMs;
    if(!cfg.Valid() || cfg.robotVelocity>100000 || cfg.robotAcceleration>1000000 ||
        cfg.robotTolerance>cfg.robotSegment || !std::isfinite(feedSegmentMm) || feedSegmentMm<=0 || feedSegmentMm>1) return false;
    for(int i=0;i<8;++i) {
        CString section; section.Format(L"Axis%d",i+1);
        const double lo=Ini(m_configPath,section,L"NegativeCounts"),hi=Ini(m_configPath,section,L"PositiveCounts");
        const double sign=Ini(m_configPath,section,L"PullSign");
        if(!Integer(lo,-2147483647,2147483647) || !Integer(hi,-2147483647,2147483647) || lo>=hi || (sign!=1 && sign!=-1)) return false;
        m_axisSigns[i]=(int)sign; m_linkLimits[i]={i+1,lo,hi};
    }
    m_linkLimits[8]={9,f.negative,f.positive};
    m_port.config=c; m_feed=f; m_linkSettings=cfg;
    return true;
}
void CKongTan8dianjiDlg::RefreshControls() {
    const bool idle=m_nine.Ready() && !m_link.Active();
    for(UINT id:buttons) if(GetDlgItem(id)) GetDlgItem(id)->EnableWindow(idle);
    for(UINT id:{IDC_FEED_FORWARD,IDC_FEED_BACKWARD,IDC_FEED_ORIGIN,IDC_DEMO_SEQUENCE}) GetDlgItem(id)->EnableWindow(idle);
    GetDlgItem(IDC_CONNECT_NINE)->EnableWindow(!m_nine.Ready());
}
void CKongTan8dianjiDlg::OnConnectNine() {
    if(m_nine.Ready()) return;
    if(!LoadFeedConfig()) {
        SetSystemStatus(L"未连接：配置未确认或数值无效。检查 EXE 同目录 machine.ini。\nArduino 引脚须在 BoardConfig.h 中确认。"); return;
    }
    if(!SetTimer(901,50,nullptr)) { SetSystemStatus(L"状态监控定时器创建失败。"); return; }
    std::vector<machine::AxisConfig> robot(m_linkLimits.begin(),m_linkLimits.begin()+8);
    SetSystemStatus(L"正在连接 Copley 八轴与 Mega 2560……");
    if(!m_nine.Connect(m_feed,robot)) {
        SetSystemStatus(L"连接失败：检查 COM、固件接线确认项、CAN 和配置。\n"+CString(m_nine.Error().c_str()));
        KillTimer(901);
    } else SetSystemStatus(L"九轴已使能。进给当前位置记为 0；每次重新连接前须核实剩余行程。");
    RefreshControls();
}
void CKongTan8dianjiDlg::OnStopNine() {
    m_link.Cancel(); const bool ok=m_nine.Stop(); KillTimer(901);
    SetSystemStatus(ok ? L"已停止脉冲、撤销使能并断开连接。" : L"停止未全部确认，请使用硬件停止回路并检查设备。");
    SetDlgItemText(IDC_SEQUENCE_STATUS,L"联动已停止"); RefreshControls();
}
bool CKongTan8dianjiDlg::StartLinked(const std::array<double,9>& delta,double velocity) {
    if(!m_nine.Ready() || m_link.Active()) return false;
    bool noMotion=true;
    for(double d:delta) noMotion=noMotion && std::isfinite(d) && std::round(d)==0;
    if(noMotion) { SetSystemStatus(L"本次位移量化后为 0，未发送运动指令。"); return true; }
    auto cfg=m_linkSettings;
    if(velocity!=0) {
        if(!std::isfinite(velocity) || velocity<=0 || velocity>cfg.robotVelocity) {
            SetSystemStatus(L"手动速度须大于 0，且不超过配置的八轴速度上限。"); return false;
        }
        cfg.robotVelocity=velocity;
    }
    if(!m_link.Start(delta,m_linkLimits,cfg,GetTickCount64())) {
        const CString reason(m_link.Error().c_str()); OnStopNine(); SetSystemStatus(L"运动未执行/已停止："+reason); return false;
    }
    SetSystemStatus(L"分段运动中；全部轴到位后才下发下一段。"); RefreshControls(); return true;
}
void CKongTan8dianjiDlg::MoveFeed(bool backward,bool origin) {
    if(!m_nine.Ready() || m_link.Active()) return;
    std::array<double,9> delta{};
    if(origin) {
        machine::AxisFeedback f;
        if(!m_port.Read(9,f)) { OnStopNine(); SetSystemStatus(L"进给反馈读取失败。"); return; }
        delta[8]=m_nine.Origins().at(9)-f.position;
    } else delta[8]=(backward?-1:1)*m_feed.direction*m_feed.step;
    StartLinked(delta);
}
void CKongTan8dianjiDlg::OnFeedForward(){MoveFeed(false,false);}
void CKongTan8dianjiDlg::OnFeedBackward(){MoveFeed(true,false);}
void CKongTan8dianjiDlg::OnFeedOrigin(){MoveFeed(false,true);}
void CKongTan8dianjiDlg::OnDemoSequence() {
    const UINT inputs[]={IDC_LINK_UP,IDC_LINK_LEFT,IDC_LINK_MIDDLE,IDC_LINK_BASE,IDC_LINK_FEED};
    double values[5]={};
    for(int i=0;i<5;++i) {
        CString text; GetDlgItemText(inputs[i],text); values[i]=Number(text);
        if(!std::isfinite(values[i])) { SetSystemStatus(L"联动位移必须是有效数值。"); return; }
    }
    auto delta=machine::BendDeltas(values[0],values[1],values[2],values[3],std::round(values[4]*m_port.config.stepsPerMm)*m_feed.direction);
    for(int i=0;i<8;++i) delta[i]*=m_axisSigns[i];
    StartLinked(delta);
}
BOOL CKongTan8dianjiDlg::OnCommand(WPARAM wp,LPARAM lp) {
    const UINT id=LOWORD(wp);
    if(id==IDCCANCEL) { OnCancel(); return TRUE; }
    int index=-1; for(int i=0;i<13;++i) if(buttons[i]==id) index=i;
    if(index<0) return CDialogEx::OnCommand(wp,lp);
    if(!m_nine.Ready() || m_link.Active()) return TRUE;
    if(!UpdateData(TRUE)) return TRUE;
    std::array<double,9> delta{};
    double velocity=0;
    if(index==12 || index%3==2) {
        const int first=index==12?0:(index/3)*2, last=index==12?8:first+2;
        for(int i=first;i<last;++i) {
            machine::AxisFeedback f;
            if(!m_port.Read(i+1,f)) { OnStopNine(); return TRUE; }
            delta[i]=m_nine.Origins().at(i+1)-f.position;
        }
    } else {
        const int group=index/3;
        const double steps[]={motor_S12,motor_S34,motor_S56,motor_S78};
        const double speeds[]={motor_V12,motor_V34,motor_V56,motor_V78};
        if(!std::isfinite(steps[group]) || steps[group]<1 || !std::isfinite(speeds[group]) || speeds[group]<=0) {
            SetSystemStatus(L"手动步长须至少 1 count，速度须为正数。"); return TRUE;
        }
        const double d=(index%3==0?1:-1)*steps[group];
        delta[group*2]=d*m_axisSigns[group*2]; delta[group*2+1]=-d*m_axisSigns[group*2+1];
        velocity=speeds[group];
    }
    StartLinked(delta,velocity); return TRUE;
}
void CKongTan8dianjiDlg::OnTimer(UINT_PTR id) {
    if(id!=901) { CDialogEx::OnTimer(id); return; }
    if(!m_nine.Ready()) return;
    double feed=0;
    if(!m_nine.Poll(GetTickCount64(),feed)) {
        const CString reason(m_nine.Error().c_str()); m_link.Cancel(); KillTimer(901);
        SetSystemStatus(L"九轴反馈故障，已尝试停止："+reason); RefreshControls(); return;
    }
    CString position; position.Format(L"进给：%.3f mm / %.0f 脉冲（开环计数）",feed/m_port.config.stepsPerMm,feed);
    SetDlgItemText(IDC_FEED_POSITION,position);
    const bool active=m_link.Active();
    if(!m_link.Tick(GetTickCount64())) {
        const CString reason(m_link.Error().c_str()); OnStopNine(); SetSystemStatus(L"联动停止："+reason); return;
    }
    if(active) {
        CString text; text.Format(L"分段：%u / %u%s",m_link.Segment(),m_link.Count(),m_link.Active()?L"，等待全部轴到位":L"，指令完成");
        SetDlgItemText(IDC_SEQUENCE_STATUS,text);
        if(!m_link.Active()) SetSystemStatus(L"本次运动完成：八轴反馈已到位，进给脉冲已输出。");
    }
    RefreshControls();
}
void CKongTan8dianjiDlg::OnCancel() { OnStopNine(); CDialogEx::OnCancel(); }
