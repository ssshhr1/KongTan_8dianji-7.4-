#include "pch.h"
#include "framework.h"
#include "KongTan_8dianji.h"
#include "KongTan_8dianjiDlg.h"
#include "afxdialogex.h"
#define _USE_MATH_DEFINES
#include <cmath>
#include <vector>
#include <algorithm>

using namespace Eigen;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//————————————————————————————————————————
const std::set<CML::uint> MotorIds = { 1, 2, 3, 4, 5, 6, 7, 8 }; // 电机ID
//————————————————————————————————————————


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CKongTan8dianjiDlg 对话框



CKongTan8dianjiDlg::CKongTan8dianjiDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_KONGTAN_8DIANJI_DIALOG, pParent)
	, motor_V12(30000)
	, motor_V34(30000)
	, motor_V56(30000)
	, motor_V78(30000)
	, motor_S12(10000)
	, motor_S34(10000)
	, motor_S56(10000)
	, motor_S78(10000)
	, m_bendAngle3(0.0)
	, m_bendAngle4(0.0)
	, m_planeAngle(0.0)
	, m_ikX(0)
	, m_ikY(0)
	, m_ikZ(0)
	, m_delta1(0)
	, m_delta2(0)
	, m_delta3(0)
	, m_delta4(0)
	, m_delta5(0)
	, m_delta6(0)
	, m_usePose(TRUE)
	, m_trajEndX(32.0)
	, m_trajEndY(0.0)
	, m_trajEndZ(85.0)
	, m_trajSteps(11)
	, m_resetSpeedRatio(4.0)
	, m_accelSteps(3)
	, m_loadThreshold(0.0)
	, m_direction(-1)

{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKongTan8dianjiDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, motor_V12);
	DDX_Text(pDX, IDC_EDIT2, motor_V34);
	DDX_Text(pDX, IDC_EDIT3, motor_V56);
	DDX_Text(pDX, IDC_EDIT4, motor_V78);
	DDX_Text(pDX, IDC_EDIT5, motor_S12);
	DDX_Text(pDX, IDC_EDIT6, motor_S34);
	DDX_Text(pDX, IDC_EDIT7, motor_S56);
	DDX_Text(pDX, IDC_EDIT8, motor_S78);
	DDX_Text(pDX, IDC_EDIT_BEND3, m_bendAngle3);
	DDX_Text(pDX, IDC_EDIT_BEND4, m_bendAngle4);
	DDX_Text(pDX, IDC_EDIT_PLANE, m_planeAngle);
	DDX_Text(pDX, IDC_EDIT_IK_X, m_ikX);
	DDX_Text(pDX, IDC_EDIT_IK_Z, m_ikZ);
	DDX_Text(pDX, IDC_EDIT_IK_Y, m_ikY);
	DDX_Text(pDX, IDC_EDIT9, m_delta1);
	DDX_Text(pDX, IDC_EDIT10, m_delta2);
	DDX_Text(pDX, IDC_EDIT11, m_delta3);
	DDX_Text(pDX, IDC_EDIT12, m_delta4);
	DDX_Text(pDX, IDC_EDIT13, m_delta5);
	DDX_Text(pDX, IDC_EDIT14, m_delta6);
	DDX_Text(pDX, IDC_EDIT_ROLL, m_theta_x);
	DDX_Text(pDX, IDC_EDIT_PITCH, m_theta_y);
	DDX_Text(pDX, IDC_EDIT_YAW, m_theta_z);
	DDX_Check(pDX, IDC_CHECK_USE_POSE, m_usePose);
	DDX_Text(pDX, IDC_EDIT_TRAJ_END_X, m_trajEndX);
	DDX_Text(pDX, IDC_EDIT_TRAJ_END_Y, m_trajEndY);
	DDX_Text(pDX, IDC_EDIT_TRAJ_END_Z, m_trajEndZ);
	DDX_Text(pDX, IDC_EDIT_TRAJ_STEPS, m_trajSteps);
}

BEGIN_MESSAGE_MAP(CKongTan8dianjiDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CKongTan8dianjiDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CKongTan8dianjiDlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CKongTan8dianjiDlg::OnBnClickedButton3)
	ON_BN_CLICKED(IDC_BUTTON4, &CKongTan8dianjiDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CKongTan8dianjiDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CKongTan8dianjiDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CKongTan8dianjiDlg::OnBnClickedButton7)
	ON_BN_CLICKED(IDC_BUTTON8, &CKongTan8dianjiDlg::OnBnClickedButton8)
	ON_BN_CLICKED(IDC_BUTTON9, &CKongTan8dianjiDlg::OnBnClickedButton9)
	ON_BN_CLICKED(IDC_BUTTON10, &CKongTan8dianjiDlg::OnBnClickedButton10)
	ON_BN_CLICKED(IDC_BUTTON11, &CKongTan8dianjiDlg::OnBnClickedButton11)
	ON_BN_CLICKED(IDC_BUTTON12, &CKongTan8dianjiDlg::OnBnClickedButton12)
	ON_BN_CLICKED(IDC_BUTTON13, &CKongTan8dianjiDlg::OnBnClickedButton13)
	ON_BN_CLICKED(IDCCANCEL, &CKongTan8dianjiDlg::OnBnClickedCcancel)
	ON_BN_CLICKED(IDC_BTN_EXECUTE_BENDING, &CKongTan8dianjiDlg::OnBnClickedBtnExecuteBending)
	ON_EN_CHANGE(IDC_EDIT9, &CKongTan8dianjiDlg::OnEnChangeEdit9)
	ON_BN_CLICKED(IDC_BTN_IK_SOLVE, &CKongTan8dianjiDlg::OnBnClickedBtnIkSolve)
	ON_BN_CLICKED(IDC_BTN_PROPORTIONAL_HOOK, &CKongTan8dianjiDlg::OnBnClickedBtnProportionalHook)
	ON_BN_CLICKED(IDC_BTN_SET_PRESET, &CKongTan8dianjiDlg::OnBnClickedBtnSetPreset)
	ON_BN_CLICKED(IDC_BTN_LINEAR_TRAJECTORY, &CKongTan8dianjiDlg::OnBnClickedBtnLinearTrajectory)
END_MESSAGE_MAP()

// 初始化电机软限位配置
void CKongTan8dianjiDlg::InitMotorSoftLimits()
{
	// 清空现有配置
	m_motorSoftLimits.clear();
	// 电机1: ±90°弯角时delta1最大约227k，留余量1M
	m_motorSoftLimits[1] = { 2000000.0, -2000000.0 };
	// 电机2
	m_motorSoftLimits[2] = { 3000000.0, -3000000.0 };
	// 电机3
	m_motorSoftLimits[3] = { 3000000.0, -3000000.0 };
	// 电机4
	m_motorSoftLimits[4] = { 2000000.0, -2000000.0 };
	// 电机5: ±90°弯角时delta5约±270k，留余量1M
	m_motorSoftLimits[5] = { 2000000.0, -2000000.0 };
	// 电机6: ±90°弯角时delta6约±810k，留余量2M
	m_motorSoftLimits[6] = { 3000000.0, -3000000.0 };
	// 电机7
	m_motorSoftLimits[7] = { 50000.0, -50000.0 };
	// 电机8
	m_motorSoftLimits[8] = { 50000.0, -50000.0 };

	//TRACE(_T("Motor soft limits configured for %d motors\n"), m_motorSoftLimits.size());
}

