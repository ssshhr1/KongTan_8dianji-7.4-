#include "pch.h"
#include "KongTan_8dianji.h"
#include "KongTan_8dianjiDlg.h"
#include <cerrno>
#include <cwctype>
#include <limits>

namespace {
double IniNumber(const CString& path, const wchar_t* section, const wchar_t* key) {
    wchar_t text[128] = {};
    GetPrivateProfileStringW(section, key, L"", text, _countof(text), path);
    wchar_t* end = nullptr;
    errno = 0;
    const double result = wcstod(text, &end);
    if (end == text || errno == ERANGE) return std::numeric_limits<double>::quiet_NaN();
    while (iswspace(*end)) ++end;
    return *end == L'\0' && std::isfinite(result) ? result : std::numeric_limits<double>::quiet_NaN();
}
bool Integer(double v, double low, double high) { return std::isfinite(v) && v >= low && v <= high && std::floor(v) == v; }
const UINT legacyMotion[] = { IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5,
    IDC_BUTTON6, IDC_BUTTON7, IDC_BUTTON8, IDC_BUTTON9, IDC_BUTTON10, IDC_BUTTON11, IDC_BUTTON12,
    IDC_BUTTON13, IDC_BTN_EXECUTE_BENDING, IDC_BTN_IK_SOLVE, IDC_BTN_PROPORTIONAL_HOOK, IDC_BTN_LINEAR_TRAJECTORY };
const wchar_t* StateText(inspection::State s) {
    switch (s) {
    case inspection::State::Idle: return L"未启动";
    case inspection::State::Preparing: return L"对准 / 等待锁相";
    case inspection::State::Tracking: return L"同步跟踪检测";
    case inspection::State::Retracting: return L"退出检测区域";
    case inspection::State::Indexing: return L"切换下一叶片";
    case inspection::State::Completed: return L"全部叶片完成";
    case inspection::State::Stopped: return L"已停止";
    case inspection::State::Fault: return L"故障";
    }
    return L"未知";
}
}

void CKongTan8dianjiDlg::SetSystemStatus(const CString& text) { SetDlgItemText(IDC_NINE_STATUS, text); }

bool CKongTan8dianjiDlg::LoadFeedConfig() {
    machine::FeedConfig f;
    const double node = IniNumber(m_configPath, L"Feed", L"NodeId");
    const double direction = IniNumber(m_configPath, L"Feed", L"Direction");
    const double timeout = IniNumber(m_configPath, L"Feed", L"MoveTimeoutMs");
    if (!Integer(node, 9, 12) || !Integer(direction, -1, 1) || direction == 0 || !Integer(timeout, 100, 600000)) return false;
    f.node = static_cast<int>(node); f.direction = static_cast<int>(direction);
    f.timeoutMs = static_cast<std::uint64_t>(timeout);
    f.confirmed = IniNumber(m_configPath, L"Feed", L"Confirmed") == 1;
    f.negative = IniNumber(m_configPath, L"Feed", L"NegativeLimitCounts");
    f.positive = IniNumber(m_configPath, L"Feed", L"PositiveLimitCounts");
    f.velocity = IniNumber(m_configPath, L"Feed", L"VelocityCountsPerSec");
    f.acceleration = IniNumber(m_configPath, L"Feed", L"AccelerationCountsPerSec2");
    f.step = IniNumber(m_configPath, L"Feed", L"StepCounts");
    f.tolerance = IniNumber(m_configPath, L"Feed", L"PositionToleranceCounts");
    if (!f.Valid()) return false;
    m_feed = f; return true;
}

void CKongTan8dianjiDlg::RefreshControls() {
    const bool manual = m_nine.Ready() && !m_nine.Busy() && !m_legacyBusy && !m_sequence.Active();
    for (UINT id : legacyMotion) if (GetDlgItem(id)) GetDlgItem(id)->EnableWindow(manual);
    for (UINT id : { IDC_FEED_FORWARD, IDC_FEED_BACKWARD, IDC_FEED_ORIGIN }) GetDlgItem(id)->EnableWindow(manual);
    GetDlgItem(IDC_CONNECT_NINE)->EnableWindow(!m_nine.Ready() && !m_legacyBusy && !m_sequence.Active());
    GetDlgItem(IDC_DEMO_SEQUENCE)->EnableWindow(!m_nine.Ready() && !m_legacyBusy && !m_sequence.Active());
}

