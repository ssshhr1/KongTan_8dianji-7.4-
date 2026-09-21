#pragma once
#include "NineAxisController.h"
#include "include/CML_lib-C++/CMLMotorCtrl.h"

class CmlMotorPort : public machine::IMotorPort {
    CmlMotor& motor_;
public:
    explicit CmlMotorPort(CmlMotor& motor) : motor_(motor) {}
    bool Open(const std::vector<machine::AxisConfig>& axes) override {
        // MotorIds may only change after the previous bus is completely closed.
        if (!motor_.CloseMotorBus()) return false;
        MotorIds.clear();
        for (const auto& a : axes) MotorIds.insert(a.id);
        return motor_.OpenMotorBus();
    }
    bool Read(int id, machine::AxisFeedback& f) override {
        CML::uint16 status = 0;
        CML::AMP_EVENT events = static_cast<CML::AMP_EVENT>(0);
        if (!motor_.GetPositionActual(id, f.position) || !motor_.GetStatusWord(id, status) || !motor_.GetEvent(id, events)) return false;
        f.enabled = (status & 0x006f) == 0x0027;
        f.fault = (status & 0x0008) != 0 || (events & (CML::AMPEVENT_FAULT | CML::AMPEVENT_ERROR)) != 0;
        f.done = (events & CML::AMPEVENT_MOVEDONE) != 0;
        return true;
    }
    bool Limit(int id, double negative, double positive) override { return motor_.SetSoftLimit(id, positive, negative); }
    bool Enable(int id) override { return motor_.EnableMotor(id, true); }
    bool Profile(int id, double velocity, double acceleration) override { return motor_.SetMotorVelocity(id, velocity, acceleration, acceleration); }
    bool Move(int id, double target) override { return motor_.MotorMoveAbs(id, target); }
    bool StopAndClose() override { return motor_.CloseMotorBus(); }
};
