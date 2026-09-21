#include "../NineAxisController.h"
#include "../BladeSequence.h"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <functional>

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error(#expr); } while (0)
using namespace machine;
using namespace inspection;

struct FakeMotor : IMotorPort {
    std::map<int, AxisFeedback> feedback;
    int opens = 0, stops = 0, moves = 0, limits = 0, enables = 0;
    int badLimit = 0, badEnable = 0, badRead = 0;
    bool openOk = true, stopOk = true, profileOk = true, moveOk = true;
    double lastTarget = 0;
    FakeMotor() { for (int i = 1; i <= 12; ++i) { feedback[i].done = true; feedback[i].position = 10; } }
    bool Open(const std::vector<AxisConfig>& axes) override { ++opens; CHECK(axes.size() == 9); return openOk; }
    bool Read(int id, AxisFeedback& f) override { f = feedback.at(id); return id != badRead; }
    bool Limit(int id, double, double) override { ++limits; CHECK(enables == 0); return id != badLimit; }
    bool Enable(int id) override { ++enables; CHECK(limits == 9); feedback[id].enabled = true; return id != badEnable; }
    bool Profile(int, double, double) override { return profileOk; }
    bool Move(int id, double target) override { ++moves; lastTarget = target; feedback[id].done = false; return moveOk; }
    bool StopAndClose() override { ++stops; for (auto& f : feedback) f.second.enabled = false; return stopOk; }
};
FeedConfig Feed() {
    FeedConfig f; f.confirmed = true; f.negative = -100; f.positive = 100;
    f.velocity = 10; f.acceleration = 20; f.step = 5; f.tolerance = .5; f.timeoutMs = 1000; return f;
}
std::vector<AxisConfig> Robot() {
    std::vector<AxisConfig> a; for (int i = 1; i <= 8; ++i) a.push_back({i,-1000,1000}); return a;
}
struct FakeTracking : ITrackingBackend {
    Feedback f;
    unsigned prepares = 0, tracks = 0, retracts = 0, indexes = 0, stops = 0;
    double lastAngle = 0;
    bool readOk = true, commandOk = true, stopOk = true, fresh = true;
    void Identity(std::uint64_t run, unsigned blade) { f = Feedback{}; f.run = run; f.blade = blade; f.phaseLocked = true; }
    bool Prepare(std::uint64_t run, unsigned blade, double angle) override { ++prepares; Identity(run, blade); lastAngle = angle; return commandOk; }
    bool StartTracking(std::uint64_t run, unsigned blade) override { ++tracks; Identity(run, blade); return commandOk; }
    bool Retract(std::uint64_t run, unsigned blade) override { ++retracts; Identity(run, blade); return commandOk; }
    bool Index(std::uint64_t run, unsigned blade, double angle) override { ++indexes; Identity(run, blade); lastAngle = angle; return commandOk; }
    bool Read(Feedback& out, std::uint64_t now) override { if (fresh) f.timestampMs = now; out = f; return readOk; }
    bool Stop() override { ++stops; return stopOk; }
};

int main() {
    int cases = 0;
    auto test = [&](const char* name, std::function<void()> body) {
        try { body(); ++cases; std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { std::cerr << "FAIL " << name << ": " << e.what() << '\n'; throw; }
    };
    test("unconfirmed config never opens hardware", [] {
        FakeMotor p; NineAxisController c(p); auto f = Feed(); f.confirmed = false;
        CHECK(!c.Connect(f, Robot())); CHECK(p.opens == 0);
    });
    test("invalid node limits speed direction timeout and NaN rejected", [] {
        const auto nan = std::numeric_limits<double>::quiet_NaN();
        std::vector<FeedConfig> invalid;
        auto f = Feed(); f.node = 8; invalid.push_back(f); f = Feed(); f.node = 13; invalid.push_back(f);
        f = Feed(); f.negative = f.positive; invalid.push_back(f);
        f = Feed(); f.velocity = -1; invalid.push_back(f); f = Feed(); f.acceleration = nan; invalid.push_back(f);
        f = Feed(); f.step = nan; invalid.push_back(f); f = Feed(); f.tolerance = f.step; invalid.push_back(f);
        f = Feed(); f.direction = 0; invalid.push_back(f); f = Feed(); f.timeoutMs = 0; invalid.push_back(f);
        for (auto bad : invalid) { FakeMotor p; NineAxisController c(p); CHECK(!c.Connect(bad, Robot())); CHECK(!p.opens); }
    });
    test("robot must contain exactly all eight unique nodes", [] {
        FakeMotor p; NineAxisController c(p); auto a = Robot(); a[7].id = 1;
        CHECK(!c.Connect(Feed(), a)); CHECK(!p.opens);
    });
    test("nine axes setup before any enable", [] {
        FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(), Robot()));
        CHECK(c.Ready()); CHECK(p.enables == 9); CHECK(c.Origins().size() == 9); CHECK(p.moves == 0);
        CHECK(!c.Connect(Feed(), Robot())); CHECK(p.opens == 1);
    });
    test("partial bus initialization rolls back", [] {
        FakeMotor p; p.openOk = false; NineAxisController c(p); CHECK(!c.Connect(Feed(), Robot())); CHECK(p.stops == 1); CHECK(!c.Ready());
    });
    test("every failing limit rolls back before enabling", [] {
        for (int id=1;id<=9;++id) { FakeMotor p; p.badLimit=id; NineAxisController c(p);
            CHECK(!c.Connect(Feed(), Robot())); CHECK(!p.enables); CHECK(p.stops == 1); }
    });
    test("every failing enable rolls back earlier axes", [] {
        for (int id=1;id<=9;++id) { FakeMotor p; p.badEnable=id; NineAxisController c(p);
            CHECK(!c.Connect(Feed(), Robot())); CHECK(p.stops == 1); for (auto& item:p.feedback) CHECK(!item.second.enabled); }
    });
    test("invalid initial position prevents enable", [] {
        FakeMotor p; p.feedback[9].position = 200; NineAxisController c(p);
        CHECK(!c.Connect(Feed(), Robot())); CHECK(!p.enables); CHECK(p.stops == 1);
    });
    test("feed profile failure rolls back", [] {
        FakeMotor p; p.profileOk=false; NineAxisController c(p); CHECK(!c.Connect(Feed(), Robot())); CHECK(p.stops==1);
    });
    test("feed target feedback and busy interlock", [] {
        FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(), Robot()));
        CHECK(c.FeedMove(5,false,100)); CHECK(p.lastTarget==15); CHECK(c.Busy());
        CHECK(!c.FeedMove(5,false,101)); CHECK(p.moves==1);
        double pos; p.feedback[9].position=15; CHECK(c.Poll(200,pos)); CHECK(c.Busy());
        p.feedback[9].done=true; CHECK(c.Poll(300,pos)); CHECK(!c.Busy());
        CHECK(c.FeedMove(0,true,400)); CHECK(p.lastTarget==10);
    });
    test("feed direction independent and node configurable", [] {
        FakeMotor p; NineAxisController c(p); auto f=Feed(); f.node=12; f.direction=-1;
        CHECK(c.Connect(f,Robot())); CHECK(c.FeedMove(5,false,0)); CHECK(p.lastTarget==5); CHECK(!p.feedback[12].done);
    });
    test("feed bounds and nonfinite target send nothing", [] {
        FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(),Robot()));
        CHECK(!c.FeedMove(1000,false,0)); CHECK(!c.FeedMove(std::numeric_limits<double>::infinity(),false,0)); CHECK(!p.moves);
    });
    test("feed command failure stops nine axes", [] {
        FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(),Robot())); p.moveOk=false;
        CHECK(!c.FeedMove(5,false,0)); CHECK(p.stops==1); CHECK(!c.Ready());
    });
    test("wrong reached position does not complete then times out", [] {
        FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(),Robot())); CHECK(c.FeedMove(5,false,0));
        p.feedback[9].done=true; double pos; CHECK(c.Poll(500,pos)); CHECK(c.Busy());
        CHECK(!c.Poll(1001,pos)); CHECK(p.stops==1); CHECK(!c.Ready());
    });
    test("loss of any axis feedback stops all", [] {
        for (int id=1;id<=9;++id) { FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(),Robot()));
            p.badRead=id; double pos; CHECK(!c.Poll(1,pos)); CHECK(p.stops==1); }
    });
    test("drive fault and disabled feedback stop all", [] {
        for (bool fault : {true,false}) { FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(),Robot()));
            if(fault) p.feedback[7].fault=true; else p.feedback[8].enabled=false;
            double pos; CHECK(!c.Poll(1,pos)); CHECK(!c.Ready()); }
    });
    test("stop does not command home and reports failure", [] {
        FakeMotor p; NineAxisController c(p); CHECK(c.Connect(Feed(),Robot())); p.stopOk=false;
        CHECK(!c.Stop()); CHECK(!c.Ready()); CHECK(!p.moves); CHECK(!c.Error().empty());
    });
    test("demo completes precisely three blades without hardware", [] {
        DemoBackend b; BladeSequence c(b); CHECK(c.Start(Plan{},0));
        for(std::uint64_t t=100;c.Active() && t<10000;t+=100) CHECK(c.Tick(t));
        CHECK(c.Current()==State::Completed); CHECK(c.Completed()==3); CHECK(c.Blade()==2);
    });
    test("index waits for scan completion AND retraction", [] {
        FakeTracking b; BladeSequence c(b); Plan p; p.firstAngleDeg=20; p.direction=-1;
        CHECK(c.Start(p,0)); CHECK(b.lastAngle==20); CHECK(!c.Start(p,0));
        b.f.prepared=true; CHECK(c.Tick(1)); CHECK(b.tracks==1);
        CHECK(c.Tick(2)); CHECK(!b.retracts); CHECK(!b.indexes);
        b.f.scanDone=true; CHECK(c.Tick(3)); CHECK(b.retracts==1); CHECK(!b.indexes);
        CHECK(c.Tick(4)); CHECK(!b.indexes);
        b.f.retracted=true; CHECK(c.Tick(5)); CHECK(b.indexes==1); CHECK(b.lastAngle==-100);
        CHECK(c.Tick(6)); CHECK(b.prepares==1);
        b.f.indexed=true; CHECK(c.Tick(7)); CHECK(b.prepares==2); CHECK(c.Blade()==1);
    });
    test("single blade completion stops instead of indexing", [] {
        FakeTracking b; BladeSequence c(b); Plan p; p.bladeCount=1; CHECK(c.Start(p,0));
        b.f.prepared=true; CHECK(c.Tick(1)); b.f.scanDone=true; CHECK(c.Tick(2));
        b.f.retracted=true; CHECK(c.Tick(3)); CHECK(c.Current()==State::Completed); CHECK(!b.indexes); CHECK(b.stops==1);
    });
    test("invalid plans rejected before backend command", [] {
        FakeTracking b; BladeSequence c(b); Plan p; p.bladeCount=0; CHECK(!c.Start(p,0));
        p=Plan{}; p.firstAngleDeg=std::numeric_limits<double>::quiet_NaN(); CHECK(!c.Start(p,0)); CHECK(!b.prepares);
    });
    test("prepare tracking retract index failures stop backend", [] {
        for(int stage=0;stage<4;++stage) { FakeTracking b; BladeSequence c(b);
            if(stage==0) { b.commandOk=false; CHECK(!c.Start(Plan{},0)); }
            else { CHECK(c.Start(Plan{},0)); b.f.prepared=true;
                if(stage==1) { b.commandOk=false; CHECK(!c.Tick(1)); }
                else { CHECK(c.Tick(1)); b.f.scanDone=true;
                    if(stage==2) { b.commandOk=false; CHECK(!c.Tick(2)); }
                    else { CHECK(c.Tick(2)); b.f.retracted=true; b.commandOk=false; CHECK(!c.Tick(3)); }
                }
            }
            CHECK(c.Current()==State::Fault); CHECK(b.stops>=1);
        }
    });
    test("phase loss excessive error and NaN abort tracking", [] {
        for(int mode=0;mode<3;++mode) { FakeTracking b; BladeSequence c(b); CHECK(c.Start(Plan{},0));
            b.f.prepared=true; CHECK(c.Tick(1));
            if(mode==0) b.f.phaseLocked=false; if(mode==1) b.f.phaseErrorDeg=1;
            if(mode==2) b.f.phaseErrorDeg=std::numeric_limits<double>::quiet_NaN();
            CHECK(!c.Tick(2)); CHECK(c.Current()==State::Fault); CHECK(!b.indexes); CHECK(b.stops==1);
        }
    });
    test("read fault stale future wrong run or blade abort", [] {
        for(int mode=0;mode<6;++mode) { FakeTracking b; BladeSequence c(b); CHECK(c.Start(Plan{},1000));
            if(mode==0) b.readOk=false; if(mode==1) b.f.fault=true;
            if(mode==2) { b.fresh=false; b.f.timestampMs=0; }
            if(mode==3) { b.fresh=false; b.f.timestampMs=3000; }
            if(mode==4) ++b.f.run; if(mode==5) ++b.f.blade;
            CHECK(!c.Tick(1600)); CHECK(c.Current()==State::Fault); CHECK(b.stops==1);
        }
    });
    test("state timeout and backwards clock abort", [] {
        for(bool backwards : {true,false}) { FakeTracking b; BladeSequence c(b); CHECK(c.Start(Plan{},100));
            CHECK(!c.Tick(backwards ? 99 : 10101)); CHECK(c.Current()==State::Fault); }
    });
    test("stop during tracking forbids subsequent index", [] {
        FakeTracking b; BladeSequence c(b); CHECK(c.Start(Plan{},0)); b.f.prepared=true; CHECK(c.Tick(1));
        CHECK(c.Stop()); b.f.scanDone=true; CHECK(!c.Tick(2)); CHECK(!b.indexes); CHECK(c.Current()==State::Stopped);
    });
    test("failed stop remains fault rather than success", [] {
        FakeTracking b; BladeSequence c(b); CHECK(c.Start(Plan{},0)); b.stopOk=false;
        CHECK(!c.Stop()); CHECK(c.Current()==State::Fault); CHECK(!c.Error().empty());
    });
    std::cout << "All " << cases << " test groups passed. No hardware accessed.\n";
}
