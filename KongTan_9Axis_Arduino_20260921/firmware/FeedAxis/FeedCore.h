#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

namespace feedwire {
inline uint16_t crc(const char* s) {
    uint16_t c = 0xffff;
    while (*s) {
        c ^= (uint16_t)(uint8_t)*s++ << 8;
        for (int b=0; b<8; ++b) c = (c & 0x8000) ? (uint16_t)((c<<1)^0x1021) : (uint16_t)(c<<1);
    }
    return c;
}
inline bool encode(const char* payload, char* out, size_t size) {
    const int n = snprintf(out, size, "%s*%04X\n", payload, (unsigned)crc(payload));
    return n > 0 && (size_t)n < size;
}
inline bool decode(char* frame) {
    char* star = strchr(frame, '*');
    if (!star || strlen(star+1) != 4) return false;
    for (char* p=star+1; *p; ++p)
        if (!((*p>='0' && *p<='9') || (*p>='A' && *p<='F'))) return false;
    const unsigned long expected = strtoul(star+1, 0, 16);
    *star = 0;
    return crc(frame) == expected;
}
inline bool integer(const char* s, int32_t& value) {
    if (!s || !*s) return false;
    errno = 0; char* end = 0;
    const long v = strtol(s, &end, 10);
    if (*end || errno == ERANGE || v < -2147483647L || v > 2147483647L) return false;
    value = (int32_t)v; return true;
}
// Hardware-independent protocol and pulse planner, shared by firmware and tests.
class Core {
    int32_t lastSeq_ = 0, low_ = 0, high_ = 0, maxV_ = 0, maxA_ = 0;
    int32_t start_ = 0, target_ = 0;
    uint32_t lastHost_ = 0, startMs_ = 0, duration_ = 0, lastPulseUs_ = 0;
    bool configured_ = false;
public:
    int32_t position = 0;
    bool enabled = false, fault = false, moving = false;
    const bool wiring;
    explicit Core(bool wiringConfirmed) : wiring(wiringConfirmed) {}
    void stop(bool failed) { enabled = moving = configured_ = false; fault = fault || failed; }
    // Reply returns status after executing exactly one checked command.
    void command(char* payload, uint32_t now, char* reply, size_t replySize) {
        char* parts[8] = {}; unsigned n=0;
        char* p = payload;
        while (*p && n<8) {
            while (*p==' ') ++p;
            if (!*p) break;
            parts[n++] = p;
            while (*p && *p!=' ') ++p;
            if (*p) *p++=0;
        }
        while (*p==' ') ++p;
        int32_t seq=0, x=0, y=0, v=0, a=0;
        bool ok = !*p && n>=2 && integer(parts[0], seq) && seq>0;
        const bool hello = ok && n==2 && strcmp(parts[1],"H")==0;
        if (hello) { stop(false); fault=false; position=0; lastSeq_=0; }
        ok = ok && seq>lastSeq_;
        if (ok) {
            lastSeq_=seq;
            if (hello) { /* H always stops; new connection coordinates start here. */ }
            else if (n==2 && strcmp(parts[1],"S")==0) { }
            else if (n==2 && strcmp(parts[1],"X")==0) stop(false);
            else if (n==6 && strcmp(parts[1],"C")==0 && !enabled && !moving && wiring &&
                     integer(parts[2],x) && integer(parts[3],y) && integer(parts[4],v) && integer(parts[5],a) &&
                     x>=-1000000L && y<=1000000L && x<0 && y>0 && v>0 && v<=1000 && a>0 && a<=10000) {
                low_=x; high_=y; maxV_=v; maxA_=a; configured_=true; fault=false;
            }
            else if (n==2 && strcmp(parts[1],"E")==0 && configured_ && !fault && wiring) enabled=true;
            else if (n==4 && strcmp(parts[1],"M")==0 && enabled && !fault && !moving &&
                     integer(parts[2],x) && integer(parts[3],y) && x>=low_ && x<=high_ && y>=250 && y<=600000) {
                const double d = fabs((double)x-position), t = y/1000.0;
                ok = 2*d/t <= maxV_ + 0.0001 && 4*d/(t*t) <= maxA_ + 0.0001;
                if (ok) { start_=position; target_=x; duration_=(uint32_t)y; startMs_=now; lastPulseUs_=now*1000UL; moving=x!=position; }
            }
            else ok=false;
            if (ok) lastHost_=now;
        }
        if (!ok) stop(true); // Malformed, replayed, or disallowed commands latch a stop.
        snprintf(reply, replySize, "%ld %s %ld %d %d %d 1 %d", (long)seq, ok?"OK":"ERR",
                 (long)position, enabled?1:0, fault?1:0, moving?0:1, wiring?1:0);
    }
    void badFrame() { stop(true); }
    // Return one signed pulse, never a burst. Caller emits it immediately.
    int tick(uint32_t nowMs, uint32_t nowUs) {
        if (enabled && (uint32_t)(nowMs-lastHost_)>750) stop(true);
        if (!moving) return 0;
        const uint32_t elapsed=(uint32_t)(nowMs-startMs_);
        if (elapsed>duration_+1000UL) { stop(true); return 0; }
        const double u = elapsed>=duration_ ? 1.0 : (double)elapsed/duration_;
        const double progress = u<0.5 ? 2*u*u : 1-2*(1-u)*(1-u);
        const int32_t total = target_-start_;
        const int32_t wanted = start_ + (int32_t)(fabs((double)total)*progress) * (total<0?-1:1);
        if (wanted==position || (uint32_t)(nowUs-lastPulseUs_) < (1000000UL+(uint32_t)maxV_-1)/(uint32_t)maxV_) return 0;
        const int direction=total<0?-1:1;
        if (position+direction<low_ || position+direction>high_) { stop(true); return 0; }
        position+=direction; lastPulseUs_=nowUs;
        if (position==target_) moving=false;
        return direction;
    }
};
}