void CKongTan8dianjiDlg::OnConnectNine() {
    if (m_nine.Ready() || m_legacyBusy || m_sequence.Active()) return;
    if (!LoadFeedConfig()) {
        SetSystemStatus(L"未连接：进给轴配置缺失、无效或未确认。");
        MessageBox(L"请填写 EXE 同目录的 machine.ini：实际节点号、编码器坐标限位、速度、加速度、步长、容差和方向，再设 Confirmed=1。\n\n配置文件：\n" + m_configPath, L"配置待填写", MB_ICONINFORMATION);
        return;
    }
    if (!SetTimer(901, 100, nullptr)) { SetSystemStatus(L"无法建立状态监控，连接已取消。"); return; }
    std::vector<machine::AxisConfig> robot;
    InitMotorSoftLimits();
    for (int id = 1; id <= 8; ++id) {
        const auto& limits = m_motorSoftLimits.at(id);
        robot.push_back({id, limits.negLimit, limits.posLimit});
    }
    SetSystemStatus(L"正在初始化九轴，请等待状态确认……");
    if (m_nine.Connect(m_feed, robot)) {
        m_motorZeroPos = m_nine.Origins();
        CString status;
        status.Format(L"1～8 号轴及进给 %d 号轴已使能；连接位置已记录。", m_feed.node);
        SetSystemStatus(status);
    } else {
        m_motorZeroPos.clear();
        SetSystemStatus(L"连接失败，已尝试停止并撤销使能：" + CString(m_nine.Error().c_str()));
    }
    RefreshControls();
}

void CKongTan8dianjiDlg::OnStopNine() {
    const bool sequenceStopped = m_sequence.Stop();
    const bool motorsStopped = m_nine.Stop();
    m_motorZeroPos.clear();
    SetSystemStatus(sequenceStopped && motorsStopped ? L"已停止并关闭总线；再次连接会重新记录起点。" : L"停止/关闭未获全部确认，请检查设备及硬件停止回路。");
    SetDlgItemText(IDC_SEQUENCE_STATUS, L"演示：已停止");
    RefreshControls();
}

void CKongTan8dianjiDlg::MoveFeed(bool backward, bool origin) {
    if (m_legacyBusy || m_sequence.Active()) return;
    if (!m_nine.FeedMove(backward ? -m_feed.step : m_feed.step, origin, GetTickCount64()))
        SetSystemStatus(L"进给指令未执行：" + CString(m_nine.Error().c_str()));
    else SetSystemStatus(origin ? L"进给轴正在返回本次连接起点……" : L"进给轴正在执行单步定位……");
    RefreshControls();
}
void CKongTan8dianjiDlg::OnFeedForward() { MoveFeed(false, false); }
void CKongTan8dianjiDlg::OnFeedBackward() { MoveFeed(true, false); }
void CKongTan8dianjiDlg::OnFeedOrigin() { MoveFeed(false, true); }

bool CKongTan8dianjiDlg::SetLegacyVelocity(int id, double velocity, double acceleration, double deceleration) {
    if (!m_nine.Ready()) return false;
    if (id < 1 || id > 8 || !std::isfinite(velocity) || velocity <= 0 ||
        !std::isfinite(acceleration) || acceleration <= 0 || !std::isfinite(deceleration) || deceleration <= 0 ||
        !m_motorCtrl.SetMotorVelocity(id, velocity, acceleration, deceleration)) {
        OnStopNine(); SetSystemStatus(L"八轴速度参数无效或下发失败，已尝试停止九轴。"); return false;
    }
    return true;
}

bool CKongTan8dianjiDlg::MoveLegacyAbsolute(int id, double target) {
    if (!m_nine.Ready()) return false;
    const auto limit = m_motorSoftLimits.find(id);
    if (limit == m_motorSoftLimits.end() || !std::isfinite(target) ||
        target < limit->second.negLimit || target > limit->second.posLimit ||
        !m_motorCtrl.MotorMoveAbs(id, target)) {
        OnStopNine(); SetSystemStatus(L"八轴目标超限或运动指令失败，已尝试停止九轴。"); return false;
    }
    return true;
}

bool CKongTan8dianjiDlg::MotorMoveRel(int id, double distance) {
    if (!m_nine.Ready()) return false;
    double current = 0;
    const auto limit = m_motorSoftLimits.find(id);
    if (!std::isfinite(distance) || limit == m_motorSoftLimits.end() || !m_motorCtrl.GetPositionActual(id, current)) {
        OnStopNine(); SetSystemStatus(L"八轴位移无效或位置读取失败，已尝试停止九轴。"); return false;
    }
    const double signedDistance = m_direction * distance;
    const double target = current + signedDistance;
    if (!std::isfinite(target) || target < limit->second.negLimit || target > limit->second.posLimit ||
        !m_motorCtrl.MotorMoveRel(id, signedDistance)) {
        OnStopNine(); SetSystemStatus(L"八轴目标超限或运动指令失败，已尝试停止九轴。"); return false;
    }
    return true;
}

