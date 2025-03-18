#pragma once

#include <util/stream/output.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/system/mutex.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NAapi {

class TProgress: public TThrRefBase {
public:
    inline TProgress()
        : Total(0)
        , Cur(0)
    {
    }

    inline void IncTotal(size_t n = 1) {
        AtomicAdd(Total, n);
    }

    inline void IncCur(size_t n = 1) {
        AtomicAdd(Cur, n);
    }

    inline float Progress() const {
        if (AtomicGet(Total)) {
            return static_cast<float>(AtomicGet(Cur)) / AtomicGet(Total) * 100.0f;
        }
        return 0.0f;
    }

    inline void Display(const TStringBuf& message, IOutputStream& out = Cout) const {
        with_lock(DisplayMutex) {
            out << ProgressString() << " " << message << "\n";
        }
    }

private:
    TAtomic Total;
    TAtomic Cur;
    mutable TMutex DisplayMutex;

    inline TString ProgressString() const {
        return TStringBuilder{} << "[" << FloatToString(Progress(), PREC_POINT_DIGITS, 1) << "%]";
    }
};

using TProgressPtr = TIntrusivePtr<TProgress>;

}  // namespace NAapi
