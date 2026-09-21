#pragma once
#include"CML.h"
#include<set>
//#include"can_ixxat.h"
#include"can_ixxat_v3.h"
//#include"can_copley.h"

#define  MaxMotorNum 12 //电机数量
#define  LinkNum 4

// 请在自己的机器人文件里定义MotorIds的对象,指定要使用的电机ID号
// 例如: const std::set<uint> MotorIds = {1, 2, 3};
extern std::set<CML::uint> MotorIds;//必须要写否则会报错

//删除无需在基类中体现的函数，将几个不必定义为抽象函数的函数写了简单实例，增加了一个记录错误输出函数，周圆，20180410
struct MotorPara
{
	int motorID;
	double velocity;
	double accelerate;
	double decelerate;
	double posLimit;
	double negLimit;
	double currentPos;
	double currentActual;
	double countPerRad;
	bool bEnable;
	bool bUpate;
};

//派生类实现基类纯虚函数
class CmlMotor//: public CBaseMotorCtrl
{
public:
	CmlMotor(void);
	~CmlMotor();
	bool OpenCanCard();
	bool OpenMotorBus();
	bool EnableMotorBus();
	bool CloseMotorBus();
	bool GetMotorPara(int motorID, MotorPara& motorPara);
	//电机相对运动，以脉冲个数
	bool MotorMoveRel(int motorID, double movects);
	//电机绝对位置运动，以脉冲个数
	bool MotorMoveAbs(int motorID, double movects);
	bool SetMotorVelocity(int motorIndex, double velocity, double Accelerate, double Decelerate);
	// 设置指定电机的软限位  posLimit 为正数  negLimit 为负数
	bool SetSoftLimit(int motorIndex, double posLimit, double negLimit);
	bool EnableMotor(int motorIndex, bool bEnable = true);
	bool SetMotorPara(int motorID, MotorPara& motorPara);
	bool SetHome(int motorID);
	bool SetHome(int motorID, CML::COPLEY_HOME_METHOD method, double velFast, double velSlow, double accel, double offset);
	bool SetPositionLoad(int motorID, double value);
	//电机相对运动，以脉冲个数
	bool MotorMoveRel(int motorIndex, double movects, double velocity);
	//电机绝对位置运动，以脉冲个数
	bool MotorMoveAbs(int motorIndex, double movects, double velocity);
	bool Stop(int motorID);
	bool ResetAmp(int motorID);
	CML::int16 GetRefVoltage(int motorID);
	bool GetRefVoltage(int motorID, CML::int16& value);
	CML::int16 GetCurrentActual(int motorID);
	bool GetCurrentActual(int motorID, CML::int16& value);
	CML::uint16 GetDigitalInputs(int motorID);
	bool GetDigitalInputs(int motorID, CML::uint16& value);
	bool SetInputConfig(int motorID, CML::int8 pin, CML::INPUT_PIN_CONFIG cfg);
	CML::uint16 GetDigitalOutputs(int motorID);
	bool GetDigitalOutputs(int motorID, CML::uint16& value);
	bool SetDigitalOutputs(int motorID, CML::uint16 value);
	CML::uunit GetPositionLoad(int motorID);
	bool GetPositionLoad(int motorID, double& value);
	CML::uunit GetPositionActual(int motorID);
	bool GetPositionActual(int motorID, double& value);
	CML::uunit GetVelocityLoad(int motorID);
	bool GetVelocityLoad(int motorID, double& value);
	CML::uunit GetVelocityActual(int motorID);
	bool GetVelocityActual(int motorID, double& value);
	bool IsHardwareEnabled(int motorID);
	bool IsSoftwareEnabled(int motorID);
	bool IsReferenced(int motorID);
	bool IsInitialized(int motorID);
	bool ClearFaults(int motorID);
	bool WaitMoveDone(int motorID, float value);
	bool WaitHomeDone(int motorID, float value);
	bool SetAmpMode(int motorID, CML::AMP_MODE mode);
	bool GetAmpMode(int motorID, CML::AMP_MODE AmpMode);
	CML::uint32 GetFaults(int motorID);
	bool GetFaults(int motorID, CML::AMP_FAULT& value);
	CML::uint16 GetStatusWord(int motorID);
	bool GetStatusWord(int motorID, CML::uint16& value);
	CML::uint32 GetEvent(int motorID);
	bool GetEvent(int motorID, CML::AMP_EVENT& e);
	bool WaitEvent(int motorID, CML::AMP_EVENT event, float time);
	bool SendData(CML::uint32 canID, CML::uint8 data[]);
	bool LinkMove(int motorIndex, double cts[]);
	bool LinkInit();
	bool LinkStop();
	CML::uunit GetPositionCommand(int motorID);
	bool GetPositionCommand(int motorID, double& value);
	bool SetOutputsConfig(int motorID, CML::int8 pin, CML::OUTPUT_PIN_CONFIG cfg, CML::uint32 mask1);
	bool SetPositionActual(int motorID, double value);
	bool GetHomeAdjustment(int motorID, double& value);

protected:
	//标记电机控制总线是否已经开启
	std::set<CML::uint> m_initialized;
	bool m_cleanupFailed = false;
	bool m_bOpen;
	bool m_bcanOpen;
	bool m_bcanOpenOpen;

	//注意，此处为了与CAN总线的编号对应一致，所有电机起始编号从1开始，0无效
	char m_lastError[256];

private:

	/*CML::CopleyCAN can;*/    // CAN port
	//CML::IxxatCAN can;
	CML::IxxatCANV3 can;
	CML::CanOpen canOpen;  // Create the upper level CANopen object
	CML::Amp ampObj[MaxMotorNum];                 //为节点对象  （AmpObj或IOObj）声明一个或多个变量并创建这些变量的实例。
	CML::AmpSettings AmpSettings[MaxMotorNum];
	const CML::Error* err;
	CML::Linkage link;
};
