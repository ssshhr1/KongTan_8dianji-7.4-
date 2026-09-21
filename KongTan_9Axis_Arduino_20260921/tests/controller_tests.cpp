#include "../LinkedMotion.h"
#include "../firmware/FeedAxis/FeedCore.h"
#include <iostream>
#include <stdexcept>
#include <functional>
#include <limits>
#include <cstring>
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
struct FakePort : machine::IMotorPort {
    std::array<machine::AxisFeedback,9> feedback{};
    std::vector<int> moves;
    std::array<double,9> targets{};
    int stops=0,failRead=0,failProfile=0,failMove=0,failEnable=0,failLimit=0;
    bool failOpen=false;
    FakePort(){for(auto& f:feedback){f.done=true;f.enabled=true;}}
    bool Open(const std::vector<machine::AxisConfig>&) override{return !failOpen;}
    bool Read(int id,machine::AxisFeedback& f) override{f=feedback[id-1];return failRead!=id;}
    bool Limit(int id,double,double) override{return id!=failLimit;}
    bool Enable(int id) override{return id!=failEnable;}
    bool Profile(int id,double v,double a) override{CHECK(v>0 && a>0);return id!=failProfile;}
    bool Move(int id,double x) override {moves.push_back(id);targets[id-1]=x;feedback[id-1].done=false;return id!=failMove;}
    bool StopAndClose() override{++stops;return true;}
    void Finish(){for(size_t i=0;i<9;++i){feedback[i].position=targets[i];feedback[i].done=true;}}
};
std::array<machine::AxisConfig,9> limits(){std::array<machine::AxisConfig,9> a{};for(int i=0;i<9;++i)a[i]={i+1,-10000,10000};return a;}
machine::LinkSettings settings(){machine::LinkSettings c;c.feedSegment=10;c.feedVelocity=50;c.feedAcceleration=100;return c;}
machine::FeedConfig feedCfg(){machine::FeedConfig f;f.confirmed=true;f.negative=-10000;f.positive=10000;f.velocity=50;f.acceleration=100;f.step=10;f.tolerance=0.25;return f;}
std::vector<machine::AxisConfig> robot(){auto a=limits();return {a.begin(),a.begin()+8};}
std::string cmd(feedwire::Core& c,const std::string& text,uint32_t now=0){
    char input[128]={},reply[96]={};CHECK(text.size()<sizeof(input));
    memcpy(input,text.c_str(),text.size()); c.command(input,now,reply,sizeof(reply));return reply;
}
void boot(feedwire::Core& c,uint32_t now=0){CHECK(cmd(c,"1 H",now).find(" OK ")!=std::string::npos);CHECK(cmd(c,"2 C -10000 10000 200 400",now).find(" OK ")!=std::string::npos);CHECK(cmd(c,"3 E",now).find(" OK ")!=std::string::npos);}
int main(){int passed=0;auto test=[&](const char* name,const std::function<void()>& f){f();++passed;std::cout<<"PASS "<<name<<'\n';};
try {
    test("three-section axis and sign mapping",[]{auto d=machine::BendDeltas(10,20,30,40,5);CHECK((d==std::array<double,9>{{10,-10,20,-20,30,-30,40,-40,5}}));});
    test("unconfirmed configuration never opens",[]{FakePort p;machine::NineAxisController n(p);auto f=feedCfg();f.confirmed=false;CHECK(!n.Connect(f,robot()));CHECK(p.stops==0);});
    test("startup enable rollback",[]{FakePort p;p.failEnable=6;machine::NineAxisController n(p);CHECK(!n.Connect(feedCfg(),robot()));CHECK(p.stops==1&&!n.Ready());});
    test("startup limit rollback before movement",[]{FakePort p;p.failLimit=9;machine::NineAxisController n(p);CHECK(!n.Connect(feedCfg(),robot()));CHECK(p.stops==1&&p.moves.empty());});
    test("feed moves last and all axes participate",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(100,100,100,100,10),limits(),settings(),0));CHECK(p.moves.size()==9&&p.moves.back()==9);});
    test("feed limited segments",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(400,0,0,0,25),limits(),settings(),0));CHECK(m.Count()==3);CHECK(p.targets[8]==8);});
    test("barrier blocks next segment while one axis lags",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(400,0,0,0,25),limits(),settings(),0));p.Finish();p.feedback[7].done=false;auto count=p.moves.size();CHECK(m.Tick(1000));CHECK(p.moves.size()==count&&m.Segment()==1);p.feedback[7].done=true;CHECK(m.Tick(1001));CHECK(m.Segment()==2);});
    test("stale done cannot bypass position check",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(100,0,0,0,10),limits(),settings(),0));for(auto& f:p.feedback)f.done=true;CHECK(m.Tick(1000));CHECK(m.Active());});
    test("duration guard blocks old completion",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(100,0,0,0,10),limits(),settings(),0));p.Finish();CHECK(m.Tick(1));CHECK(m.Active());CHECK(m.Tick(1000));CHECK(!m.Active());});
    test("target rejection sends no moves",[]{FakePort p;machine::LinkedMotion m(p);CHECK(!m.Start(machine::BendDeltas(20000,0,0,0,10),limits(),settings(),0));CHECK(p.moves.empty());});
    test("NaN rejected",[]{FakePort p;machine::LinkedMotion m(p);CHECK(!m.Start(machine::BendDeltas(NAN,0,0,0,10),limits(),settings(),0));CHECK(p.moves.empty());});
    test("timeout stops every transport",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(100,0,0,0,10),limits(),settings(),0));CHECK(!m.Tick(15001));CHECK(p.stops==1&&!m.Active());});
    test("profile failure prevents all starts",[]{FakePort p;p.failProfile=6;machine::LinkedMotion m(p);CHECK(!m.Start(machine::BendDeltas(100,100,100,100,10),limits(),settings(),0));CHECK(p.moves.empty()&&p.stops==1);});
    test("partial robot dispatch failure prevents feed",[]{FakePort p;p.failMove=4;machine::LinkedMotion m(p);CHECK(!m.Start(machine::BendDeltas(100,100,100,100,10),limits(),settings(),0));CHECK(p.moves.back()==4&&p.stops==1);});
    test("feed dispatch failure stops robot",[]{FakePort p;p.failMove=9;machine::LinkedMotion m(p);CHECK(!m.Start(machine::BendDeltas(100,100,100,100,10),limits(),settings(),0));CHECK(p.stops==1);});
    test("runtime robot fault stops linked move",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(100,0,0,0,10),limits(),settings(),0));p.feedback[4].fault=true;CHECK(!m.Tick(100));CHECK(p.stops==1);});
    test("runtime feed link loss stops robot",[]{FakePort p;machine::LinkedMotion m(p);CHECK(m.Start(machine::BendDeltas(100,0,0,0,10),limits(),settings(),0));p.failRead=9;CHECK(!m.Tick(100));CHECK(p.stops==1);});
    test("full positive and negative linked endpoints",[]{for(double sign:{-1.,1.}){FakePort p;machine::LinkedMotion m(p);auto d=machine::BendDeltas(sign*400,sign*100,sign*300,sign*50,sign*25);CHECK(m.Start(d,limits(),settings(),0));uint64_t now=0;for(int n=0;n<10&&m.Active();++n){p.Finish();now+=2000;CHECK(m.Tick(now));}CHECK(!m.Active());CHECK(p.targets==d);}});
    test("CRC corruption and truncation",[]{char frame[128];CHECK(feedwire::encode("4 M 100 2000",frame,sizeof(frame)));frame[strlen(frame)-1]=0;CHECK(feedwire::decode(frame));CHECK(feedwire::encode("4 M 100 2000",frame,sizeof(frame)));frame[strlen(frame)-1]=0;frame[4]='9';CHECK(!feedwire::decode(frame));char shortFrame[]="1 S*0";CHECK(!feedwire::decode(shortFrame));});
    test("unconfirmed wiring prevents enable and pulses",[]{feedwire::Core c(false);cmd(c,"1 H");CHECK(cmd(c,"2 C -100 100 200 400").find(" ERR ")!=std::string::npos);cmd(c,"3 E");CHECK(!c.enabled);CHECK(c.tick(100,100000)==0);});
    test("sequence replay latches stop",[]{feedwire::Core c(true);boot(c);cmd(c,"3 E");CHECK(c.fault&&!c.enabled);});
    test("integer overflow and trailing tokens rejected",[]{for(const char* bad:{"4 M 99999999999999999 1000","4 M 10 1000 garbage","4 M nan 1000","4 C -1 1 200 400","4 M 10 1000 a b c d e"}){feedwire::Core c(true);boot(c);CHECK(cmd(c,bad).find(" ERR ")!=std::string::npos);CHECK(c.fault&&!c.enabled);}});
    test("overspeed and overacceleration rejected",[]{for(const char* bad:{"4 M 1000 250","4 M 20 250"}){feedwire::Core c(true);boot(c);CHECK(cmd(c,bad).find(" ERR ")!=std::string::npos);CHECK(c.fault);}});
    test("firmware soft limits enforced",[]{feedwire::Core c(true);boot(c);CHECK(cmd(c,"4 M 10001 600000").find(" ERR ")!=std::string::npos);});
    test("host watchdog stops pulses within configured interval",[]{feedwire::Core c(true);boot(c);cmd(c,"4 M 100 2000");CHECK(c.tick(751,751000)==0);CHECK(c.fault&&!c.enabled&&!c.moving);});
    test("stop command cuts pulse generation",[]{feedwire::Core c(true);boot(c);cmd(c,"4 M 100 2000");cmd(c,"5 X",100);CHECK(c.tick(101,101000)==0&&!c.enabled&&!c.moving);});
    test("timed pulse generation reaches signed target without bursts",[]{for(int sign:{-1,1}){feedwire::Core c(true);boot(c);cmd(c,"4 M "+std::to_string(sign*100)+" 2000");int seq=5,pulses=0;uint32_t last=0;for(uint32_t us=0;us<2500000;us+=100){if(us%100000==0)cmd(c,std::to_string(seq++)+" S",us/1000);int pulse=c.tick(us/1000,us);if(pulse){CHECK(pulse==sign);if(last)CHECK(us-last>=5000);last=us;++pulses;}}CHECK(pulses==100&&c.position==sign*100&&!c.moving&&!c.fault);}});
    test("watchdog tolerates unsigned millis rollover",[]{feedwire::Core c(true);const uint32_t start=0xffffff00UL;boot(c,start);cmd(c,"4 M 100 2000",start);c.tick(start+200,start*1000U+200000U);CHECK(!c.fault);c.tick(start+751,start*1000U+751000U);CHECK(c.fault);});
    test("bad frame clears motion and requires reconfiguration",[]{feedwire::Core c(true);boot(c);c.badFrame();CHECK(c.fault&&!c.enabled);CHECK(cmd(c,"4 E").find(" ERR ")!=std::string::npos);});
    test("planner lag faults instead of unlimited catch-up",[]{feedwire::Core c(true);boot(c);cmd(c,"4 M 100 2000");cmd(c,"5 S",3100);CHECK(c.tick(3100,3100000)==0);CHECK(c.fault&&!c.moving);});
    std::cout<<"All "<<passed<<" test groups passed. No hardware accessed.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<passed<<" groups: "<<e.what()<<'\n';return 1;}}