// CKongTan8dianjiDlg 消息处理程序

BOOL CKongTan8dianjiDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	// ========== 电机初始化开始 ==========
	// 1. 打开电机总线
	if (!m_motorCtrl.OpenMotorBus())
	{
		TRACE(_T("OpenMotorBus failed!\n"));
		MessageBox(_T("电机总线打开失败！"), _T("错误"), MB_ICONERROR);
	}
	else
	{
		// 2. 使能所有电机
		if (!m_motorCtrl.EnableMotorBus())
		{
			TRACE(_T("EnableMotorBus failed!\n"));
			MessageBox(_T("电机使能失败！"), _T("错误"), MB_ICONERROR);
		}
		else
		{
			// 3. 记录每个电机的上电位置为零位
			TRACE(_T("Number of motors to initialize: %d\n"), MotorIds.size());

			for (const CML::uint& motorId : MotorIds)
			{
				double currentPos = 0.0;
				if (m_motorCtrl.GetPositionActual(motorId, currentPos))
				{
					m_motorZeroPos[motorId] = currentPos;
					TRACE(_T("Motor %d zero position: %f\n"), motorId, currentPos);
				}
				else
				{
					TRACE(_T("GetPositionActual for motor %d failed!\n"), motorId);
				}
			}
			// 4. 为电机设置软限位
			InitMotorSoftLimits();
			for (const CML::uint& motorId : MotorIds)
			{
				auto it = m_motorSoftLimits.find(motorId);
				if (it != m_motorSoftLimits.end())
				{
					double posLimit = it->second.posLimit;
					double negLimit = it->second.negLimit;

					if (!m_motorCtrl.SetSoftLimit(motorId, posLimit, negLimit))
					{
						TRACE(_T("SetSoftLimit for motor %d failed!\n"), motorId);
					}
					else
					{
						TRACE(_T("SetSoftLimit for motor %d success! posLimit=%.0f, negLimit=%.0f\n"),
							motorId, posLimit, negLimit);
					}
				}
			else
			{
				// 电机未配置软限位，跳过
				TRACE(_T("Warning: No soft limit config for motor %d, skipping...\n"), motorId);
			}
			}
			//TRACE(_T("Motor initialization completed! Total motors initialized: %d\n"), m_motorZeroPos.size());
		}
	}
	// ========== 电机初始化结束 ==========
	m_theta_x = 0.0;
	m_theta_y = 250.0;   // 可根据需要改为 250.0 以匹配 zuobioadianweizi.cpp 示例
	m_theta_z = 0.0;
	m_bendAngle3 = 0.0;
	m_bendAngle4 = 0.0;
	m_planeAngle = 0.0;
	m_ikX = 60.0;
	m_ikY = 0.0;
	m_ikZ = 100.0;
	UpdateData(FALSE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CKongTan8dianjiDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CKongTan8dianjiDlg::OnPaint()
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
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CKongTan8dianjiDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CKongTan8dianjiDlg::OnBnClickedButton1()  //S1-上
{
	UpdateData(TRUE);

	// 检查电机1和电机2是否在MotorIds中
	if (MotorIds.find(1) == MotorIds.end() || MotorIds.find(2) == MotorIds.end())
	{
		TRACE(_T("Motor 1 or 2 not available!\n"));
		MessageBox(_T("电机1或2不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机1正转，电机2反转
	if (m_motorCtrl.SetMotorVelocity(1, motor_V12, 200000, 200000))
	{
		MotorMoveRel(1, motor_S12);
	}

	if (m_motorCtrl.SetMotorVelocity(2, motor_V12, 200000, 200000))
	{
		MotorMoveRel(2, -motor_S12);
	}

	//TRACE(_T("S1-UP: Motor1 +%.0f, Motor2 -%.0f, Speed=%.0f\n"), motor_S12, motor_S12, motor_V12);
}

void CKongTan8dianjiDlg::OnBnClickedButton2()  //S1-下
{
	UpdateData(TRUE);

	// 检查电机1和电机2是否在MotorIds中
	if (MotorIds.find(1) == MotorIds.end() || MotorIds.find(2) == MotorIds.end())
	{
		TRACE(_T("Motor 1 or 2 not available!\n"));
		MessageBox(_T("电机1或2不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机1反转，电机2正转
	if (m_motorCtrl.SetMotorVelocity(1, motor_V12, 200000, 200000))
	{
		MotorMoveRel(1, -motor_S12);
	}

	if (m_motorCtrl.SetMotorVelocity(2, motor_V12, 200000, 200000))
	{
		MotorMoveRel(2, motor_S12);
	}

	//TRACE(_T("S1-DOWN: Motor1 -%.0f, Motor2 +%.0f, Speed=%.0f\n"), motor_S12, motor_S12, motor_V12);
}


void CKongTan8dianjiDlg::OnBnClickedButton3()  //S1-上下复位
{
	// 检查电机1和电机2是否在MotorIds中
	if (MotorIds.find(1) == MotorIds.end() || MotorIds.find(2) == MotorIds.end())
	{
		TRACE(_T("Motor 1 or 2 not available!\n"));
		MessageBox(_T("电机1或2不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 同步梯形复位：电机1和2同时启停、同时到达零位
	// 复位速度 = 启动速度(motor_V12) × 比例系数(m_resetSpeedRatio)
	SyncedTrapezoidalReset({ 1, 2 });
}

void CKongTan8dianjiDlg::OnBnClickedButton4()  //S1-左
{
	UpdateData(TRUE);

	// 检查电机3和电机4是否在MotorIds中
	if (MotorIds.find(3) == MotorIds.end() || MotorIds.find(4) == MotorIds.end())
	{
		TRACE(_T("Motor 3 or 4 not available!\n"));
		MessageBox(_T("电机3或4不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机3正转，电机4反转
	if (m_motorCtrl.SetMotorVelocity(3, motor_V34, 200000, 200000))
	{
		MotorMoveRel(3, motor_S34);
	}

	if (m_motorCtrl.SetMotorVelocity(4, motor_V34, 200000, 200000))
	{
		MotorMoveRel(4, -motor_S34);
	}

	//TRACE(_T("S1-LEFT: Motor3 +%.0f, Motor4 -%.0f, Speed=%.0f\n"), motor_S34, motor_S34, motor_V34);
}

void CKongTan8dianjiDlg::OnBnClickedButton5()  //S1-右
{
	UpdateData(TRUE);

	// 检查电机3和电机4是否在MotorIds中
	if (MotorIds.find(3) == MotorIds.end() || MotorIds.find(4) == MotorIds.end())
	{
		TRACE(_T("Motor 3 or 4 not available!\n"));
		MessageBox(_T("电机3或4不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机3反转，电机4正转
	if (m_motorCtrl.SetMotorVelocity(3, motor_V34, 200000, 200000))
	{
		MotorMoveRel(3, -motor_S34);
	}

	if (m_motorCtrl.SetMotorVelocity(4, motor_V34, 200000, 200000))
	{
		MotorMoveRel(4, motor_S34);
	}

	//TRACE(_T("S1-RIGHT: Motor3 -%.0f, Motor4 +%.0f, Speed=%.0f\n"), motor_S34, motor_S34, motor_V34);
}

void CKongTan8dianjiDlg::OnBnClickedButton6()  //S1-左右复位
{
	// 检查电机3和电机4是否在MotorIds中
	if (MotorIds.find(3) == MotorIds.end() || MotorIds.find(4) == MotorIds.end())
	{
		TRACE(_T("Motor 3 or 4 not available!\n"));
		MessageBox(_T("电机3或4不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 同步梯形复位：电机3和4同时启停、同时到达零位
	// 复位速度 = 启动速度(motor_V34) × 比例系数(m_resetSpeedRatio)
	SyncedTrapezoidalReset({ 3, 4 });
}

void CKongTan8dianjiDlg::OnBnClickedButton7()  //S2-勾
{
	UpdateData(TRUE);

	// 检查电机5和电机6是否在MotorIds中
	if (MotorIds.find(5) == MotorIds.end() || MotorIds.find(6) == MotorIds.end())
	{
		TRACE(_T("Motor 5 or 6 not available!\n"));
		MessageBox(_T("电机5或6不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机5正转，电机6反转
	if (m_motorCtrl.SetMotorVelocity(5, 2.5 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(5, 2.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(6, motor_V56, 200000, 200000))
	{
		MotorMoveRel(6,-motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(1, 1.5 / 1.1 * motor_V56, 200000, 200000))//M1
	{
		MotorMoveRel(1, 1.5 / 1.1 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(2, 4.25 / 1.5 * motor_V56, 200000, 200000))//M2
	{
		MotorMoveRel(2, -4.25 / 1.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(3, 2.75 / 1.5 * motor_V56, 200000, 200000))//M3
	{
		MotorMoveRel(3, -2.75 / 1.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(4, 0.34 / 1.5 * motor_V56, 200000, 200000))//M4
	{
		MotorMoveRel(4, -0.34 / 1.5 * motor_S56);
	}
	//TRACE(_T("S2-HOOK: Motor5 +%.0f, Motor6 -%.0f, Speed=%.0f\n"), motor_S56, motor_S56, motor_V56);
}

void CKongTan8dianjiDlg::OnBnClickedButton8()  //S2-回
{
	UpdateData(TRUE);

	// 检查电机5和电机6是否在MotorIds中
	if (MotorIds.find(5) == MotorIds.end() || MotorIds.find(6) == MotorIds.end())
	{
		TRACE(_T("Motor 5 or 6 not available!\n"));
		MessageBox(_T("电机5或6不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机5反转，电机6正转
	if (m_motorCtrl.SetMotorVelocity(5, motor_V56, 200000, 200000))
	{
		MotorMoveRel(5, -motor_S56);
	}

	if (m_motorCtrl.SetMotorVelocity(6, 3 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(6, 3 * motor_S56);
	}

	if (m_motorCtrl.SetMotorVelocity(1, 1.5 / 1.1 * motor_V56, 200000, 200000))//M1
	{
		MotorMoveRel(1, -1.5 / 1.1 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(2, 4.25 / 1.5 * motor_V56, 200000, 200000))//M2
	{
		MotorMoveRel(2, 4.25 / 1.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(3, 2.75 / 1.5 * motor_V56, 200000, 200000))//M3
	{
		MotorMoveRel(3, 2.75 / 1.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(4, 0.34 / 1.5 * motor_V56, 200000, 200000))//M4
	{
		MotorMoveRel(4, 0.34 / 1.5 * motor_S56);
	}
	//TRACE(_T("S2-BACK: Motor5 -%.0f, Motor6 +%.0f, Speed=%.0f\n"), motor_S56, motor_S56, motor_V56);
}

void CKongTan8dianjiDlg::OnBnClickedButton9()  //S2-复位
{
	// 检查电机5和电机6是否在MotorIds中
	if (MotorIds.find(5) == MotorIds.end() || MotorIds.find(6) == MotorIds.end())
	{
		TRACE(_T("Motor 5 or 6 not available!\n"));
		MessageBox(_T("电机5或6不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 同步梯形复位：电机5和6同时启停、同时到达零位
	// 复位速度 = 启动速度(motor_V56) × 比例系数(m_resetSpeedRatio)
	SyncedTrapezoidalReset({ 5, 6 });
}

void CKongTan8dianjiDlg::OnBnClickedButton10()  //S3-俯
{
	UpdateData(TRUE);

	// 检查电机7和电机8是否在MotorIds中
	if (MotorIds.find(7) == MotorIds.end() || MotorIds.find(8) == MotorIds.end())
	{
		TRACE(_T("Motor 7 or 8 not available!\n"));
		MessageBox(_T("电机7或8不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机7正转，电机8反转
	if (m_motorCtrl.SetMotorVelocity(7, motor_V78, 200000, 200000))
	{
		MotorMoveRel(7, motor_S78);
	}

	if (m_motorCtrl.SetMotorVelocity(8, motor_V78, 200000, 200000))
	{
		MotorMoveRel(8, -motor_S78);
	}

	//TRACE(_T("S3-DOWN: Motor7 +%.0f, Motor8 -%.0f, Speed=%.0f\n"), motor_S78, motor_S78, motor_V78);
}

void CKongTan8dianjiDlg::OnBnClickedButton11()  //S3-仰
{
	// 更新界面数据到变量
	UpdateData(TRUE);

	// 检查电机7和电机8是否在MotorIds中
	if (MotorIds.find(7) == MotorIds.end() || MotorIds.find(8) == MotorIds.end())
	{
		TRACE(_T("Motor 7 or 8 not available!\n"));
		MessageBox(_T("电机7或8不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 电机7反转，电机8正转
	if (m_motorCtrl.SetMotorVelocity(7, motor_V78, 200000, 200000))
	{
		MotorMoveRel(7, -motor_S78);
	}

	if (m_motorCtrl.SetMotorVelocity(8, motor_V78, 200000, 200000))
	{
		MotorMoveRel(8, motor_S78);
	}

	//TRACE(_T("S3-UP: Motor7 -%.0f, Motor8 +%.0f, Speed=%.0f\n"), motor_S78, motor_S78, motor_V78);
}

void CKongTan8dianjiDlg::OnBnClickedButton12()  //S3-复位
{
	// 检查电机7和电机8是否在MotorIds中
	if (MotorIds.find(7) == MotorIds.end() || MotorIds.find(8) == MotorIds.end())
	{
		TRACE(_T("Motor 7 or 8 not available!\n"));
		MessageBox(_T("电机7或8不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 同步梯形复位：电机7和8同时启停、同时到达零位
	// 复位速度 = 启动速度(motor_V78) × 比例系数(m_resetSpeedRatio)
	SyncedTrapezoidalReset({ 7, 8 });
}

void CKongTan8dianjiDlg::OnBnClickedButton13()  //全部复位
{
	// 同步梯形复位：所有电机同时启停、同时到达零位
	// 每个电机复位速度 = 对应启动速度(motor_Vxx) × 比例系数(m_resetSpeedRatio)
	std::vector<int> allMotorIDs;
	for (const CML::uint& motorId : MotorIds)
	{
		if (m_motorZeroPos.find(motorId) != m_motorZeroPos.end())
			allMotorIDs.push_back((int)motorId);
		else
			TRACE(_T("All Reset: Motor %d has no zero position record!\n"), motorId);
	}
	SyncedTrapezoidalReset(allMotorIDs);

	MessageBox(_T("所有电机正在复位！"), _T("提示"), MB_ICONINFORMATION);
}


void CKongTan8dianjiDlg::ComputeMotorDeltas(double kappa3, double kappa4, double plane_angle,
	double& delta1, double& delta2, double& delta3,
	double& delta4, double& delta5, double& delta6)
{
	const double L3 = 11.0;      // mm
	const double L4 = 15.91;     // mm

	// 与 zuobioadianweizi.cpp 完全一致
	delta5 = 1.5 * L3 * kappa3 * 4096 * 28;
	delta6 = -4.5 * L3 * kappa3 * 4096 * 28;

	// 注意：角度偏置使用弧度，公式中的常数沿用原文件
	double deg25 = 25.0 * M_PI / 180.0;
	double deg75 = 75.0 * M_PI / 180.0;
	double deg245 = 245.0 * M_PI / 180.0;
	double deg165 = 165.0 * M_PI / 180.0;

	delta1 = 1.26 * L3 * kappa3 * 4096 * 28 + 3.05 * cos(plane_angle - deg25) * L4 * kappa4 * 4096 * 28;
	delta4 = -0.21 * L3 * kappa3 * 4096 * 28 + 3.05 * cos(plane_angle + deg75) * L4 * kappa4 * 4096 * 28;
	delta3 = -2.79 * L3 * kappa3 * 4096 * 28 + 3.05 * cos(plane_angle + deg245) * L4 * kappa4 * 4096 * 28;
	delta2 = -4.26 * L3 * kappa3 * 4096 * 28 + 3.05 * cos(plane_angle + deg165) * L4 * kappa4 * 4096 * 28;
}
void CKongTan8dianjiDlg::ExecuteBendingMotion(double bend_angle3_deg, double bend_angle4_deg, double plane_angle_deg)
{
	const double L3 = 11.0;      
	const double L4 = 15.91;

	// 角度转弧度
	double bend3_rad = bend_angle3_deg * M_PI / 180.0;
	double bend4_rad = bend_angle4_deg * M_PI / 180.0;
	double plane_rad = plane_angle_deg * M_PI / 180.0;

	// 曲率 (1/mm)
	double kappa3 = (std::abs(bend3_rad) < 1e-6) ? 0.0 : bend3_rad / L3;
	double kappa4 = (std::abs(bend4_rad) < 1e-6) ? 0.0 : bend4_rad / L4;

	// 检查曲率边界（避免钢丝过拉）
	if (std::abs(kappa3) > 0.5 || std::abs(kappa4) > 0.5) {
		MessageBox(_T("曲率超出安全范围 (±0.5 1/mm)！"), _T("警告"), MB_ICONWARNING);
		return;
	}

	// 计算电机推拉量 (mm)
	double d1, d2, d3, d4, d5, d6;
	ComputeMotorDeltas(kappa3, kappa4, plane_rad, d1, d2, d3, d4, d5, d6);

	// 获取当前各电机位置，检查软限位（防止超限）
	auto it_limit = m_motorSoftLimits.find(1);
	if (it_limit != m_motorSoftLimits.end()) {
		// 这里可以增加实际位置校验，为了简洁只做打印
		TRACE(_T("Delta: M1=%.2f M2=%.2f M3=%.2f M4=%.2f M5=%.2f M6=%.2f\n"),
			d1, d2, d3, d4, d5, d6);
	}

	// ── 同步运动：速度按行程正比例分配，所有电机同时启停 ──
	struct MotorInfo { int id; double dist; double speed; double accel; };
	std::vector<MotorInfo> motors;
	double maxDist = 0.0;
	int motorIds[] = { 1, 2, 3, 4, 5, 6 };
	double deltas[] = { d1, d2, d3, d4, d5, d6 };

	for (int i = 0; i < 6; i++)
	{
		int id = motorIds[i];
		if (MotorIds.find(id) == MotorIds.end()) continue;
		double dist = std::abs(deltas[i]);
		if (dist < 0.5) continue;
		if (dist > maxDist) maxDist = dist;
		motors.push_back({ id, dist, 0.0, 0.0 });
	}

	if (!motors.empty() && maxDist >= 0.5)
	{
		for (auto& m : motors)
		{
			double startupSpeed = GetStartupSpeed(m.id) * m_resetSpeedRatio;
			m.speed = startupSpeed * (m.dist / maxDist);
			if (m.speed < 2000.0) m.speed = 2000.0;
			m.accel = 200000.0;
			m_motorCtrl.SetMotorVelocity(m.id, m.speed, m.accel, m.accel);
		}

		for (int i = 0; i < 6; i++)
		{
			int id = motorIds[i];
			if (MotorIds.find(id) == MotorIds.end()) continue;
			MotorMoveRel(id, deltas[i]);
		}

		for (auto& m : motors)
		{
			double timeoutMs = (std::max)(m.dist / (m.speed + 1.0) * 2.0 * 1000.0, 5000.0);
			if (timeoutMs > 60000.0) timeoutMs = 60000.0;
			m_motorCtrl.WaitMoveDone(m.id, (float)timeoutMs);
		}
	}

	TRACE(_T("弯曲运动已执行：θ3=%.2f°, θ4=%.2f°, φ=%.2f°\n"),
		bend_angle3_deg, bend_angle4_deg, plane_angle_deg);
}
void CKongTan8dianjiDlg::OnBnClickedBtnExecuteBending()
{
	UpdateData(TRUE);   // 从界面读取角度值
	ExecuteBendingMotion(m_bendAngle3, m_bendAngle4, m_planeAngle);
}

void CKongTan8dianjiDlg::OnBnClickedCcancel()  //退出
{
	// 先执行全部复位（同步梯形复位）
	TRACE(_T("Exit button clicked, resetting all motors...\n"));

	std::vector<int> allMotorIDs;
	for (const CML::uint& motorId : MotorIds)
	{
		if (m_motorZeroPos.find(motorId) != m_motorZeroPos.end())
			allMotorIDs.push_back((int)motorId);
	}
	SyncedTrapezoidalReset(allMotorIDs);

	// 等待所有电机运动完成
	TRACE(_T("Waiting for motors to complete movement...\n"));
	for (const CML::uint& motorId : MotorIds)
	{
		m_motorCtrl.WaitMoveDone(motorId, 5000.0f);
	}

	// 关闭电机总线
	m_motorCtrl.CloseMotorBus();

	TRACE(_T("Exiting application...\n"));

	// 关闭对话框
	CDialogEx::OnCancel();
}
// ========== 辅助函数：复位速度比例调节 + 梯形加减速（同时启停、同时到达）==========

double CKongTan8dianjiDlg::GetStartupSpeed(int motorID)
{
	switch (motorID)
	{
	case 1: case 2: return motor_V12;
	case 3: case 4: return motor_V34;
	case 5: case 6: return motor_V56;
	case 7: case 8: return motor_V78;
	default:  return 5000.0;  // 默认启动速度
	}
}

void CKongTan8dianjiDlg::SyncedTrapezoidalReset(const std::vector<int>& motorIDs)
{
	if (motorIDs.empty()) return;

	// ── 第1步：获取所有电机的当前位置和目标（零位）位置 ──
	struct MotorInfo {
		int id;
		double currentPos;
		double targetPos;
		double distance;
		double direction;    // +1 或 -1
		double velocity;     // 按距离比例缩放后的目标速度
		double accel;        // 按距离比例缩放后的加速度
	};
	std::vector<MotorInfo> motors;
	double maxDist = 0.0;

	for (int id : motorIDs)
	{
		if (MotorIds.find(id) == MotorIds.end()) continue;

		MotorInfo m;
		m.id = id;

		// 获取当前实际位置
		if (!m_motorCtrl.GetPositionActual(id, m.currentPos))
		{
			TRACE(_T("SyncedReset: 无法获取电机 %d 当前位置\n"), id);
			continue;
		}

		// 获取零位
		auto it = m_motorZeroPos.find(id);
		if (it == m_motorZeroPos.end())
		{
			TRACE(_T("SyncedReset: 电机 %d 无零位记录\n"), id);
			continue;
		}
		m.targetPos = it->second;
		m.distance = std::abs(m.targetPos - m.currentPos);
		m.direction = (m.targetPos > m.currentPos) ? 1.0 : -1.0;

		if (m.distance > maxDist) maxDist = m.distance;
		motors.push_back(m);
	}

	if (motors.empty() || maxDist < 1.0) return;  // 已在目标位置

	// ── 第2步：按正比例计算速度，行程越大速度越大（同时启停，防钢丝拉断）──
	// 所有电机同时到达，保证推拉同步
	for (auto& m : motors)
	{
		double startupSpeed = GetStartupSpeed(m.id) * m_resetSpeedRatio;
		double ratio = m.distance / maxDist;
		m.velocity = startupSpeed * ratio;
		if (m.velocity < 2000.0) m.velocity = 2000.0;
		m.accel = 200000.0;
	}

	TRACE(_T("SyncedReset: %zu motors, maxDist=%.0f\n"), motors.size(), maxDist);

	// ── 第3步：负载检测（如有配置）──
	if (m_loadThreshold > 0.0)
	{
		for (auto& m : motors)
		{
			CML::int16 currentActual = m_motorCtrl.GetCurrentActual(m.id);
			if (std::abs((double)currentActual) > m_loadThreshold)
			{
				m.velocity *= 0.5;
				m.accel *= 0.5;
				TRACE(_T("SyncedReset: 电机 %d 负载过高(current=%d), 降速到 %.0f\n"),
					m.id, (int)currentActual, m.velocity);
			}
		}
	}

	// ── 第4步：先设置所有电机的运动参数（速度+加速度）──
	// 分两个循环确保所有电机参数设置完毕后再统一启动
	for (auto& m : motors)
	{
		m_motorCtrl.SetMotorVelocity(m.id, m.velocity, m.accel, m.accel);
	}

	// ── 第5步：再同时发出所有电机的绝对定位指令（确保同时启动）──
	for (auto& m : motors)
	{
		m_motorCtrl.MotorMoveAbs(m.id, m.targetPos);
	}

	// ── 第6步：等待所有电机运动完成 ──
	for (auto& m : motors)
	{
		double estTimeSec = m.distance / (m.velocity + 1.0);
		double timeoutMs = (std::max)(estTimeSec * 2.0 * 1000.0, 5000.0);
		if (timeoutMs > 60000.0) timeoutMs = 60000.0;

		if (!m_motorCtrl.WaitMoveDone(m.id, (float)timeoutMs))
		{
			TRACE(_T("SyncedReset: 电机 %d WaitMoveDone 超时 (%.0f ms)\n"), m.id, timeoutMs);
		}
	}

	TRACE(_T("SyncedReset: 所有电机复位完成\n"));
}
// ========== 运动学辅助函数实现 ==========
Matrix3d CKongTan8dianjiDlg::Rx(double a) {
	Matrix3d R;
	R << 1, 0, 0,
		0, cos(a), -sin(a),
		0, sin(a), cos(a);
	return R;
}
Matrix3d CKongTan8dianjiDlg::Rz(double a) {
	Matrix3d R;
	R << cos(a), -sin(a), 0,
		sin(a), cos(a), 0,
		0, 0, 1;
	return R;
}
Matrix3d CKongTan8dianjiDlg::Ry(double a) {
	Matrix3d R;
	R << cos(a), 0, sin(a),
		0, 1, 0,
		-sin(a), 0, cos(a);
	return R;
}
Vector3d CKongTan8dianjiDlg::so3_log(const Matrix3d& R) {
	double cos_theta = (R.trace() - 1.0) / 2.0;
	cos_theta = (std::max)(-1.0, (std::min)(1.0, cos_theta));
	double theta = acos(cos_theta);
	if (std::abs(theta) < 1e-10) return Vector3d::Zero();
	double scale = theta / (2.0 * sin(theta));
	return scale * Vector3d(R(2, 1) - R(1, 2), R(0, 2) - R(2, 0), R(1, 0) - R(0, 1));
}
Matrix4d CKongTan8dianjiDlg::cc_T(double L, double kappa, double offset, Matrix3d(*Ry_func)(double)) {
	Matrix4d T = Matrix4d::Identity();
	if (std::abs(kappa) < 1e-10) { T(2, 3) = L; return T; }
	double R_arc = 1.0 / kappa;
	double theta_total = kappa * L;
	double x = R_arc * (1.0 - cos(theta_total)) + offset;
	double z = R_arc * sin(theta_total);
	Matrix3d R = Ry_func(theta_total);
	T.block<3, 3>(0, 0) = R;
	T(0, 3) = x;
	T(2, 3) = z;
	return T;
}
Matrix4d CKongTan8dianjiDlg::fk(const VectorXd& q, const RobotParams& params) {
	double d = q(0);
	double theta_base = q(1);
	double kappa3 = q(2);
	double kappa4 = q(3);
	double plane_angle = q(4);

	auto trans = [](double x, double y, double z)->Matrix4d {
		Matrix4d H = Matrix4d::Identity(); H(0, 3) = x; H(1, 3) = y; H(2, 3) = z; return H; };
	auto rotZ = [this](double a)->Matrix4d {
		Matrix4d H = Matrix4d::Identity(); H.block<3, 3>(0, 0) = Rz(a); return H; };
	auto rotY = [this](double a)->Matrix4d {
		Matrix4d H = Matrix4d::Identity(); H.block<3, 3>(0, 0) = Ry(a); return H; };

	Matrix4d T = Matrix4d::Identity();
	T = T * trans(0, 0, d);
	T = T * rotZ(theta_base);
	T = T * trans(0, 0, params.L1);
	T = T * rotY(params.phi);
	T = T * trans(0, 0, params.L2);
	T = T * cc_T(params.L3, kappa3, params.offset, Ry);
	T = T * trans(-params.offset, 0, 0);
	T = T * rotZ(plane_angle);
	T = T * cc_T(params.L4, kappa4, 0.0, Ry);
	return T;
}
VectorXd CKongTan8dianjiDlg::residual(const VectorXd& q, const Vector3d& p_target, const Matrix3d& R_target,
	double wp, double wr, const RobotParams& params) {
	Matrix4d T = fk(q, params);
	Vector3d p = T.block<3, 1>(0, 3);
	Matrix3d R = T.block<3, 3>(0, 0);
	Vector3d e_pos = wp * (p - p_target);
	Matrix3d R_delta = R_target.transpose() * R;
	Vector3d e_rot = wr * so3_log(R_delta);
	VectorXd res(6);
	res << e_pos, e_rot;
	return res;
}
CKongTan8dianjiDlg::LMResult CKongTan8dianjiDlg::lmsolve(const VectorXd& q0, const VectorXd& lb, const VectorXd& ub,
	std::function<VectorXd(const VectorXd&)> res_func, int max_iter, double ftol, double xtol) {
	int n = q0.size();
	VectorXd q = q0, q_prev = q0;
	double lambda = 0.001, nu = 2.0;
	VectorXd f = res_func(q);
	double F = f.squaredNorm();
	for (int iter = 0; iter < max_iter; ++iter) {
		MatrixXd J(f.size(), n);
		double eps = 1e-8;
		for (int j = 0; j < n; ++j) {
			VectorXd q_plus = q;
			q_plus(j) += eps;
			VectorXd f_plus = res_func(q_plus);
			J.col(j) = (f_plus - f) / eps;
		}
		MatrixXd JTJ = J.transpose() * J;
		VectorXd JTF = J.transpose() * f;
		for (int i = 0; i < n; ++i) JTJ(i, i) += lambda;
		VectorXd delta = JTJ.ldlt().solve(-JTF);
		VectorXd q_new = q + delta;
		for (int i = 0; i < n; ++i) q_new(i) = (std::max)(lb(i), (std::min)(ub(i), q_new(i)));
		VectorXd f_new = res_func(q_new);
		double F_new = f_new.squaredNorm();
		if (F_new < F) {
			q_prev = q; q = q_new; f = f_new; F = F_new;
			lambda *= 0.5; nu = 2.0;
			if (F < ftol || (q - q_prev).norm() < xtol) return { q, F, 1 };
		}
		else {
			lambda *= nu; nu *= 2.0;
			if (F < ftol) return { q, F, 1 };
		}
	}
	return { q, F, 0 };
}

void CKongTan8dianjiDlg::SolveAndExecuteIK(double target_x, double target_y, double target_z,
	double theta_x_deg, double theta_y_deg, double theta_z_deg,
	double wr)
{
	RobotParams params;
	Vector3d p_target(target_x, target_y, target_z);

	double tx = theta_x_deg * M_PI / 180.0;
	double ty = theta_y_deg * M_PI / 180.0;
	double tz = theta_z_deg * M_PI / 180.0;
	Matrix3d R_target = Rz(tz) * Ry(ty) * Rx(tx);

	VectorXd lb(5), ub(5);
	lb << 0.0, 0.0, -M_PI / 11.0, -M_PI / 15.91, 0.0;
	ub << 0.0, 0.0, M_PI / 11.0, M_PI / 15.91, 2.0 * M_PI;

	auto res_func = [&](const VectorXd& q) -> VectorXd {
		return residual(q, p_target, R_target, 1.0, wr, params);
		};

	std::vector<VectorXd> q0_list;
	q0_list.push_back((VectorXd(5) << 0, 0, 0.08, 0.08, M_PI / 6.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.05, 0.05, M_PI / 4.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.12, 0.12, M_PI / 3.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.15, 0.15, 0.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.07, 0.07, -M_PI / 6.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.09, 0.09, M_PI / 2.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, -0.08, -0.08, M_PI / 6.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, -0.05, -0.05, M_PI / 4.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, -0.12, -0.12, M_PI / 3.0).finished());

	VectorXd best_q;
	double best_err = 1e30;
	for (const auto& q0 : q0_list) {
		LMResult res = lmsolve(q0, lb, ub, res_func, 3000);
		Vector3d p_sol = fk(res.q, params).block<3, 1>(0, 3);
		double err = (p_sol - p_target).norm();
		if (err < best_err) {
			best_err = err;
			best_q = res.q;
		}
	}

	if (best_err > 1.0) {
		CString msg;
		msg.Format(_T("逆解失败：位置误差 %.2f mm"), best_err);
		MessageBox(msg, _T("逆运动学警告"), MB_ICONWARNING);
		return;
	}

	double k3 = best_q(2), k4 = best_q(3), plane = best_q(4);
	double bend3_deg = k3 * params.L3 * 180.0 / M_PI;
	double bend4_deg = k4 * params.L4 * 180.0 / M_PI;
	double plane_deg = plane * 180.0 / M_PI;

	CString info;
	info.Format(_T("逆解成功: θ3=%.1f°, θ4=%.1f°, φ=%.1f°\n目标位置:\n 位置(%.1f,%.1f,%.1f) mm\n 姿态(%.1f,%.1f,%.1f)°\n位置误差=%.3f mm"),
		bend3_deg, bend4_deg, plane_deg,
		target_x, target_y, target_z,
		theta_x_deg, theta_y_deg, theta_z_deg,
		best_err);
	MessageBox(info, _T("逆解结果"), MB_ICONINFORMATION);

	ExecuteBendingMotion(bend3_deg, bend4_deg, plane_deg);
}

void CKongTan8dianjiDlg::OnBnClickedBtnIkSolve()
{
	UpdateData(TRUE);   // 获取界面上的 m_ikX, m_ikY, m_ikZ, m_theta_x, m_theta_y, m_theta_z

	double d1, d2, d3, d4, d5, d6;
	// 根据复选框决定是否启用姿态约束
	double wr = m_usePose ? 0.01 : 0.0;
	if (ComputeDeltasFromPosition(m_ikX, m_ikY, m_ikZ,
		m_theta_x, m_theta_y, m_theta_z,
		wr,
		d1, d2, d3, d4, d5, d6))
	{
		m_delta1 = d1; m_delta2 = d2; m_delta3 = d3;
		m_delta4 = d4; m_delta5 = d5; m_delta6 = d6;
		UpdateData(FALSE);   // 刷新结果显示到界面

		CString msg;
		msg.Format(_T("推拉量计算结果 (mm):\nΔ1=%.2f  Δ2=%.2f  Δ3=%.2f\nΔ4=%.2f  Δ5=%.2f  Δ6=%.2f\n\n是否驱动电机？"),
			d1, d2, d3, d4, d5, d6);
		if (MessageBox(msg, _T("确认执行"), MB_YESNO) == IDYES)
		{
			// 根据最优关节角度执行弯曲运动（不需要再次做逆解，直接用上面计算的结果）
			// 简便方法：调用 SolveAndExecuteIK 以重用内部逻辑
			SolveAndExecuteIK(m_ikX, m_ikY, m_ikZ, m_theta_x, m_theta_y, m_theta_z, wr);
		}
	}
	else
	{
		MessageBox(_T("逆运动学求解失败，请检查目标点是否可达。"), _T("错误"), MB_ICONERROR);
	}
}

// ========== 比例联动按钮 ==========
void CKongTan8dianjiDlg::OnBnClickedBtnProportionalHook()
{
	UpdateData(TRUE);

	// 检查电机5和电机6是否在MotorIds中
	if (MotorIds.find(5) == MotorIds.end() || MotorIds.find(6) == MotorIds.end())
	{
		TRACE(_T("Motor 5 or 6 not available!\n"));
		MessageBox(_T("电机5或6不可用！"), _T("错误"), MB_ICONERROR);
		return;
	}

	// 使用当前S56步长值进行比例联动
	// 电机5正向，电机6反向（比例 1:3）
	if (m_motorCtrl.SetMotorVelocity(5, motor_V56, 200000, 200000))
	{
		MotorMoveRel(5, motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(6, 3 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(6, -3 * motor_S56);
	}

	// 联动电机1-4
	if (m_motorCtrl.SetMotorVelocity(1, 1.5 / 1.1 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(1, 1.5 / 1.1 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(2, 4.25 / 1.5 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(2, -4.25 / 1.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(3, 2.75 / 1.5 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(3, -2.75 / 1.5 * motor_S56);
	}
	if (m_motorCtrl.SetMotorVelocity(4, 0.34 / 1.5 * motor_V56, 200000, 200000))
	{
		MotorMoveRel(4, -0.34 / 1.5 * motor_S56);
	}

	TRACE(_T("ProportionalHook: M5 +%.0f, M6 -%.0f, with M1-M4 coordinated\n"), motor_S56, 3 * motor_S56);
}

// ========== 预设姿态按钮: 设置 Rx=0°, Ry=250°, Rz=0° ==========
void CKongTan8dianjiDlg::OnBnClickedBtnSetPreset()
{
	m_theta_x = 0.0;
	m_theta_y = 250.0;
	m_theta_z = 0.0;
	UpdateData(FALSE);   // 刷新界面显示

	TRACE(_T("Preset pose set: Rx=%.1f, Ry=%.1f, Rz=%.1f\n"), m_theta_x, m_theta_y, m_theta_z);
}

void CKongTan8dianjiDlg::ExecuteLinearTrajectory(
	double start_x, double start_y, double start_z,
	double end_x, double end_y, double end_z,
	int num_steps,
	double theta_x_deg, double theta_y_deg, double theta_z_deg,
	double wr)
{
	if (num_steps < 2) {
		MessageBox(_T("直线轨迹步数至少为 2。"), _T("轨迹错误"), MB_ICONERROR);
		return;
	}

	const double dx = end_x - start_x;
	const double dy = end_y - start_y;
	const double dz = end_z - start_z;
	const double length = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (length < 1e-6) {
		MessageBox(_T("起始点和终点不能相同，请输入有效直线轨迹。"), _T("轨迹错误"), MB_ICONERROR);
		return;
	}

	// Step 1: compute IK for each trajectory point, store absolute motor deltas
	std::vector<double> delta_abs[6];
	for (int k = 0; k < 6; k++) delta_abs[k].resize(num_steps, 0.0);
	double last_pose_x = theta_x_deg;
	double last_pose_y = theta_y_deg;
	double last_pose_z = theta_z_deg;

	for (int i = 0; i < num_steps; i++)
	{
		double t = (double)i / (num_steps - 1);
		double px = start_x + t * (end_x - start_x);
		double py = start_y + t * (end_y - start_y);
		double pz = start_z + t * (end_z - start_z);
		double next_t = (i < num_steps - 1) ? (double)(i + 1) / (num_steps - 1) : (double)(i - 1) / (num_steps - 1);
		double next_x = start_x + next_t * (end_x - start_x);
		double next_y = start_y + next_t * (end_y - start_y);
		double next_z = start_z + next_t * (end_z - start_z);
		if (i == num_steps - 1) {
			ComputePoseTowardNextPoint(next_x, next_y, next_z, px, py, pz,
				last_pose_x, last_pose_y, last_pose_z);
		}
		else {
			ComputePoseTowardNextPoint(px, py, pz, next_x, next_y, next_z,
				last_pose_x, last_pose_y, last_pose_z);
		}

		double d1, d2, d3, d4, d5, d6;
		if (!ComputeDeltasFromPosition(px, py, pz, last_pose_x, last_pose_y, last_pose_z, wr, d1, d2, d3, d4, d5, d6))
		{
			CString msg;
			msg.Format(_T("轨迹点 %d (%.1f, %.1f, %.1f) 姿态(%.1f, %.1f, %.1f) 逆解失败"),
				i, px, py, pz, last_pose_x, last_pose_y, last_pose_z);
			MessageBox(msg, _T("轨迹错误"), MB_ICONERROR);
			return;
		}
		delta_abs[0][i] = d1; delta_abs[1][i] = d2; delta_abs[2][i] = d3;
		delta_abs[3][i] = d4; delta_abs[4][i] = d5; delta_abs[5][i] = d6;
	}

	// Step 2: execute incrementally — first point moves from zero, rest are incremental
	int motorIds[] = { 1, 2, 3, 4, 5, 6 };

	for (int i = 0; i < num_steps; i++)
	{
		double move[6];
		if (i == 0) {
			for (int k = 0; k < 6; k++) move[k] = delta_abs[k][0];
		}
		else {
			for (int k = 0; k < 6; k++) move[k] = delta_abs[k][i] - delta_abs[k][i - 1];
		}

		// find max distance for speed scaling
		double maxDist = 0.0;
		for (int k = 0; k < 6; k++) {
			if (std::abs(move[k]) > maxDist) maxDist = std::abs(move[k]);
		}
		if (maxDist < 0.5) continue;

		// set speeds proportional to distance
		for (int k = 0; k < 6; k++) {
			int id = motorIds[k];
			if (MotorIds.find(id) == MotorIds.end()) continue;
			double dist = std::abs(move[k]);
			if (dist < 0.5) continue;
			double speed = GetStartupSpeed(id) * m_resetSpeedRatio * (dist / maxDist);
			if (speed < 2000.0) speed = 2000.0;
			m_motorCtrl.SetMotorVelocity(id, speed, 200000, 200000);
		}

		// send relative move commands
		for (int k = 0; k < 6; k++) {
			int id = motorIds[k];
			if (MotorIds.find(id) == MotorIds.end()) continue;
			if (std::abs(move[k]) < 0.5) continue;
			MotorMoveRel(id, move[k]);
		}

		// wait for completion
		for (int k = 0; k < 6; k++) {
			int id = motorIds[k];
			if (MotorIds.find(id) == MotorIds.end()) continue;
			double dist = std::abs(move[k]);
			if (dist < 0.5) continue;
			double speed = GetStartupSpeed(id) * m_resetSpeedRatio * (dist / maxDist);
			if (speed < 2000.0) speed = 2000.0;
			double timeoutMs = (std::max)(dist / (speed + 1.0) * 2.0 * 1000.0, 5000.0);
			if (timeoutMs > 60000.0) timeoutMs = 60000.0;
			m_motorCtrl.WaitMoveDone(id, (float)timeoutMs);
		}
	}

	TRACE(_T("线性轨迹执行完成: (%.1f,%.1f,%.1f) -> (%.1f,%.1f,%.1f), %d steps\n"),
		start_x, start_y, start_z, end_x, end_y, end_z, num_steps);
	MessageBox(_T("连续直线轨迹执行完成！"), _T("提示"), MB_ICONINFORMATION);
}

void CKongTan8dianjiDlg::OnBnClickedBtnLinearTrajectory()
{
	UpdateData(TRUE);  // 从界面读取 IK 起点 (m_ikX/m_ikY/m_ikZ)、终点 (m_trajEndX/Y/Z)、步数、姿态角

	if (m_trajSteps < 2) {
		MessageBox(_T("直线轨迹步数至少为 2。"), _T("轨迹错误"), MB_ICONERROR);
		return;
	}

	double traj_dx = m_trajEndX - m_ikX;
	double traj_dy = m_trajEndY - m_ikY;
	double traj_dz = m_trajEndZ - m_ikZ;
	double traj_length = std::sqrt(traj_dx * traj_dx + traj_dy * traj_dy + traj_dz * traj_dz);
	if (traj_length < 1e-6) {
		MessageBox(_T("起始点和终点不能相同，请输入有效直线轨迹。"), _T("轨迹错误"), MB_ICONERROR);
		return;
	}

	double first_pose_x = m_theta_x;
	double first_pose_y = m_theta_y;
	double first_pose_z = m_theta_z;
	if (ComputePoseTowardNextPoint(m_ikX, m_ikY, m_ikZ, m_trajEndX, m_trajEndY, m_trajEndZ,
		first_pose_x, first_pose_y, first_pose_z))
	{
		m_theta_x = first_pose_x;
		m_theta_y = first_pose_y;
		m_theta_z = first_pose_z;
		UpdateData(FALSE);
	}

	// 先复位到零位
	std::vector<int> allIDs;
	for (const CML::uint& id : MotorIds)
	{
		if (m_motorZeroPos.find(id) != m_motorZeroPos.end())
			allIDs.push_back((int)id);
	}
	SyncedTrapezoidalReset(allIDs);

	double wr = m_usePose ? 0.01 : 0.0;

	ExecuteLinearTrajectory(
		m_ikX, m_ikY, m_ikZ,            // start (复用 IK 位置输入)
		m_trajEndX, m_trajEndY, m_trajEndZ, // end
		m_trajSteps,                      // steps
		m_theta_x, m_theta_y, m_theta_z,  // theta_x, theta_y, theta_z
		wr);                              // wr
}

bool CKongTan8dianjiDlg::ComputePoseTowardNextPoint(double current_x, double current_y, double current_z,
	double next_x, double next_y, double next_z,
	double& theta_x_deg, double& theta_y_deg, double& theta_z_deg)
{
	double dx = next_x - current_x;
	double dy = next_y - current_y;
	double dz = next_z - current_z;
	double length = std::sqrt(dx * dx + dy * dy + dz * dz);
	if (length < 1e-6) {
		return false;
	}

	dx /= length;
	dy /= length;
	dz /= length;

	double yaw = std::atan2(dy, dx);
	double pitch = std::atan2(std::sqrt(dx * dx + dy * dy), dz);

	theta_x_deg = 0.0;
	theta_y_deg = pitch * 180.0 / M_PI;
	theta_z_deg = yaw * 180.0 / M_PI;
	return true;
}

void CKongTan8dianjiDlg::OnEnChangeEdit9()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
bool CKongTan8dianjiDlg::ComputeDeltasFromPosition(double target_x, double target_y, double target_z,
	double theta_x_deg, double theta_y_deg, double theta_z_deg,
	double wr,
	double& delta1, double& delta2, double& delta3,
	double& delta4, double& delta5, double& delta6)
{
	RobotParams params;
	Vector3d p_target(target_x, target_y, target_z);

	// 计算目标旋转矩阵（与 zuobioadianweizi.cpp 顺序一致：Rz * Ry * Rx）
	double tx = theta_x_deg * M_PI / 180.0;
	double ty = theta_y_deg * M_PI / 180.0;
	double tz = theta_z_deg * M_PI / 180.0;
	Matrix3d R_target = Rz(tz) * Ry(ty) * Rx(tx);   // Rx 为绕 X 轴旋转，固定角

	// 关节边界
	VectorXd lb(5), ub(5);
	lb << 0.0, 0.0, -M_PI / 11.0, -M_PI / 15.91, 0.0;
	ub << 0.0, 0.0, M_PI / 11.0, M_PI / 15.91, 2.0 * M_PI;

	std::vector<VectorXd> q0_list;
	q0_list.push_back((VectorXd(5) << 0, 0, 0.08, 0.08, M_PI / 6.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.05, 0.05, M_PI / 4.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.12, 0.12, M_PI / 3.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.15, 0.15, 0.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.07, 0.07, -M_PI / 6.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, 0.09, 0.09, M_PI / 2.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, -0.08, -0.08, M_PI / 6.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, -0.05, -0.05, M_PI / 4.0).finished());
	q0_list.push_back((VectorXd(5) << 0, 0, -0.12, -0.12, M_PI / 3.0).finished());

	// 残差函数：使用位置权重 1.0，姿态权重 wr
	auto res_func = [&](const VectorXd& q) -> VectorXd {
		return residual(q, p_target, R_target, 1.0, wr, params);
		};

	VectorXd best_q;
	double best_err = 1e30;
	for (const auto& q0 : q0_list) {
		LMResult res = lmsolve(q0, lb, ub, res_func, 3000);
		Vector3d p_sol = fk(res.q, params).block<3, 1>(0, 3);
		double err = (p_sol - p_target).norm();
		if (err < best_err) {
			best_err = err;
			best_q = res.q;
		}
	}

	if (best_err > 1.0) {
		CString msg;
		msg.Format(_T("逆解失败：位置误差 %.2f mm"), best_err);
		MessageBox(msg, _T("逆运动学警告"), MB_ICONWARNING);
		return false;
	}

	double k3 = best_q(2), k4 = best_q(3), plane = best_q(4);
	ComputeMotorDeltas(k3, k4, plane, delta1, delta2, delta3, delta4, delta5, delta6);
	return true;
}
