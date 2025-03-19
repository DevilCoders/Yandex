#pragma once

#include <util/system/types.h>

using TRelevance = i64;
using TUnsignedRelevance = ui64;

static inline TRelevance GetMaxRelevance() {
    return LL(0x7fff'ffff'ffff'ffff);
}

constexpr TRelevance RELEVANCE_SCALE = 10'000'000;
constexpr TRelevance RELEVANCE_OFFSET = 100'000'000;
constexpr TRelevance MAX_RELEVANCE_SCORE = 1 << 29;


constexpr TRelevance USUAL_RELEVANCE = 100'000'000;
constexpr TRelevance REFINE_RELEVANCE = 200'000'000;
constexpr TRelevance NAVIGATE_RELEVANCE = 300'000'000;
constexpr TRelevance STRONG_NAVIGATE_RELEVANCE = 400'000'000;

#define PHRASE_PRIORITY    5
#define STRONG_PRIORITY    2
#define NO_STRONG_PRIORITY 0

inline ui32 CalcExternalPriority(ui32 prior) {
    return (prior < ui32(STRONG_PRIORITY)) ? 0 : ((prior < PHRASE_PRIORITY) ? 1 : 2);
}

template <class T>
inline TRelevance CheckRelevanceScoreOverflow(T relev) {
    return relev > MAX_RELEVANCE_SCORE ? TRelevance(MAX_RELEVANCE_SCORE) : TRelevance(relev);
}

using TPriority = ui8;

constexpr int PRIORITY_LEVELS = 8;

class TSortable {
public:
    explicit TSortable(TRelevance relev = -1, TPriority prior = 0)
        : Relevance_(relev)
        , Priority_(prior)
    {
    }

    TRelevance GetRelevance() const noexcept {
        return Relevance_;
    }
    void SetRelevance(TRelevance relevance) noexcept{
        Relevance_ = relevance;
    }
    void IncRelevance(TRelevance diff) noexcept {
        Relevance_ += diff;
    }
    TPriority GetPriority() const noexcept {
        return Priority_;
    }
    void SetPriority(TPriority priority) noexcept {
        Priority_ = priority;
    }

private:
    TRelevance Relevance_ = -1;
    TPriority Priority_ = 0;
};
using TRelevPredict = float;
