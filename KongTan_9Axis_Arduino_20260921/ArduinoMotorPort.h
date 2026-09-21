#pragma once
#include "CmlMotorPort.h"
#include "firmware/FeedAxis/FeedCore.h"
#include <Windows.h>
#include <sstream>
#include <iomanip>

struct ArduinoConfig {
    std::wstring port;
    bool confirmed=false;
    double stepsPerMm=0, maxMmPerSec=0.5, accelerationMmPerSec2=0.5;
    double negativeMm=0, positiveMm=0;
    int maxPulseRate=200, maxPulseAcceleration=400;
    bool Valid() const {
        if (!confirmed || port.size()<4 || port.substr(0,3)!=L"COM") return false;
        for (size_t i=3;i<port.size();++i) if (port[i]<L'0' || port[i]>L'9') return false;
        return port[3]!=L'0' && std::isfinite(stepsPerMm) && stepsPerMm>0 && stepsPerMm<=1000000 &&
            std::isfinite(maxMmPerSec) && maxMmPerSec>0 && maxMmPerSec<=2 &&
            std::isfinite(accelerationMmPerSec2) && accelerationMmPerSec2>0 && accelerationMmPerSec2<=10 &&
            std::isfinite(negativeMm) && std::isfinite(positiveMm) && negativeMm<0 && positiveMm>0 &&
            negativeMm*stepsPerMm>=-1000000 && positiveMm*stepsPerMm<=1000000 &&
            maxPulseRate>=1 && maxPulseRate<=1000 && maxPulseAcceleration>=1 && maxPulseAcceleration<=10000;
    }
};
class ArduinoMotorPort : public machine::IMotorPort {
    CmlMotorPort robot_;
    HANDLE serial_=INVALID_HANDLE_VALUE;
    long sequence_=0;
    double velocity_=1, acceleration_=1;
    machine::AxisFeedback feed_;
    bool Request(const std::string& command, bool requireWiring=true) {
        if (serial_==INVALID_HANDLE_VALUE || sequence_>=2147483646) return false;
        const long seq=++sequence_;
        const std::string payload=std::to_string(seq)+" "+command;
        char frame[128];
        if (!feedwire::encode(payload.c_str(),frame,sizeof(frame))) return false;
        DWORD sent=0;
        if (!WriteFile(serial_,frame,(DWORD)strlen(frame),&sent,nullptr) || sent!=strlen(frame)) return false;
        char response[128]={}; size_t used=0;
        const ULONGLONG start=GetTickCount64();
        while (GetTickCount64()-start<200) {
            char c=0; DWORD received=0;
            if (!ReadFile(serial_,&c,1,&received,nullptr)) return false;
            if (!received) { Sleep(1); continue; }
            if (c=='\r') continue;
            if (c=='\n') {
                response[used]=0;
                if (!feedwire::decode(response)) return false;
                std::istringstream in(response);
                long actual=0, position=0; std::string status, extra;
                int enabled=0,fault=0,done=0,version=0,wiring=0;
                if (!(in>>actual>>status>>position>>enabled>>fault>>done>>version>>wiring) || (in>>extra) ||
                    actual!=seq || status!="OK" || version!=1 || (requireWiring && wiring!=1) ||
                    position < -1000000 || position > 1000000 ||
                    (enabled!=0 && enabled!=1) || (fault!=0 && fault!=1) || (done!=0 && done!=1)) return false;
                feed_.position=position; feed_.enabled=enabled!=0; feed_.fault=fault!=0; feed_.done=done!=0;
                return true;
            }
            if (used+1>=sizeof(response)) return false;
            response[used++]=c;
        }
        return false; // No motion retry after an uncertain acknowledgement.
    }
public:
    ArduinoConfig config;
    explicit ArduinoMotorPort(CmlMotor& motor) : robot_(motor) {}
    ~ArduinoMotorPort() { if (serial_!=INVALID_HANDLE_VALUE) { Request("X",false); CloseHandle(serial_); } }
    bool Open(const std::vector<machine::AxisConfig>& axes) override {
        if (!config.Valid() || axes.size()!=9 || !StopAndClose()) return false;
        std::vector<machine::AxisConfig> robot;
        for (const auto& a:axes) { if (a.id>=1 && a.id<=8) robot.push_back(a); else if(a.id!=9) return false; }
        if (robot.size()!=8) return false;
        const std::wstring path=L"\\\\.\\"+config.port;
        serial_=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,0,nullptr);
        if (serial_==INVALID_HANDLE_VALUE) return false;
        DCB dcb={}; dcb.DCBlength=sizeof(dcb);
        bool ok=GetCommState(serial_,&dcb)!=FALSE;
        dcb.BaudRate=CBR_115200; dcb.ByteSize=8; dcb.Parity=NOPARITY; dcb.StopBits=ONESTOPBIT;
        dcb.fBinary=TRUE; dcb.fParity=FALSE; dcb.fOutxCtsFlow=FALSE; dcb.fOutxDsrFlow=FALSE;
        dcb.fDtrControl=DTR_CONTROL_ENABLE; dcb.fDsrSensitivity=FALSE; dcb.fOutX=FALSE; dcb.fInX=FALSE;
        dcb.fRtsControl=RTS_CONTROL_DISABLE; dcb.fAbortOnError=FALSE;
        COMMTIMEOUTS timeouts={}; timeouts.ReadIntervalTimeout=MAXDWORD; timeouts.WriteTotalTimeoutConstant=200;
        ok=ok && SetCommState(serial_,&dcb) && SetCommTimeouts(serial_,&timeouts);
        // Opening USB serial can reset a Mega. No axis is enabled while waiting.
        if (ok) { Sleep(2000); ok=PurgeComm(serial_,PURGE_RXCLEAR|PURGE_TXCLEAR)!=FALSE; }
        sequence_=0;
        if (!ok || !Request("H") || !robot_.Open(robot)) { StopAndClose(); return false; }
        return true;
    }
    bool Read(int id, machine::AxisFeedback& f) override {
        if (id!=9) return robot_.Read(id,f);
        if (!Request("S")) return false;
        f=feed_; return true;
    }
    bool Limit(int id,double negative,double positive) override {
        if (id!=9) return robot_.Limit(id,negative,positive);
        if (!std::isfinite(negative) || !std::isfinite(positive) || negative<-1000000 || positive>1000000 || negative>=0 || positive<=0) return false;
        return Request("C "+std::to_string((long)std::ceil(negative))+" "+std::to_string((long)std::floor(positive))+" "+
                       std::to_string(config.maxPulseRate)+" "+std::to_string(config.maxPulseAcceleration));
    }
    bool Enable(int id) override { return id==9 ? Request("E") : robot_.Enable(id); }
    bool Profile(int id,double v,double a) override {
        if (id!=9) return robot_.Profile(id,v,a);
        if (!std::isfinite(v) || !std::isfinite(a) || v<=0 || a<=0 ||
            v>config.maxPulseRate+0.0001 || a>config.maxPulseAcceleration+0.0001 ||
            v>config.stepsPerMm*config.maxMmPerSec+0.0001 || a>config.stepsPerMm*config.accelerationMmPerSec2+0.0001) return false;
        velocity_=v; acceleration_=a; return true;
    }
    bool Move(int id,double target) override {
        if (id!=9) return robot_.Move(id,target);
        if (!std::isfinite(target) || target<-1000000 || target>1000000 || target!=std::round(target) || !Request("S") || !feed_.enabled || feed_.fault || !feed_.done) return false;
        const double distance=std::abs(target-feed_.position);
        const double seconds=(std::max)(0.25,(std::max)(2*distance/velocity_,std::sqrt(4*distance/acceleration_)));
        const double ms=std::ceil(seconds*1000);
        if (ms>600000) return false;
        return Request("M "+std::to_string((long)target)+" "+std::to_string((long)ms));
    }
    bool StopAndClose() override {
        bool feedOk=true;
        if (serial_!=INVALID_HANDLE_VALUE) {
            feedOk=Request("X",false);
            CloseHandle(serial_); serial_=INVALID_HANDLE_VALUE;
        }
        const bool robotOk=robot_.StopAndClose();
        return feedOk && robotOk;
    }
};
