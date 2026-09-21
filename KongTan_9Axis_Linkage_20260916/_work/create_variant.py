from pathlib import Path
import shutil, re

dst = Path(__file__).resolve().parents[1]
src = dst.parent
def write(rel, text):
    p = dst / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text, encoding='utf-8-sig', newline='\n')
def read(p):
    b = p.read_bytes()
    try: return b.decode('utf-8-sig')
    except UnicodeDecodeError: return b.decode('gbk', errors='replace')
def replace_function(s, signature, body):
    start = s.index(signature)
    opening = s.index('{', start)
    level = 1
    end = opening + 1
    while level:
        if s[end] == '{': level += 1
        elif s[end] == '}': level -= 1
        end += 1
    return s[:start] + signature + '\n{\n' + body + '\n}\n' + s[end:]

for name in ['KongTan_8dianji.cpp','KongTan_8dianji.h','pch.cpp','pch.h','framework.h','targetver.h','packages.config','resource.h','KongTan_8dianji.sln','KongTan_8dianji.vcxproj','KongTan_8dianji.vcxproj.filters']:
    shutil.copy2(src/name, dst/name)
for rel in ['include/CML/inc','include/CML/lib/x64/vc16','include/CML/lib/x86','include/CML_lib-C++','include/ixxat/VCI-V3/inc','packages','res']:
    shutil.copytree(src/rel, dst/rel, dirs_exist_ok=True)
for p in (src/'include/ixxat').iterdir():
    if p.is_file(): shutil.copy2(p, dst/'include/ixxat'/p.name)

# Correct lifecycle management only in the delivered copy.
h = read(dst/'include/CML_lib-C++/CMLMotorCtrl.h')
h = h.replace('extern const std::set<CML::uint> MotorIds', 'extern std::set<CML::uint> MotorIds')
h = h.replace('bool m_bOpen;', 'std::set<CML::uint> m_initialized;\n\tbool m_bOpen;')
write('include/CML_lib-C++/CMLMotorCtrl.h', h)
s = read(dst/'include/CML_lib-C++/CMLMotorCtrl.cpp')
s = replace_function(s, 'CmlMotor::~CmlMotor()', '\tCloseMotorBus();')
s = replace_function(s, 'bool CmlMotor::OpenMotorBus()', '''    if (m_bOpen) return true;
    if (!m_initialized.empty() && !CloseMotorBus()) return false;
    if (!OpenCanCard()) { CloseMotorBus(); return false; }
    try {
        for (const uint id : MotorIds) {
            if (id < 1 || id > MaxMotorNum) { CloseMotorBus(); return false; }
            const int index = id - 1;
            AmpSettings[index].enableOnInit = FALSE;
            // Include a partially initialized node in the cleanup attempt.
            m_initialized.insert(id);
            err = ampObj[index].Init(canOpen, id, AmpSettings[index]);
            if (err) { TRACE("Node %u Init failed: %s\\n", id, err->toString()); CloseMotorBus(); return false; }
        }
    } catch (...) { CloseMotorBus(); return false; }
    m_bOpen = true;
    return true;''')
s = replace_function(s, 'bool CmlMotor::CloseMotorBus()', '''    bool ok = true;
    m_bOpen = false;
    // Stop every axis before starting disable/uninitialize; never return early.
    for (const uint id : m_initialized) {
        try { if (ampObj[id - 1].HaltMove()) ok = false; } catch (...) { ok = false; }
    }
    for (const uint id : m_initialized) {
        try { if (ampObj[id - 1].Disable()) ok = false; } catch (...) { ok = false; }
        try { if (ampObj[id - 1].UnInit()) ok = false; } catch (...) { ok = false; }
    }
    m_initialized.clear();
    if (m_bcanOpenOpen) {
        try { if (canOpen.Close()) ok = false; } catch (...) { ok = false; }
        m_bcanOpenOpen = false;
    }
    if (m_bcanOpen) {
        try { if (can.Close()) ok = false; } catch (...) { ok = false; }
        m_bcanOpen = false;
    }
    return ok;''')
