#pragma once

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>

#include <util/stream/output.h>
#include <util/system/types.h>

namespace NOffroad::NMinHash {

struct TArithmeticProgressionHit {
    ui32 Init = 0;
    ui32 Diff = 0;

    std::tuple<ui32, ui32> AsTuple() const {
        return { Init, Diff };
    }
};

inline bool operator<(TArithmeticProgressionHit l, TArithmeticProgressionHit r) {
    return l.AsTuple() < r.AsTuple();
}

inline IOutputStream& operator<<(IOutputStream& stream, TArithmeticProgressionHit hit) {
    stream << "[" << hit.Init << ", " << hit.Diff << "]";
    return stream;
}

class TArithmeticProgressionVectorizer {
public:
    enum {
        TupleSize = 2
    };

    template <class Slice>
    static void Scatter(TArithmeticProgressionHit hit, Slice&& slice) {
        slice[1] = hit.Diff;
        slice[0] = hit.Init;
    }

    template <class Slice>
    static void Gather(Slice&& slice, TArithmeticProgressionHit* hit) {
        hit->Diff = slice[1];
        hit->Init = slice[0];
    }
};

using TArithmeticProgressionSubtractor = NOffroad::TINSubtractor;

} // namespace NOffroad::NMinHash
