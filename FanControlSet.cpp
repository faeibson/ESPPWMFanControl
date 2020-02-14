
#include "FanControlSet.h"

FanControlSet::FanControlSet(const uint8_t t_fanCount) : fanCount(t_fanCount) {
    fanSpeeds = new uint8_t[fanCount];
}

FanControlSet::~FanControlSet() {
    delete [] fanSpeeds;
}

bool FanControlSet::compareAscending(const FanControlSet &a, const FanControlSet &b) {
    return a.tempThreshold < b.tempThreshold;
}
    
bool FanControlSet::compareDescending(const FanControlSet &a, const FanControlSet &b) {
    return a.tempThreshold > b.tempThreshold;
}