s = replace_function(s, 'bool CmlMotor::EnableMotorBus()', '''    if (!m_bOpen) return false;
    for (const uint id : MotorIds) {
        if (!EnableMotor(id, true)) { CloseMotorBus(); return false; }
    }
    return true;''')
s = s.replace('ampObj[motorID - 1].Enable();', 'ampObj[motorID - 1].Enable(TRUE);')
write('include/CML_lib-C++/CMLMotorCtrl.cpp', s)

h = read(src/'KongTan_8dianjiDlg.h')
h = h.replace('#include <map>', '#include "CmlMotorPort.h"\n#include "BladeSequence.h"\n#include <map>')
h = h.replace('CmlMotor m_motorCtrl;', '''CmlMotor m_motorCtrl;
    CmlMotorPort m_port{m_motorCtrl};
    machine::NineAxisController m_nine{m_port};
    machine::FeedConfig m_feed;
    inspection::DemoBackend m_demoBackend;
    inspection::BladeSequence m_sequence{m_demoBackend};
    CString m_configPath;
    bool m_legacyBusy = false, m_closePending = false;
    bool LoadFeedConfig();
    void SetSystemStatus(const CString& text);
    void RefreshControls();
    void MoveFeed(bool backward, bool origin);
    bool WaitForMotorMotion(int id, float timeoutMs);
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
    virtual void OnCancel();
    virtual void OnOK() {} // Enter must not silently close an enabled machine.
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnConnectNine();
    afx_msg void OnStopNine();
    afx_msg void OnFeedForward();
    afx_msg void OnFeedBackward();
    afx_msg void OnFeedOrigin();
    afx_msg void OnDemoSequence();''')
write('KongTan_8dianjiDlg.h', h)
s = read(src/'KongTan_8dianjiDlg.cpp')
s = s.replace('const std::set<CML::uint> MotorIds', 'std::set<CML::uint> MotorIds')
s = s.replace('BEGIN_MESSAGE_MAP(CKongTan8dianjiDlg, CDialogEx)', '''BEGIN_MESSAGE_MAP(CKongTan8dianjiDlg, CDialogEx)
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_CONNECT_NINE, &CKongTan8dianjiDlg::OnConnectNine)
    ON_BN_CLICKED(IDC_STOP_NINE, &CKongTan8dianjiDlg::OnStopNine)
    ON_BN_CLICKED(IDC_FEED_FORWARD, &CKongTan8dianjiDlg::OnFeedForward)
    ON_BN_CLICKED(IDC_FEED_BACKWARD, &CKongTan8dianjiDlg::OnFeedBackward)
    ON_BN_CLICKED(IDC_FEED_ORIGIN, &CKongTan8dianjiDlg::OnFeedOrigin)
    ON_BN_CLICKED(IDC_DEMO_SEQUENCE, &CKongTan8dianjiDlg::OnDemoSequence)''')
a = s.index('\t// ========== 电机初始化开始')
b = s.index('\tm_theta_x =', a)
s = s[:a] + '''    // Opening the application never connects/enables hardware.
    wchar_t executable[32768] = {};
    GetModuleFileNameW(nullptr, executable, _countof(executable));
    m_configPath = executable;
    m_configPath = m_configPath.Left(m_configPath.ReverseFind(L'\\\\') + 1) + L"machine.ini";
    InitMotorSoftLimits();
    SetSystemStatus(L"未连接：核实 machine.ini 后，点击连接并使能九轴。离线演示无需连接。 ");
    RefreshControls();
    if (!SetTimer(901, 100, nullptr))
        SetSystemStatus(L"状态监控定时器创建失败，请重新启动程序。");
''' + s[b:]
s = replace_function(s, 'void CKongTan8dianjiDlg::OnBnClickedCcancel()', '    OnCancel();')
# Keep original all-reset scoped to the robot; feed origin is a separate command.
start = s.index('void CKongTan8dianjiDlg::OnBnClickedButton13()')
end = s.index('void CKongTan8dianjiDlg::ComputeMotorDeltas', start)
part = s[start:end].replace('for (const CML::uint& motorId : MotorIds)', 'for (CML::uint motorId = 1; motorId <= 8; ++motorId)').replace('所有电机正在复位', '八个机器人电机复位流程结束；进给轴请单独回起点')
s = s[:start] + part + s[end:]
s = s.replace('m_motorCtrl.WaitMoveDone(', 'WaitForMotorMotion(')
write('KongTan_8dianjiDlg.cpp', s)

