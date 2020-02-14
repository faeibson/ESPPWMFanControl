
#ifndef _FAN_CONTROL_SET_H_
#define _FAN_CONTROL_SET_H_

#include <cstdint>

struct FanControlSet {
    const uint8_t fanCount;
    uint8_t *fanSpeeds;
    float tempThreshold;

    FanControlSet(const uint8_t t_fanCount);
    ~FanControlSet();
    
    static bool compareAscending(const FanControlSet &a, const FanControlSet &b);
    static bool compareDescending(const FanControlSet &a, const FanControlSet &b);
};

#endif
