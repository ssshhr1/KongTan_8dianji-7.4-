#pragma once
#include <cmath>
#include <cstdint>
#include <string>

namespace inspection {
enum class State { Idle, Preparing, Tracking, Retracting, Indexing, Completed, Stopped, Fault };
struct Plan {
    unsigned bladeCount = 3;
    double firstAngleDeg = 0;
    int direction = 1;
    std::uint64_t stateTimeoutMs = 10000, feedbackTimeoutMs = 500;
    double maxPhaseErrorDeg = 0.5;
    bool Valid() const {
        return bladeCount > 0 && bladeCount <= 10000 && std::isfinite(firstAngleDeg) &&
            (direction == 1 || direction == -1) && stateTimeoutMs > 0 &&
            feedbackTimeoutMs > 0 && feedbackTimeoutMs <= stateTimeoutMs &&
            std::isfinite(maxPhaseErrorDeg) && maxPhaseErrorDeg > 0 && maxPhaseErrorDeg < 180;
    }
};
struct Feedback {
    std::uint64_t timestampMs = 0, run = 0;
    unsigned blade = 0; // Zero based, from the active command; stale replies cannot advance a blade.
    bool fault = false, prepared = false, phaseLocked = false;
    bool scanDone = false, retracted = false, indexed = false;
    double phaseErrorDeg = 0;
};
// Supervisory handshake only. The backend must execute encoder-master tracking,
// synchronized trajectories and acquisition triggers on the hardware controller.
// All methods must return promptly; Stop must stop BOTH rotor and robot and acquisition.
class ITrackingBackend {
public:
    virtual ~ITrackingBackend() = default;
    virtual bool Prepare(std::uint64_t run, unsigned blade, double angleDeg) = 0;
    virtual bool StartTracking(std::uint64_t run, unsigned blade) = 0;
    virtual bool Retract(std::uint64_t run, unsigned blade) = 0;
    virtual bool Index(std::uint64_t run, unsigned nextBlade, double angleDeg) = 0;
    virtual bool Read(Feedback& feedback, std::uint64_t now) = 0;
    virtual bool Stop() = 0;
};
class BladeSequence {
    ITrackingBackend& backend_;
    Plan plan_;
    State state_ = State::Idle;
    unsigned blade_ = 0, completed_ = 0;
    std::uint64_t run_ = 0, entered_ = 0, lastTick_ = 0, lastFeedback_ = 0;
    std::string error_;
    bool Fail(const char* reason) {
        state_ = State::Fault; error_ = reason;
        if (!backend_.Stop()) error_ += "; backend stop failed";
        return false;
    }
    void Enter(State s, std::uint64_t now) { state_ = s; entered_ = now; }
    double Angle(unsigned blade) const { return plan_.firstAngleDeg + plan_.direction * static_cast<double>(blade) * (360.0 / plan_.bladeCount); }
public:
    explicit BladeSequence(ITrackingBackend& backend) : backend_(backend) {}
    State Current() const { return state_; }
    unsigned Blade() const { return blade_; }
    unsigned Completed() const { return completed_; }
    const std::string& Error() const { return error_; }
    bool Active() const { return state_ == State::Preparing || state_ == State::Tracking || state_ == State::Retracting || state_ == State::Indexing; }
    bool Start(const Plan& plan, std::uint64_t now) {
        if (Active()) return false;
        if (!plan.Valid()) { error_ = "Invalid inspection plan"; return false; }
        plan_ = plan; blade_ = completed_ = 0; ++run_; error_.clear();
        lastTick_ = lastFeedback_ = now;
        Enter(State::Preparing, now);
        if (!backend_.Prepare(run_, blade_, Angle(blade_))) return Fail("Prepare failed");
        return true;
    }
    bool Stop() {
        if (!backend_.Stop()) return Fail("Stop failed");
        state_ = State::Stopped; return true;
    }
    bool Tick(std::uint64_t now) {
        if (!Active()) return false;
        if (now < lastTick_) return Fail("Clock moved backwards");
        lastTick_ = now;
        if (now - entered_ > plan_.stateTimeoutMs) return Fail("State timeout");
        Feedback f;
        if (!backend_.Read(f, now)) return Fail("Feedback read failed");
        if (f.fault) return Fail("Backend fault");
        if (f.timestampMs > now || f.timestampMs < lastFeedback_ || now - f.timestampMs > plan_.feedbackTimeoutMs)
            return Fail("Stale/invalid feedback clock");
        // Backend clears completion flags on accepting each command.
        const unsigned expected = state_ == State::Indexing ? blade_ + 1 : blade_;
        if (f.run != run_ || f.blade != expected) return Fail("Feedback command identity mismatch");
        lastFeedback_ = f.timestampMs;
        switch (state_) {
        case State::Preparing:
            if (f.prepared && f.phaseLocked) {
                if (!std::isfinite(f.phaseErrorDeg) || std::abs(f.phaseErrorDeg) > plan_.maxPhaseErrorDeg)
                    return Fail("Phase error before tracking");
                if (!backend_.StartTracking(run_, blade_)) return Fail("Tracking start failed");
                Enter(State::Tracking, now);
            }
            break;
        case State::Tracking:
            if (!f.phaseLocked || !std::isfinite(f.phaseErrorDeg) || std::abs(f.phaseErrorDeg) > plan_.maxPhaseErrorDeg)
                return Fail("Tracking phase lost");
            if (f.scanDone) {
                if (!backend_.Retract(run_, blade_)) return Fail("Retraction failed");
                Enter(State::Retracting, now);
            }
            break;
        case State::Retracting:
            if (f.retracted) {
                ++completed_;
                if (completed_ == plan_.bladeCount) {
                    if (!backend_.Stop()) return Fail("Final stop failed");
                    Enter(State::Completed, now);
                } else {
                    if (!backend_.Index(run_, blade_ + 1, Angle(blade_ + 1))) return Fail("Index command failed");
                    Enter(State::Indexing, now);
                }
            }
            break;
        case State::Indexing:
            if (f.indexed) {
                ++blade_;
                if (!backend_.Prepare(run_, blade_, Angle(blade_))) return Fail("Next blade prepare failed");
                Enter(State::Preparing, now);
            }
            break;
        default: break;
        }
        return true;
    }
};

// Deliberately has NO motor/driver reference. Demonstrates handshakes, not physical tracking.
class DemoBackend : public ITrackingBackend {
    Feedback feedback_;
    unsigned ticks_ = 0;
    State operation_ = State::Idle;
    void Command(std::uint64_t run, unsigned blade, State op) {
        feedback_ = Feedback{}; feedback_.run = run; feedback_.blade = blade;
        operation_ = op; ticks_ = 0;
    }
public:
    bool Prepare(std::uint64_t run, unsigned blade, double) override { Command(run, blade, State::Preparing); return true; }
    bool StartTracking(std::uint64_t run, unsigned blade) override { Command(run, blade, State::Tracking); return true; }
    bool Retract(std::uint64_t run, unsigned blade) override { Command(run, blade, State::Retracting); return true; }
    bool Index(std::uint64_t run, unsigned blade, double) override { Command(run, blade, State::Indexing); return true; }
    bool Read(Feedback& out, std::uint64_t now) override {
        ++ticks_; feedback_.timestampMs = now; feedback_.phaseLocked = true;
        const bool done = ticks_ >= 5;
        feedback_.prepared = done && operation_ == State::Preparing;
        feedback_.scanDone = done && operation_ == State::Tracking;
        feedback_.retracted = done && operation_ == State::Retracting;
        feedback_.indexed = done && operation_ == State::Indexing;
        out = feedback_; return true;
    }
    bool Stop() override { operation_ = State::Stopped; return true; }
};
} // namespace inspection