# Restore readable captions and valid resource encoding from the damaged source.
raw = (src/'KongTan8dianji.rc').read_bytes().decode('gbk', errors='replace')
raw = raw.replace('\r','')
dialog = re.search(r'IDD_KONGTAN_8DIANJI_DIALOG DIALOGEX.*?\nEND', raw, re.S).group()
dialog = re.sub(r'DIALOGEX 0, 0, 396, 390', 'DIALOGEX 0, 0, 650, 390', dialog)
dialog = re.sub(r'CAPTION .*', 'CAPTION "孔探机器人：八轴 + 独立进给轴"', dialog)
dialog = re.sub(r'FONT .*', 'FONT 9, "Microsoft YaHei UI", 0, 0, 0x86', dialog)
captions = {f'IDC_BUTTON{i+1}': caption for i, caption in enumerate(['S1-上','S1-下','S1-上下复位','S1-左','S1-右','S1-左右复位','S2-勾','S2-回','S2-复位','S3-俯','S3-仰','S3-复位','八轴复位'])}
captions.update({'IDCCANCEL':'退出','IDC_BTN_EXECUTE_BENDING':'执行弯曲','IDC_BTN_IK_SOLVE':'逆解执行','IDC_BTN_PROPORTIONAL_HOOK':'比例勾动','IDC_CHECK_USE_POSE':'启用姿态约束','IDC_BTN_SET_PRESET':'预设姿态 0 / 250 / 0','IDC_BTN_LINEAR_TRAJECTORY':'直线轨迹'})
groups = iter(['第一段','第二段','第三段','弯曲角度','逆运动学','轨迹终点与步数'])
statics = iter(['速度','位移','速度','位移','速度','位移','速度','位移','角3','角4','平面','X','Z','Y','1','2','3','4','5','6','Rx','Ry','Rz','终点X','Y','Z','步数'])
lines=[]
for line in dialog.splitlines():
    if not line.strip(): continue
    if 'IDC_STATIC' in line and 'GROUPBOX' in line:
        line = re.sub(r'"[^"]*"', '"'+next(groups,'参数')+'"',line,count=1)
    elif 'IDC_STATIC' in line and 'LTEXT' in line:
        # Static labels regenerated from their original coordinates below.
        xy = re.search(r'IDC_STATIC,(\d+),(\d+)',line)
        x,y=map(int,xy.groups())
        label = '速度' if y in [18,49,102,153] and x<70 else '位移'
        mapping={(24,200):'角3',(98,200):'角4',(181,200):'平面',(24,237):'X',(106,237):'Z',(179,237):'Y',(21,267):'Rx',(92,268):'Ry',(159,268):'Rz',(24,364):'终点X',(90,364):'Y',(142,364):'Z',(200,364):'步数'}
        label=mapping.get((x,y), label)
        line = re.sub(r'"[^"]*"', '"'+label+'"',line,count=1)
    else:
        for control,caption in captions.items():
            if re.search(r','+control+r',',line):
                line=re.sub(r'"[^"]*"','"'+caption+'"',line,count=1); break
    lines.append(line)
