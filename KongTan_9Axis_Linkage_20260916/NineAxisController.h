#pragma once
#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace machine {
struct AxisConfig {
    int id;
    double negative, positive;
};
struct FeedConfig {
    int node = 9; // Placeholder until confirmed against the drive.
    bool confirmed = false;
    double negative = 0, positive = 0;
    double velocity = 0, acceleration = 0, step = 0, tolerance = 0;
    int direction = 1;
    std::uint64_t timeoutMs = 30000;
    bool Valid() const {
        return confirmed && node >= 9 && node <= 12 &&
            std::isfinite(negative) && std::isfinite(positive) &&
            negative >= -2147483647.0 && positive <= 2147483647.0 && negative < positive &&
            std::isfinite(velocity) && velocity > 0 && velocity <= 2147483647.0 &&
            std::isfinite(acceleration) && acceleration > 0 && acceleration <= 2147483647.0 &&
            std::isfinite(step) && step > 0 && step <= positive - negative &&
            std::isfinite(tolerance) && tolerance > 0 && tolerance < step &&
            (direction == 1 || direction == -1) && timeoutMs >= 100 && timeoutMs <= 600000;
    }
};
struct AxisFeedback {
    double position = 0;
    bool enabled = false, fault = false, done = false;
};
class IMotorPort {
public:
    virtual ~IMotorPort() = default;
    virtual bool Open(const std::vector<AxisConfig>& axes) = 0;
    virtual bool Read(int id, AxisFeedback& feedback) = 0;
    virtual bool Limit(int id, double negative, double positive) = 0;
    virtual bool Enable(int id) = 0;
    virtual bool Profile(int id, double velocity, double acceleration) = 0;
    virtual bool Move(int id, double target) = 0;
    // Must try every initialized axis even if an earlier stop/disable fails.
    virtual bool StopAndClose() = 0;
};

class NineAxisController {
    IMotorPort& port_;
    FeedConfig feed_;
    std::vector<AxisConfig> axes_;
    std::map<int, double> origins_;
    bool ready_ = false, busy_ = false;
    double target_ = 0;
    std::uint64_t started_ = 0;
    std::string error_;
    bool Fail(const std::string& reason) {
        ready_ = busy_ = false;
        error_ = reason;
        if (!port_.StopAndClose()) error_ += "; STOP/CLOSE failed: check hardware";
        return false;
    }
public:
    explicit NineAxisController(IMotorPort& port) : port_(port) {}
    bool Ready() const { return ready_; }
    bool Busy() const { return busy_; }
    const std::string& Error() const { return error_; }
    const std::map<int, double>& Origins() const { return origins_; }
    bool Connect(const FeedConfig& feed, const std::vector<AxisConfig>& robot) {
        if (ready_) return false;
        error_.clear();
        origins_.clear();
        if (!feed.Valid() || robot.size() != 8) { error_ = "Invalid/unconfirmed feed configuration"; return false; }
        std::set<int> ids;
        for (const auto& a : robot) {
            if (a.id < 1 || a.id > 8 || !ids.insert(a.id).second ||
                !std::isfinite(a.negative) || !std::isfinite(a.positive) ||
                a.negative < -2147483647.0 || a.positive > 2147483647.0 || a.negative >= a.positive) {
                error_ = "Invalid robot axis configuration"; return false;
            }
        }
        feed_ = feed; axes_ = robot;
        axes_.push_back({feed.node, feed.negative, feed.positive});
        if (!port_.Open(axes_)) return Fail("CAN/node initialization failed");
        // Capture all positions and program all limits BEFORE enabling any axis.
        for (const auto& a : axes_) {
            AxisFeedback f;
            if (!port_.Read(a.id, f) || f.fault || !std::isfinite(f.position) ||
                f.position < a.negative || f.position > a.positive)
                return Fail("Invalid position/fault at node " + std::to_string(a.id));
            origins_[a.id] = f.position;
            if (!port_.Limit(a.id, a.negative, a.positive))
                return Fail("Soft limit setup failed at node " + std::to_string(a.id));
        }
        if (!port_.Profile(feed.node, feed.velocity, feed.acceleration)) return Fail("Feed profile setup failed");
        for (const auto& a : axes_) {
            AxisFeedback f;
            if (!port_.Enable(a.id) || !port_.Read(a.id, f) || !f.enabled || f.fault)
                return Fail("Enable failed at node " + std::to_string(a.id));
        }
        ready_ = true;
        return true;
    }
    bool Stop() {
        ready_ = busy_ = false;
        const bool ok = port_.StopAndClose();
        error_ = ok ? "" : "STOP/CLOSE failed: check hardware";
        return ok;
    }
    bool FeedMove(double signedStep, bool returnToOrigin, std::uint64_t now) {
        if (!ready_ || busy_) { error_ = "Not ready or feed already moving"; return false; }
        AxisFeedback f;
        if (!port_.Read(feed_.node, f) || !f.enabled || f.fault || !f.done || !std::isfinite(f.position))
            return Fail("Feed feedback unavailable/not at rest");
        const double target = returnToOrigin ? origins_.at(feed_.node) : f.position + signedStep * feed_.direction;
        if (!std::isfinite(signedStep) || !std::isfinite(target) || target < feed_.negative || target > feed_.positive) {
            error_ = "Feed target exceeds configured limits"; return false;
        }
        if (!port_.Profile(feed_.node, feed_.velocity, feed_.acceleration) || !port_.Move(feed_.node, target))
            return Fail("Feed command failed");
        target_ = target; started_ = now; busy_ = true; error_.clear();
        return true;
    }
    bool Poll(std::uint64_t now, double& feedPosition) {
        if (!ready_) return false;
        AxisFeedback feedFeedback;
        for (const auto& a : axes_) {
            AxisFeedback f;
            if (!port_.Read(a.id, f) || f.fault || !f.enabled || !std::isfinite(f.position) ||
                f.position < a.negative || f.position > a.positive)
                return Fail("Feedback/fault/limit at node " + std::to_string(a.id));
            if (a.id == feed_.node) feedFeedback = f;
        }
        feedPosition = feedFeedback.position;
        if (busy_) {
            if (now < started_ || now - started_ > feed_.timeoutMs) return Fail("Feed movement timeout/clock error");
            if (feedFeedback.done && std::abs(feedPosition - target_) <= feed_.tolerance) busy_ = false;
        }
        return true;
    }
};
} // namespace machine
