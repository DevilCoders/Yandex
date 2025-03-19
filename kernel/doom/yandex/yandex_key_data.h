#pragma once

#include <util/generic/fwd.h>

namespace NDoom {


class TYandexKeyData {
public:
    void SetHitCount(size_t hitCount) {
        HitCount = hitCount;
    }

    void SetStart(ui32 start) {
        Start_ = start;
    }

    ui32 Start() const {
        return Start_;
    }

    void SetEnd(ui32 end) {
        End_ = end;
    }

    ui32 End() const {
        return End_;
    }

    size_t HitCount = 0;

private:
    ui32 Start_ = 0;
    ui32 End_ = 0;
};

struct TYandexHitCountCalcer {
    static size_t HitCount(const TYandexKeyData& data) {
        return data.HitCount;
    }
};


} // namespace NDoom