dialog='\n'.join(lines)
extra='''    GROUPBOX "九轴连接与状态",IDC_STATIC,394,7,245,92
    PUSHBUTTON "连接并使能九轴",IDC_CONNECT_NINE,405,22,108,18
    PUSHBUTTON "停止 / 撤销使能",IDC_STOP_NINE,522,22,108,18
    LTEXT "未连接",IDC_NINE_STATUS,405,48,222,42
    GROUPBOX "独立进给轴（参数见 machine.ini）",IDC_STATIC,394,106,245,99
    PUSHBUTTON "进给 +",IDC_FEED_FORWARD,405,123,65,18
    PUSHBUTTON "退回 -",IDC_FEED_BACKWARD,482,123,65,18
    PUSHBUTTON "回连接起点",IDC_FEED_ORIGIN,559,123,69,18
    LTEXT "位置：未读取",IDC_FEED_POSITION,405,150,220,12
    LTEXT "单位：编码器计数；回起点不是限位开关寻零。",IDC_STATIC,405,172,221,22
    GROUPBOX "转子与机器人联动（离线流程演示）",IDC_STATIC,394,214,245,116
    PUSHBUTTON "演示逐叶片检测流程",IDC_DEMO_SEQUENCE,405,231,222,18
    LTEXT "演示：未启动",IDC_SEQUENCE_STATUS,405,259,220,32
    LTEXT "对准 / 锁相 → 跟踪检测 → 退出 → 下一叶片",IDC_STATIC,405,304,222,17
    LTEXT "实机同步接口待接入：编码器、控制器、轨迹及检测触发。",IDC_STATIC,405,344,224,29
'''
dialog=dialog.rsplit('END',1)[0]+extra+'END\n'
resource='''#include "resource.h"
#include "afxres.h"
#pragma code_page(65001)
LANGUAGE LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED
IDR_MAINFRAME ICON "res\\\\KongTan_8dianji.ico"
IDD_ABOUTBOX DIALOGEX 0, 0, 200, 65
STYLE DS_SETFONT | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
CAPTION "关于九轴扩展版"
FONT 9, "Microsoft YaHei UI", 0, 0, 0x86
BEGIN
 LTEXT "八轴机器人 + 进给轴 / 叶片联动接口",IDC_STATIC,10,10,180,20
 DEFPUSHBUTTON "确定",IDOK,140,42,50,14
END
''' + dialog + '''
STRINGTABLE
BEGIN
 IDS_ABOUTBOX "关于九轴扩展版(&A)..."
END
'''
write('KongTan8dianji.rc',resource)
rh=read(dst/'resource.h')
for i,name in enumerate(['CONNECT_NINE','STOP_NINE','FEED_FORWARD','FEED_BACKWARD','FEED_ORIGIN','DEMO_SEQUENCE','NINE_STATUS','FEED_POSITION','SEQUENCE_STATUS']):
    rh += f'\n#define IDC_{name} {1100+i}\n'
write('resource.h',rh)

project=read(dst/'KongTan_8dianji.vcxproj')
project=project.replace('<ClInclude Include="framework.h" />','<ClInclude Include="NineAxisController.h" /><ClInclude Include="CmlMotorPort.h" /><ClInclude Include="BladeSequence.h" />\n    <ClInclude Include="framework.h" />')
project=project.replace('<ClCompile Include="KongTan_8dianji.cpp" />','<ClCompile Include="NineAxisUi.cpp" />\n    <ClCompile Include="KongTan_8dianji.cpp" />')
project=project.replace('<PropertyGroup Label="UserMacros" />','''<PropertyGroup Label="UserMacros" />
  <PropertyGroup><TargetName>KongTan_9Axis_Linkage</TargetName></PropertyGroup>
  <Target Name="CopyMachineConfig" AfterTargets="Build">
    <Copy SourceFiles="$(ProjectDir)machine.ini" DestinationFolder="$(OutDir)" Condition="!Exists('$(OutDir)machine.ini')" />
  </Target>''')
project=project.replace('<AdditionalOptions>/utf-8', '<LanguageStandard>stdcpp17</LanguageStandard>\n      <AdditionalOptions>/utf-8')
write('KongTan_8dianji.vcxproj',project)
print('Independent variant created:',dst)
