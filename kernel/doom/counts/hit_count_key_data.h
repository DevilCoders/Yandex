#pragma once

namespace NDoom {

struct THitCountKeyData {
    size_t HitCount = 0;
};

inline THitCountKeyData operator+(const THitCountKeyData s1, const THitCountKeyData s2) {
    THitCountKeyData result;
    result.HitCount = s1.HitCount + s2.HitCount;
    return result;
}

} //namespace NDoom
