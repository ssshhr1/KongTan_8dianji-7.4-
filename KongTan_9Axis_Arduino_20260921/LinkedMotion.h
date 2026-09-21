#pragma once
#include "NineAxisController.h"
#include <algorithm>
#include <array>

namespace machine {
// Absolute axis coordinates: Copley encoder counts for 1..8, emitted pulses for 9.
struct LinkSettings {
    double robotVelocity = 2000, robotAcceleration = 2000, robotTolerance = 10;
    double robotSegment = 200, feedSegment = 1;
    double feedVelocity = 1, feedAcceleration = 1;
    std::uint64_t segmentTimeoutMs = 15000;
    bool Valid() const {
        for (double x : {robotVelocity, robotAcceleration, robotTolerance, robotSegment,
                         feedSegment, feedVelocity, feedAcceleration})
            if (!std::isfinite(x) || x <= 0) return false;
        return robotSegment >= 1 && feedSegment >= 1 && segmentTimeoutMs >= 1000 && segmentTimeoutMs <= 600000;
    }
};
inline std::array<double,9> BendDeltas(double tipUp, double tipLeft, double middleUp, double baseUp, double feed) {
    return {{tipUp, -tipUp, tipLeft, -tipLeft, middleUp, -middleUp, baseUp, -baseUp, feed}};
}
class LinkedMotion {
    IMotorPort& port_;
    LinkSettings cfg_;
    std::array<double,9> start_{}, end_{}, target_{}, prior_{};
    std::array<AxisConfig,9> limits_{};
    bool active_ = false;
    unsigned segment_ = 0, count_ = 0;
    std::uint64_t started_ = 0, minimumMs_ = 0;
    std::string error_;
    bool Fail(const std::string& reason) {
        active_ = false; error_ = reason;
        if (!port_.StopAndClose()) error_ += "; stop not confirmed";
        return false;
    }
    bool Dispatch(std::uint64_t now) {
        ++segment_;
        double seconds = 0.25;
        for (int i = 0; i < 9; ++i) {
            prior_[i] = target_[i];
            target_[i] = std::round(start_[i] + (end_[i] - start_[i]) * segment_ / count_);
            const double d = std::abs(target_[i] - prior_[i]);
            const double v = i == 8 ? cfg_.feedVelocity : cfg_.robotVelocity;
            const double a = i == 8 ? cfg_.feedAcceleration : cfg_.robotAcceleration;
            seconds = (std::max)(seconds, (std::max)(2*d/v, std::sqrt(4*d/a)));
        }
        // Round upward to the same millisecond duration used by the Arduino.
        minimumMs_ = static_cast<std::uint64_t>(std::ceil(seconds * 1000));
        if (minimumMs_ + 500 >= cfg_.segmentTimeoutMs) return Fail("Segment duration exceeds timeout; reduce step or increase timeout");
        seconds = minimumMs_ / 1000.0;
        for (int i = 0; i < 9; ++i) {
            const double d = std::abs(target_[i] - prior_[i]);
            if (d && !port_.Profile(i+1, 2*d/seconds, 4*d/(seconds*seconds)))
                return Fail("Linked profile failed at axis " + std::to_string(i+1));
        }
        // Copley commands first, feed last: feed cannot start before robot dispatch.
        for (int i = 0; i < 9; ++i)
            if (target_[i] != prior_[i] && !port_.Move(i+1, target_[i]))
                return Fail("Linked move failed at axis " + std::to_string(i+1));
        started_ = now;
        return true;
    }
public:
    explicit LinkedMotion(IMotorPort& port) : port_(port) {}
    bool Active() const { return active_; }
    unsigned Segment() const { return segment_; }
    unsigned Count() const { return count_; }
    const std::string& Error() const { return error_; }
    void Cancel() { active_ = false; }
    bool Start(const std::array<double,9>& delta, const std::array<AxisConfig,9>& limits,
               const LinkSettings& cfg, std::uint64_t now) {
        if (active_) { error_ = "Motion already active"; return false; }
        error_.clear();
        if (!cfg.Valid()) { error_ = "Invalid link settings"; return false; }
        cfg_ = cfg; limits_ = limits; count_ = 1; segment_ = 0;
        bool any = false;
        for (int i = 0; i < 9; ++i) {
            AxisFeedback f;
            if (!port_.Read(i+1, f) || f.fault || !f.enabled || !f.done || !std::isfinite(f.position))
                return Fail("Axis not at rest/feedback invalid: " + std::to_string(i+1));
            start_[i] = target_[i] = std::round(f.position);
            end_[i] = std::round(start_[i] + delta[i]);
            if (limits[i].id != i+1 || !std::isfinite(limits[i].negative) || !std::isfinite(limits[i].positive) ||
                limits[i].negative >= limits[i].positive || !std::isfinite(delta[i]) ||
                !std::isfinite(end_[i]) || start_[i] < limits[i].negative || start_[i] > limits[i].positive ||
                end_[i] < limits[i].negative || end_[i] > limits[i].positive) {
                error_ = "Invalid/over-limit linked target: " + std::to_string(i+1); return false;
            }
            const double d = std::abs(end_[i] - start_[i]);
            any = any || d != 0;
            const double n = std::ceil(d / (i == 8 ? cfg.feedSegment : cfg.robotSegment));
            if (n > 10000) { error_ = "Too many segments (maximum 10000)"; return false; }
            count_ = (std::max)(count_, static_cast<unsigned>(n));
        }
        if (!any) { error_ = "All rounded displacements are zero"; return false; }
        active_ = true;
        return Dispatch(now);
    }
    bool Tick(std::uint64_t now) {
        if (!active_) return true;
        if (now < started_ || now - started_ > cfg_.segmentTimeoutMs) return Fail("Linked segment timeout");
        bool all = true;
        for (int i = 0; i < 9; ++i) {
            AxisFeedback f;
            if (!port_.Read(i+1, f) || f.fault || !f.enabled || !std::isfinite(f.position) ||
                f.position < limits_[i].negative || f.position > limits_[i].positive)
                return Fail("Linked feedback fault: " + std::to_string(i+1));
            const double tolerance = i == 8 ? 0.0 : cfg_.robotTolerance;
            all = all && f.done && std::abs(f.position-target_[i]) <= tolerance;
        }
        // Duration guard avoids interpreting an old MOVEDONE bit as completion.
        if (!all || now-started_ < minimumMs_) return true;
        if (segment_ == count_) { active_ = false; return true; }
        return Dispatch(now);
    }
};
}