void CKongTan8dianjiDlg::OnDemoSequence() {
    if (m_nine.Ready() || m_legacyBusy || m_sequence.Active()) return;
    inspection::Plan plan;
    const double count = IniNumber(m_configPath, L"Demo", L"BladeCount");
    const double direction = IniNumber(m_configPath, L"Demo", L"Direction");
    if (!Integer(count, 1, 10000) || !Integer(direction, -1, 1) || direction == 0) {
        SetDlgItemText(IDC_SEQUENCE_STATUS, L"演示参数无效，请检查 machine.ini 的 Demo 段。"); return;
    }
    plan.bladeCount = static_cast<unsigned>(count);
    plan.direction = static_cast<int>(direction);
    plan.firstAngleDeg = IniNumber(m_configPath, L"Demo", L"FirstBladeAngleDeg");
    if (!SetTimer(901, 100, nullptr) || !m_sequence.Start(plan, GetTickCount64())) {
        SetDlgItemText(IDC_SEQUENCE_STATUS, L"演示无法启动，请检查参数和定时器。"); return;
    }
    SetSystemStatus(L"离线演示中：不连接 CAN，不发送任何电机运动指令。");
    RefreshControls();
}

void CKongTan8dianjiDlg::OnTimer(UINT_PTR id) {
    if (id != 901) { CDialogEx::OnTimer(id); return; }
    if (m_nine.Ready()) {
        const bool wasBusy = m_nine.Busy();
        double position = 0;
        if (!m_nine.Poll(GetTickCount64(), position)) {
            SetSystemStatus(L"九轴监控停止：" + CString(m_nine.Error().c_str()));
            SetDlgItemText(IDC_FEED_POSITION, L"位置：反馈无效");
        } else {
            CString value; value.Format(L"进给节点 %d，位置 %.0f counts%s", m_feed.node, position, m_nine.Busy() ? L"（运动中）" : L"");
            SetDlgItemText(IDC_FEED_POSITION, value);
            if (wasBusy && !m_nine.Busy()) SetSystemStatus(L"进给轴已到达目标位置。");
        }
        RefreshControls();
    }
    if (m_sequence.Active()) {
        m_sequence.Tick(GetTickCount64());
        CString status;
        status.Format(L"演示：第 %u 片，%s\n已完成 %u 片", m_sequence.Blade() + 1, StateText(m_sequence.Current()), m_sequence.Completed());
        if (m_sequence.Current() == inspection::State::Fault) status += L"\n" + CString(m_sequence.Error().c_str());
        SetDlgItemText(IDC_SEQUENCE_STATUS, status);
        RefreshControls();
    }
}

BOOL CKongTan8dianjiDlg::OnCommand(WPARAM wParam, LPARAM lParam) {
    const UINT id = LOWORD(wParam);
    bool motion = false;
    for (UINT candidate : legacyMotion) if (candidate == id) motion = true;
    if (!motion) return CDialogEx::OnCommand(wParam, lParam);
    if (!m_nine.Ready() || m_nine.Busy() || m_legacyBusy || m_sequence.Active()) return TRUE;
    m_legacyBusy = true; RefreshControls();
    const BOOL handled = CDialogEx::OnCommand(wParam, lParam);
    // Original step handlers return immediately after sending MoveRel. Keep the
    // manual interlock until all robot axes finish, including the 7/8 pair.
    for (int axis = 1; axis <= 8 && m_nine.Ready(); ++axis)
        if (!WaitForMotorMotion(axis, 60000.0f)) break;
    m_legacyBusy = false; RefreshControls();
    if (m_closePending) OnCancel();
    return handled;
}

bool CKongTan8dianjiDlg::WaitForMotorMotion(int id, float timeoutMs) {
    if (!std::isfinite(timeoutMs) || timeoutMs <= 0) return false;
    const ULONGLONG start = GetTickCount64();
    while (m_nine.Ready()) {
        machine::AxisFeedback f;
        if (!m_port.Read(id, f) || !f.enabled || f.fault) {
            OnStopNine(); SetSystemStatus(L"八轴运动反馈故障，已尝试停止九轴。"); return false;
        }
        if (f.done) return true;
        if (GetTickCount64() - start > static_cast<ULONGLONG>(timeoutMs)) {
            OnStopNine(); SetSystemStatus(L"八轴运动等待超时，已尝试停止九轴。"); return false;
        }
        // Keep stop/close and monitor messages serviceable during inherited waits.
        // Reentrant motion commands are blocked by m_legacyBusy in OnCommand.
        MSG message;
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) { OnStopNine(); PostQuitMessage(static_cast<int>(message.wParam)); return false; }
            if (!IsDialogMessage(&message)) { TranslateMessage(&message); DispatchMessage(&message); }
        }
        Sleep(5);
    }
    return false;
}

void CKongTan8dianjiDlg::OnCancel() {
    OnStopNine(); // Exit never commands a return-to-zero movement.
    if (m_legacyBusy) { m_closePending = true; return; }
    KillTimer(901);
    CDialogEx::OnCancel();
}
