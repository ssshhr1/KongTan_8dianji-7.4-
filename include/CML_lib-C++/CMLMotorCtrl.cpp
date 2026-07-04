#include "pch.h"
#include "CMLMotorCtrl.h"

CML_NAMESPACE_USE();

CmlMotor::CmlMotor(void)
{
	m_bOpen = false;//初始化驱动器
	m_bcanOpen = false;//* Initialize the CAN card and network
	m_bcanOpenOpen = false;//连接can卡
}

CmlMotor::~CmlMotor()
{
	if (m_bOpen)
	{
		m_bOpen = FALSE;
		for (const uint& id : MotorIds)
		{
			int motorIndex = id - 1;
			if (motorIndex < 0 || motorIndex >= MaxMotorNum) {
				continue;
			}
			ampObj[motorIndex].Disable();
			err = ampObj[motorIndex].UnInit();
			if (err)
			{
				TRACE("amp[%d].UnInit:%s\n", motorIndex + 1, err->toString());
			}
		}
		canOpen.Close();

		err = can.Close();
		if (err)
		{
			TRACE("can.Close:%s\n", err->toString());
		}

	}

}

bool CmlMotor::OpenCanCard()
{
	if (m_bcanOpenOpen)
	{
		return true;
	}
	//* Initialize the CAN card and network
	if (m_bcanOpen == 0)
	{
		err = can.Open();
		if (err)
		{
			TRACE("can:%s\n", err->toString());
			return false;
		}
		else
		{
			m_bcanOpen = 1;
		}
	}

	if (m_bcanOpenOpen == 0)
	{
		err = canOpen.Open(can);//连接can卡
		if (err)
		{
			TRACE("canOpen:%s\n", err->toString());
			return false;
		}
		else
		{
			m_bcanOpenOpen = 1;
		}
	}

	return true;
}

bool CmlMotor::OpenMotorBus()
{
	if (m_bOpen)
	{
		return true;
	}
	//* Initialize the CAN card and network
	if (m_bcanOpen == 0)
	{
		err = can.Open();
		if (err)
		{
			TRACE("can:%s\n", err->toString());
			return false;
		}
		else
		{
			m_bcanOpen = 1;
		}
	}

	if (m_bcanOpenOpen == 0)
	{
		err = canOpen.Open(can);//连接can卡
		if (err)
		{
			TRACE("canOpen:%s\n", err->toString());
			return false;
		}
		else
		{
			m_bcanOpenOpen = 1;
		}
	}


	try
	{
		//初始化驱动器
		for (const uint& id : MotorIds)
		{
			int motorIndex = id - 1;
			assert(motorIndex >= 0 && motorIndex < MaxMotorNum);
			AmpSettings[motorIndex].enableOnInit = FALSE;                                  //驱动器初始化参数设置
			err = ampObj[motorIndex].Init(canOpen, motorIndex + 1, AmpSettings[motorIndex]);         //初始化驱动器
			//err = ampObj[motorID].Init(canOpen, motorID);         //以默认参数初始化驱动器
			if (err)
			{
				TRACE("amp[%d].Init:%s\n", motorIndex + 1, err->toString());
				return false;
			}
		}
	}
	catch (...)
	{
		return false;
	}

	m_bOpen = true;

	return true;
}

bool CmlMotor::CloseMotorBus()
{
	if (m_bOpen)
	{
		m_bOpen = FALSE;
		for (const uint& id : MotorIds)
		{
			int motorIndex = id - 1;
			if (motorIndex < 0 || motorIndex >= MaxMotorNum) {
				continue;
			}
			ampObj[motorIndex].Disable();
			err = ampObj[motorIndex].UnInit();
			if (err)
			{
				TRACE("amp[%d].UnInit:%s\n", motorIndex + 1, err->toString());
				return FALSE;
			}
		}
		canOpen.Close();

		err = can.Close();
		if (err)
		{
			TRACE("can.Close:%s\n", err->toString());
			return FALSE;
		}
	}
	return TRUE;

}

