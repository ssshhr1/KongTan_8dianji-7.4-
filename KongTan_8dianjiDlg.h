#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "afxwin.h"
#include "include/CML_lib-C++/CMLMotorCtrl.h"
#include <map>
#include <set>
#include <functional>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Geometry>

struct RobotParams {
	double L1 = 100.0;
	double L2 = 32.61;
	double L3 = 11.0;
	double L4 = 15.91;
	double offset = 1.5;
	double phi = M_PI / 2.0;
};

class CKongTan8dianjiDlg : public CDialogEx
{
public:
	double m_theta_x, m_theta_y, m_theta_z;   // 目标姿态角(Rx,Ry,Rz)，通过 DDX 与界面控件关联
	CKongTan8dianjiDlg(CWnd* pParent = nullptr);

	enum { IDD = IDD_KONGTAN_8DIANJI_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;
	CmlMotor m_motorCtrl;
	std::map<int, double> m_motorZeroPos;
	struct SoftLimitConfig { double posLimit; double negLimit; };
	std::map<int, SoftLimitConfig> m_motorSoftLimits;
	void InitMotorSoftLimits();
	static Eigen::Matrix3d Rx(double a);
	static Eigen::Matrix3d Rz(double a);
	static Eigen::Matrix3d Ry(double a);
	static Eigen::Vector3d so3_log(const Eigen::Matrix3d& R);
	static Eigen::Matrix4d cc_T(double L, double kappa, double offset, Eigen::Matrix3d(*Ry_func)(double));
	Eigen::Matrix4d fk(const Eigen::VectorXd& q, const RobotParams& params);
	Eigen::VectorXd residual(const Eigen::VectorXd& q, const Eigen::Vector3d& p_target, const Eigen::Matrix3d& R_target,
		double wp, double wr, const RobotParams& params);
	struct LMResult { Eigen::VectorXd q; double resnorm; int exitflag; };
	LMResult lmsolve(const Eigen::VectorXd& q0, const Eigen::VectorXd& lb, const Eigen::VectorXd& ub,
		std::function<Eigen::VectorXd(const Eigen::VectorXd&)> res_func,
		int max_iter = 1000, double ftol = 1e-14, double xtol = 1e-14);

	void ComputeMotorDeltas(double kappa3, double kappa4, double plane_angle,
		double& delta1, double& delta2, double& delta3,
		double& delta4, double& delta5, double& delta6);
	void ExecuteBendingMotion(double bend_angle3_deg, double bend_angle4_deg, double plane_angle_deg);
	void SolveAndExecuteIK(double target_x, double target_y, double target_z,
		double theta_x_deg, double theta_y_deg, double theta_z_deg,
		double wr);
	double m_bendAngle3;
	double m_bendAngle4;
	double m_planeAngle;
	double m_ikX, m_ikY, m_ikZ;
	double m_delta1, m_delta2, m_delta3, m_delta4, m_delta5, m_delta6;
	BOOL m_usePose;

	double motor_V12, motor_V34, motor_V56, motor_V78;
	double motor_S12, motor_S34, motor_S56, motor_S78;
	double m_resetSpeedRatio;   // 复位速度比例系数: 复位速度 = 启动速度 × 此系数 (默认4.0)
	int m_accelSteps;           // 梯形加减速步数 (默认5)
	double m_loadThreshold;     // 负载阈值，0=禁用 (默认0)
	int m_direction;            // 电机方向系数: 1=正向, -1=全部反转

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButton1();
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
	afx_msg void OnBnClickedButton8();
	afx_msg void OnBnClickedButton9();
	afx_msg void OnBnClickedButton10();
	afx_msg void OnBnClickedButton11();
	afx_msg void OnBnClickedButton12();
	afx_msg void OnBnClickedButton13();
	afx_msg void OnBnClickedCcancel();
	afx_msg void OnBnClickedBtnExecuteBending();
	afx_msg void OnBnClickedBtnIkSolve();
	afx_msg void OnEnChangeEdit9();
	afx_msg void OnBnClickedBtnProportionalHook();
	afx_msg void OnBnClickedBtnSetPreset();
	afx_msg void OnBnClickedBtnLinearTrajectory();
	void ExecuteLinearTrajectory(double start_x, double start_y, double start_z,
		double end_x, double end_y, double end_z, int num_steps,
		double theta_x_deg, double theta_y_deg, double theta_z_deg, double wr);
	bool ComputePoseTowardNextPoint(double current_x, double current_y, double current_z,
		double next_x, double next_y, double next_z,
		double& theta_x_deg, double& theta_y_deg, double& theta_z_deg);
	bool ComputeDeltasFromPosition(double target_x, double target_y, double target_z,
		double theta_x_deg, double theta_y_deg, double theta_z_deg,
		double wr,
		double& delta1, double& delta2, double& delta3,
		double& delta4, double& delta5, double& delta6);
	double m_trajEndX, m_trajEndY, m_trajEndZ;
	int m_trajSteps;
	// 辅助方法：复位速度比例调节 + 梯形加减速（同时启停、同时到达）
	double GetStartupSpeed(int motorID);
	void SyncedTrapezoidalReset(const std::vector<int>& motorIDs);
	bool MotorMoveRel(int motorID, double movects) { return m_motorCtrl.MotorMoveRel(motorID, m_direction * movects); }
};