bool CmlMotor::EnableMotorBus()
{
	if (m_bOpen)
	{
		try
		{
			//初始化驱动器
			for (const uint& id : MotorIds)
			{
				int motorIndex = id - 1;
				if (motorIndex < 0 || motorIndex >= MaxMotorNum) {
					continue;
				}
				err = ampObj[motorIndex].Enable(TRUE);/*@param wait If true, the function won't return until a status message
				from the amp is received indicating that it successfully enabled.*/
				if (err)
				{
					TRACE("EnableMotorBus[%d]:%s\n", motorIndex + 1, err->toString());
					return false;
				}
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::MotorMoveRel(int motorID, double movects)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].MoveRel(movects);
			if (err)
			{
				TRACE("MotorMoveRel[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::MotorMoveAbs(int motorID, double movects)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].MoveAbs(movects);
			if (err)
			{
				TRACE("MotorMoveAbs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::MotorMoveRel(int motorID, double movects, double velocity)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetProfileVel(velocity);
			if (err)
			{
				TRACE("SetProfileVel[%d]:%s\n", motorID, err->toString());
				return false;
			}
			err = ampObj[motorID - 1].MoveRel(movects);
			if (err)
			{
				TRACE("MotorMoveRel[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::MotorMoveAbs(int motorID, double movects, double velocity)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetProfileVel(velocity);
			if (err)
			{
				TRACE("SetProfileVel[%d]:%s\n", motorID, err->toString());
				return false;
			}
			err = ampObj[motorID - 1].MoveAbs(movects);
			if (err)
			{
				TRACE("MotorMoveAbs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::SetMotorVelocity(int motorID, double velocity, double Accelerate, double Decelerate)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			ProfileConfig ProfileParameters;
			ProfileParameters.acc = Accelerate;//cts/s2
			ProfileParameters.dec = Decelerate;//cts/s2
			ProfileParameters.vel = velocity;//cts/s
			err = ampObj[motorID - 1].SetProfileConfig(ProfileParameters);
			err = ampObj[motorID - 1].SetCountsPerUnit(1);
			if (err)
			{
				TRACE("SetMotorVelocity[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SetSoftLimit(int motorID, double posLimit, double negLimit)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			SoftPosLimit softlimit;
			softlimit.neg = negLimit;
			softlimit.pos = posLimit;
			err = ampObj[motorID - 1].SetSoftLimits(softlimit);
			if (err)
			{
				TRACE("SetSoftLimit[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::EnableMotor(int motorID, bool bEnable)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			if (bEnable)
			{
				err = ampObj[motorID - 1].Enable();
				if (err)
				{
					TRACE("EnableMotor[%d]:%s\n", motorID, err->toString());
					return false;
				}
			}
			else
			{
				err = ampObj[motorID - 1].Disable();
				if (err)
				{
					TRACE("DisableMotor[%d]:%s\n", motorID, err->toString());
					return false;
				}
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::GetMotorPara(int motorID, MotorPara& motorPara)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			int motorIndex = motorID - 1;
			MotorPara m_motorPara;

			SoftPosLimit softlimit;
			ampObj[motorIndex].GetSoftLimits(softlimit);
			m_motorPara.posLimit = softlimit.pos;
			m_motorPara.negLimit = softlimit.neg;

			int16 CurrentActual;
			uunit PositionActual;
			ampObj[motorIndex].GetPositionActual(PositionActual);
			ampObj[motorIndex].GetCurrentActual(CurrentActual);
			m_motorPara.currentPos = double(PositionActual);
			m_motorPara.currentActual = double(CurrentActual);

			ProfileConfig ProfileParameters;
			ampObj[motorIndex].GetProfileConfig(ProfileParameters);
			m_motorPara.accelerate = ProfileParameters.acc;
			m_motorPara.decelerate = ProfileParameters.dec;
			m_motorPara.velocity = ProfileParameters.vel;
			m_motorPara.bEnable = ampObj[motorIndex].IsSoftwareEnabled();

			motorPara = m_motorPara;
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::SetMotorPara(int motorID, MotorPara& motorPara)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			int motorIndex = motorID - 1;
			ProfileConfig ProfileParameters;
			ProfileParameters.type = PROFILE_TRAP;//Trapezoidal profile.梯形曲线。位置，速度，加速度  Selects profile type
			ProfileParameters.acc = motorPara.accelerate;
			ProfileParameters.dec = motorPara.decelerate;
			ProfileParameters.vel = motorPara.velocity;
			ampObj[motorIndex].SetProfileConfig(ProfileParameters);

			//ampObj[motorIndex].SoftPositionPosLimit = motorPara.posLimit;
			//ampObj[motorIndex].SoftPositionNegLimit = motorPara.negLimit;

			return true;

		}
		catch (...)
		{
			return false;
		}
	}
	else return false;

}

bool CmlMotor::SetHome(int motorID)  // 电机找零（设置电机软限位不影响电机找零）
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{

			HomeConfig hcfg;
			//hcfg.method = CHM_NONE; //Set the current position to home.
			hcfg.method = CHM_HARDSTOP_POS;
			hcfg.offset = -100000;
			hcfg.velFast = 40960;//offset 速度cts/s
			hcfg.velSlow = 20480;//找零 速度cts/s
			hcfg.accel = 204800 * 60;//cts/s2
			hcfg.current = 5;//*10mA
			hcfg.delay = 3;//ms
			err = ampObj[motorID - 1].GoHome(hcfg);
			if (err)
			{
				TRACE("SetHome[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::SetHome(int motorID, COPLEY_HOME_METHOD method, double velFast, double velSlow, double accel, double offset)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{

			HomeConfig hcfg;
			hcfg.method = method;
			hcfg.offset = offset;
			hcfg.velFast = velFast;//offset 速度cts/s
			hcfg.velSlow = velSlow;//找零 速度cts/s
			hcfg.accel = accel;//cts/s2
			err = ampObj[motorID - 1].GoHome(hcfg);
			if (err)
			{
				TRACE("SetHome[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool  CmlMotor::Stop(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].HaltMove();
			if (err)
			{
				TRACE("Stop[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool  CmlMotor::ResetAmp(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].Reset();
			if (err)
			{
				TRACE("ResetAmp[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SetDigitalOutputs(int motorID, uint16 value)
{

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetOutputs(value);
			if (err)
			{
				TRACE("SetDigitalOutputs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SetInputConfig(int motorID, int8 pin, INPUT_PIN_CONFIG cfg)
{

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetInputConfig(pin, cfg, 0);
			if (err)
			{
				TRACE("SetInputConfig[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SetPositionLoad(int motorID, double value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetPositionLoad(value);
			if (err)
			{
				TRACE("SetPositionLoad[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool  CmlMotor::IsHardwareEnabled(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		return ampObj[motorID - 1].IsHardwareEnabled();
	}
	else return false;
}

bool  CmlMotor::IsSoftwareEnabled(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		return ampObj[motorID - 1].IsSoftwareEnabled();
	}
	else return false;
}

bool  CmlMotor::IsReferenced(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		return ampObj[motorID - 1].IsReferenced();
	}
	else return false;
}

bool  CmlMotor::IsInitialized(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		return ampObj[motorID - 1].IsInitialized();
	}
	else return false;
}

bool  CmlMotor::ClearFaults(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].ClearFaults();
			if (err)
			{
				TRACE("ClearFaults[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool  CmlMotor::WaitMoveDone(int motorID, float value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].WaitMoveDone(value);
			if (err)
			{
				TRACE("WaitMoveDone[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool  CmlMotor::WaitHomeDone(int motorID, float value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].WaitHomeDone(value);
			if (err)
			{
				TRACE("WaitHomeDone[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SetAmpMode(int motorID, AMP_MODE mode)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetAmpMode(mode);
			if (err)
			{
				TRACE("SetAmpMode[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::GetAmpMode(int motorID, AMP_MODE AmpMode)
{

	if (!m_bOpen)
	{

	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		AMP_MODE mode;
		try
		{
			err = ampObj[motorID - 1].GetAmpMode(mode);
			if (err)
			{
				TRACE("GetAmpMode[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		AmpMode = mode;
		return true;
	}
	else return false;
}

uint32 CmlMotor::GetFaults(int motorID)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		AMP_FAULT value;
		try
		{
			err = ampObj[motorID - 1].GetFaults(value);
			if (err)
			{
				TRACE("GetFaults[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return value;
	}
	else return false;
}

bool CmlMotor::GetFaults(int motorID, AMP_FAULT& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetFaults(value);
			if (err)
			{
				TRACE("GetFaults[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uunit CmlMotor::GetVelocityActual(int motorID)
{
	uunit VelocityActual;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetVelocityActual(VelocityActual);
			if (err)
			{
				TRACE("GetVelocityActual[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return VelocityActual;
	}
	else return false;
}

bool CmlMotor::GetVelocityActual(int motorID, double& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetVelocityActual(value);
			if (err)
			{
				TRACE("GetVelocityActual[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uunit CmlMotor::GetPositionLoad(int motorID)
{
	uunit PositionLoad;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetPositionLoad(PositionLoad);
			if (err)
			{
				TRACE("GetPositionLoad[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return PositionLoad;
	}
	else return false;
}

bool CmlMotor::GetPositionLoad(int motorID, double& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetPositionLoad(value);
			if (err)
			{
				TRACE("GetPositionLoad[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uunit CmlMotor::GetPositionActual(int motorID)
{
	uunit PositionActual;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetPositionActual(PositionActual);
			if (err)
			{
				TRACE("GetPositionActural[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return  PositionActual;
	}
	else return false;
}

bool CmlMotor::GetPositionActual(int motorID, double& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetPositionActual(value);
			if (err)
			{
				TRACE("GetPositionActural[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uunit CmlMotor::GetVelocityLoad(int motorID)
{
	uunit VelocityLoad;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetVelocityLoad(VelocityLoad);
			if (err)
			{
				TRACE("GetVelocityLoad[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return VelocityLoad;
	}
	else return false;
}

bool CmlMotor::GetVelocityLoad(int motorID, double& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetVelocityLoad(value);
			if (err)
			{
				TRACE("GetVelocityLoad[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

int16 CmlMotor::GetRefVoltage(int motorID)
{
	int16 VoltageValue;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetRefVoltage(VoltageValue);
			if (err)
			{
				TRACE("GetRefVoltage[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return VoltageValue;
	}
	else return false;

}

bool CmlMotor::GetRefVoltage(int motorID, int16& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetRefVoltage(value);
			if (err)
			{
				TRACE("GetRefVoltage[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

int16 CmlMotor::GetCurrentActual(int motorID)//*10mA
{
	int16 CurrentActual;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetCurrentActual(CurrentActual);
			if (err)
			{
				TRACE("GetCurrentActual[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return CurrentActual;
	}
	else return false;
}

bool CmlMotor::GetCurrentActual(int motorID, int16& value)//*10mA
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetCurrentActual(value);
			if (err)
			{
				TRACE("GetCurrentActual[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uint16 CmlMotor::GetDigitalInputs(int motorID)
{
	uint16 DigitalInputsValue;//0x00000000  对应输入口1-8，最低位对应1，最高位对应8，0低1高

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetInputs(DigitalInputsValue, false);
			if (err)
			{
				TRACE("GetDigitalInputs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return DigitalInputsValue;
	}
	else return false;
}

bool CmlMotor::GetDigitalInputs(int motorID, uint16& value)//0x00000000  对应输入口1-8，最低位对应1，最高位对应8，0低1高
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetInputs(value, false);
			if (err)
			{
				TRACE("GetDigitalInputs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uint16 CmlMotor::GetDigitalOutputs(int motorID)
{
	uint16 DigitalOutputsValue;//0x00000000  低四位对应输入口1-4，最低位对应1，最4位对应4，0低1高

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetOutputs(DigitalOutputsValue);
			if (err)
			{
				TRACE("GetDigitalOutputs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return DigitalOutputsValue;
	}
	else return false;
}

bool CmlMotor::GetDigitalOutputs(int motorID, uint16& value)//0x00000000  低四位对应输入口1-4，最低位对应1，最4位对应4，0低1高
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetOutputs(value);
			if (err)
			{
				TRACE("GetDigitalOutputs[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uint16 CmlMotor::GetStatusWord(int motorID)
{
	uint16 value;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetStatusWord(value);
			if (err)
			{
				TRACE("GetStatusWord[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return value;
	}
	else return false;
}

bool CmlMotor::GetStatusWord(int motorID, uint16& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetStatusWord(value);
			if (err)
			{
				TRACE("GetStatusWord[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

uint32 CmlMotor::GetEvent(int motorID)
{
	AMP_EVENT e;
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetEventMask(e);
			if (err)
			{
				TRACE("GetEvent[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return e;
	}
	else return false;
}

bool CmlMotor::GetEvent(int motorID, AMP_EVENT& e)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetEventMask(e);
			if (err)
			{
				TRACE("GetEvent[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::WaitEvent(int motorID, AMP_EVENT event, float time)
{

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			EventNone e(event);
			err = ampObj[motorID - 1].WaitEvent(e, time);
			if (err)
			{
				TRACE("WaitEvent[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SendData(uint32 canID, uint8 data[])
{
	CanFrame canframe;
	canframe.id = canID;
	canframe.length = 8;
	canframe.type = CAN_FRAME_DATA;
	canframe.data[0] = data[0];
	canframe.data[1] = data[1];
	canframe.data[2] = data[2];
	canframe.data[3] = data[3];
	canframe.data[4] = data[4];
	canframe.data[5] = data[5];
	canframe.data[6] = data[6];
	canframe.data[7] = data[7];
	canOpen.Xmit(canframe, 2000);
	return true;
	//接收数据在CanOpen::run(void)里
}

bool CmlMotor::LinkMove(int motorID, double cts[])
{

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		//		Trajectory *trj;
				//trj->StartNew();
		try
		{
			Point<LinkNum> pos;
			for (int i = 0; i < LinkNum; i++)
			{
				pos[i] = cts[i];
			}
			//*****************cts/s  cts/s2  cts/s2  cts/s3******************//
			//link.SetMoveLimits(50000, 5000000, 5000000, 5000000*3);
			link.SetMoveLimits(10000, 50000, 50000, 50000 * 3);
			err = link.MoveTo(pos);


			if (err)
			{
				TRACE("MotorMovePVT[%d]:%s\n", motorID, err->toString());
				return false;
			}
			//err = link.WaitMoveDone(1000 * 30);
			//err = ampObj[motorIndex].SendTrajectory(*trj, true);
			//err = ampObj[motorIndex].StartPVT();
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;

}

bool CmlMotor::LinkStop()
{
	if (!m_bOpen)
	{
		return false;
	}
	try
	{
		link.HaltMove();
		if (err)
		{
			TRACE("link.HaltMove:%s\n", err->toString());
			return false;
		}
	}
	catch (...)
	{
		return false;
	}
	return true;

}

bool CmlMotor::LinkInit()
{
	if (!m_bOpen)
	{
		return false;
	}
	try
	{
		err = link.Init(LinkNum, ampObj);
		if (err)
		{
			TRACE("link.Init:%s\n", err->toString());
			return false;
		}

	}
	catch (...)
	{
		return false;
	}
	return true;
}

uunit CmlMotor::GetPositionCommand(int motorID)
{
	uunit PositionCommand;

	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetPositionCommand(PositionCommand);
			if (err)
			{
				TRACE("GetPositionCommand[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return PositionCommand;
	}
	else return false;
}

bool CmlMotor::GetPositionCommand(int motorID, double& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetPositionCommand(value);
			if (err)
			{
				TRACE("GetPositionCommand[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::SetOutputsConfig(int motorID, int8 pin, OUTPUT_PIN_CONFIG cfg, uint32 mask1)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		err = ampObj[motorID - 1].SetOutputConfig(pin - 1, cfg, mask1);
		if (err)
		{
			TRACE("SetOutputsConfig[%d]:%s\n", motorID, err->toString());
			return false;
		}
		OUTPUT_PIN_CONFIG cfg_read;
		uint32 mask1_read;
		err = ampObj[motorID - 1].GetOutputConfig(pin - 1, cfg_read, mask1_read);//设置完后再读一下确认是否设置正确
		if ((cfg_read == cfg) && (mask1 == mask1_read))
		{
			return true;
		}
		else
		{
			return false;
		}

	}
	else return false;

}

bool CmlMotor::SetPositionActual(int motorID, double value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].SetPositionActual(value);
			if (err)
			{
				TRACE("SetPositionActual[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}

bool CmlMotor::GetHomeAdjustment(int motorID, double& value)
{
	if (!m_bOpen)
	{
		return false;
	}
	if (MotorIds.find(motorID) != MotorIds.end())
	{
		try
		{
			err = ampObj[motorID - 1].GetHomeAdjustment(value);
			if (err)
			{
				TRACE("GetHomeAdjustment[%d]:%s\n", motorID, err->toString());
				return false;
			}
		}
		catch (...)
		{
			return false;
		}
		return true;
	}
	else return false;
}