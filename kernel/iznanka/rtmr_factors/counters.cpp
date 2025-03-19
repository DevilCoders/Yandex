#include "counters.h"

#include <extsearch/geo/kernel/factor_norm/normalize.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>

namespace NIznanka {

using ERfi = ERtmrFactorIndex;
using TRch = NIznanka::TRtmrCountersHandler;
using TTripleIndexPair = std::pair<TTripleIndex, TTripleIndex>;
using TDstSrcToIndexesMap = THashMap<ENavigPath, THashMap<ENavigPath, TTripleIndexPair>>;

static constexpr float DWELLTIME_NORMALIZER = 100.f;
static constexpr float USER_URL_FACTORS_NORMALIZER = 10.f;
static constexpr float USER_FACTORS_NORMALIZER = 100.f;
static constexpr float FIFTEEN_MINUTES = 900.f;
static constexpr float DWELLTIME_DISCOUNT = 0.99999f;

const TVector<TRch::TDiscountIndexes> TRch::DISCOUNT_INDEXES = {
    {0.7f, ERfi::UserClick10Seconds, ERfi::UserReqTypeClick10Seconds, ERfi::UserUrlClick10Seconds},
    {0.95f, ERfi::UserClickMinute, ERfi::UserReqTypeClickMinute, ERfi::UserUrlClickMinute},
    {0.995f, ERfi::UserClick10Minutes, ERfi::UserReqTypeClick10Minutes, ERfi::UserUrlClick10Minutes},
    {0.999f, ERfi::UserClickHour, ERfi::UserReqTypeClickHour, ERfi::UserUrlClickHour},
    {0.999998f, ERfi::UserClickMonth, ERfi::UserReqTypeClickMonth, ERfi::UserUrlClickMonth},
};

const THashMap<ENavigPath, TTripleIndex> TRch::NAVIG_PATH_INDEXES = {
    {ENavigPath::Empty, {ERfi::UserNothing, ERfi::UserUrlNothing, ERfi::UserUrlNothing}},
    {ENavigPath::Nothing, {ERfi::UserNothing, ERfi::UserReqTypeNothing, ERfi::UserUrlNothing}},
    {ENavigPath::Line, {ERfi::UserLine, ERfi::UserReqTypeLine, ERfi::UserUrlLine}},
    {ENavigPath::Teaser, {ERfi::UserTeaser, ERfi::UserReqTypeTeaser, ERfi::UserUrlTeaser}},
    {ENavigPath::Opened, {ERfi::UserOpened, ERfi::UserReqTypeOpened, ERfi::UserUrlOpened}},
    {ENavigPath::Maximized, {ERfi::UserMaximized, ERfi::UserReqTypeMaximized, ERfi::UserUrlMaximized}}
};

const TTripleIndexPair REDUCE_TO_TEASER_INNER_INDEXES = {
    {ERfi::UserReduceToTeaser, ERfi::UserReqTypeReduceToTeaser, ERfi::UserUrlReduceToTeaser},
    {ERfi::UserReduceToTeaserDwelltimePrev, ERfi::UserReqTypeReduceToTeaserDwelltimePrev, ERfi::UserUrlReduceToTeaserDwelltimePrev}
};

const TTripleIndexPair REDUCE_TO_LINE_INNER_INDEXES = {
    {ERfi::UserReduceToLine, ERfi::UserReqTypeReduceToLine, ERfi::UserUrlReduceToLine},
    {ERfi::UserReduceToLineDwelltimePrev, ERfi::UserReqTypeReduceToLineDwelltimePrev, ERfi::UserUrlReduceToLineDwelltimePrev}
};

const TDstSrcToIndexesMap REDUCE_TO_TEASER_INDEXES = {
    {
        ENavigPath::Teaser, {
            {ENavigPath::Opened, REDUCE_TO_TEASER_INNER_INDEXES},
            {ENavigPath::Maximized, REDUCE_TO_TEASER_INNER_INDEXES}
        }
    }
};

const TDstSrcToIndexesMap REDUCE_TO_LINE_INDEXES = {
    {
        ENavigPath::Line, {
            {ENavigPath::Teaser, REDUCE_TO_LINE_INNER_INDEXES},
            {ENavigPath::Opened, REDUCE_TO_LINE_INNER_INDEXES},
            {ENavigPath::Maximized, REDUCE_TO_LINE_INNER_INDEXES}
        }
    }
};

const THashMap<ENavigAction, TDstSrcToIndexesMap> TRch::NAVIG_ACTION_INDEXES = {
    {ENavigAction::ClickBack, REDUCE_TO_TEASER_INDEXES},
    {ENavigAction::ClickOutsideIznanka, REDUCE_TO_TEASER_INDEXES},
    {ENavigAction::SwipeToTeaser, REDUCE_TO_TEASER_INDEXES},
    {ENavigAction::SwipeToLine, REDUCE_TO_LINE_INDEXES},
};

const TVector<ERfi> DWELLTIME_INDEXES = {
    ERfi::UserReduceToTeaserDwelltimePrev, ERfi::UserReqTypeReduceToTeaserDwelltimePrev, ERfi::UserUrlReduceToTeaserDwelltimePrev,
    ERfi::UserReduceToLineDwelltimePrev, ERfi::UserReqTypeReduceToLineDwelltimePrev, ERfi::UserUrlReduceToLineDwelltimePrev
};

const THashMap<EFixedReqType, ERequestType> TRch::FIXED_REQ_TYPE_MAP = {
    {EFixedReqType::Unknown, ERequestType::Unknown},
    {EFixedReqType::Web, ERequestType::Pageload},
    {EFixedReqType::Geo, ERequestType::Pageload},
    {EFixedReqType::Video, ERequestType::Video},
    {EFixedReqType::Sovetnik, ERequestType::Sovetnik}
};

TRch::TDiscountIndexes::TDiscountIndexes(float discount, ERfi user, ERfi reqType, ERfi url)
    : Discount(discount)
    , Indexes(user, reqType, url)
{}

float TRch::Increment(float counter, float discount, i64 delta, float value) {
    float multiplier = Power(discount, Abs(delta));
    if (delta > 0) {
        return counter * multiplier + value;
    } else {
        return counter + multiplier * value;
    }
}

void TRch::IncrementTimestamp(TIznankaActionPathCounters& actionCounter, i64 curTimestamp, float curDwelltime, float curDwelltimePrev, float value) {
    float sumWeightedDwellTimes = actionCounter.GetSumWeightedDwellTimes();
    float sumWeightedDwellTimesPrev = actionCounter.GetSumWeightedDwellTimesPrev();
    float sumWeights = actionCounter.GetSumWeights();

    i64 timestamp = actionCounter.GetTimestamp();
    auto& counters = *actionCounter.MutableCounters();

    if (timestamp == 0) {
        timestamp = curTimestamp;
    }
    i64 delta = curTimestamp - timestamp;
    if (delta > 0) {
        timestamp = curTimestamp;
    }
    for (int i = 0; i < Min<int>(DISCOUNT_INDEXES.size(), counters.size()); ++i) {
        counters[i] = Increment(counters[i], DISCOUNT_INDEXES[i].Discount, delta, value);
    }
    sumWeightedDwellTimes = Increment(sumWeightedDwellTimes, DWELLTIME_DISCOUNT, delta, curDwelltime);
    sumWeightedDwellTimesPrev = Increment(sumWeightedDwellTimesPrev, DWELLTIME_DISCOUNT, delta, curDwelltimePrev);
    sumWeights = Increment(sumWeights, DWELLTIME_DISCOUNT, delta);

    actionCounter.SetSumWeightedDwellTimes(sumWeightedDwellTimes);
    actionCounter.SetSumWeightedDwellTimesPrev(sumWeightedDwellTimesPrev);
    actionCounter.SetSumWeights(sumWeights);
    actionCounter.SetTimestamp(timestamp);
}

void TRch::IncrementTimestamps(TIznankaActionPathCounters& actionCounter, const TVector<i64>& timestamps, const TVector<float>& dwelltimes, const TVector<float>& dwelltimesPrev, float value) {
    for (size_t i = 0; i < timestamps.size(); ++i) {
        IncrementTimestamp(actionCounter, timestamps[i], dwelltimes[i], dwelltimesPrev[i], value);
    }
}

void TRch::ExtractKeyFactors(const NProtoBuf::Map<NProtoBuf::string, TIznankaActionPathCounters>& actionPathCounters, EFactorType type, i64 curTimestamp, TVector<float>& factors) {
    TVector<TString> actionPaths;
    for (auto itAction = actionPathCounters.begin(); itAction != actionPathCounters.end(); ++itAction) {
        actionPaths.emplace_back(itAction->first);
    }
    Sort(actionPaths);
    for (size_t i = 0; i < actionPaths.size(); ++i) {
        TActionPath actionPath;
        Y_PROTOBUF_SUPPRESS_NODISCARD actionPath.ParseFromString(actionPaths[i]);
        bool isClick = actionPath.HasPath();
        bool isNavig = actionPath.HasPathDestination();
        if (isClick || isNavig) {
            auto actionCounters = actionPathCounters.at(actionPaths[i]);
            float deltaTimestamp = curTimestamp - actionCounters.GetTimestamp();
            IncrementTimestamp(actionCounters, curTimestamp, 0.f, 0.f, 0.f);
            const auto& countersArray = actionCounters.GetCounters();

            if (isNavig) {
                float lastCounter = countersArray.empty() ? 0.f : *countersArray.rbegin();
                // Destination path factors
                ENavigPath destination;
                if (TryFromString<ENavigPath>(actionPath.GetPathDestination(), destination)) {
                    if (auto* pathIdxPtr = NAVIG_PATH_INDEXES.FindPtr(destination)) {
                        factors[(ui16) pathIdxPtr->Get(type)] += lastCounter;
                    }
                }

                // Action path factors
                ENavigAction action;
                ENavigPath source;
                if (actionPath.HasAction() && TryFromString<ENavigAction>(actionPath.GetAction(), action) &&
                        actionPath.HasPathSource() && TryFromString<ENavigPath>(actionPath.GetPathSource(), source)) {
                    if (auto* actionIdxPtr = NAVIG_ACTION_INDEXES.FindPtr(action)) {
                        if (auto* dstIdxPtr = actionIdxPtr->FindPtr(destination)) {
                            if (auto* srcIdxPtr = dstIdxPtr->FindPtr(source)) {
                                factors[(ui16) srcIdxPtr->first.Get(type)] += lastCounter;

                                float sumWeightedDwellTimesPrev = actionCounters.GetSumWeightedDwellTimesPrev();
                                float sumWeights = actionCounters.GetSumWeights();
                                if (sumWeights > 0.f) {
                                    ui16 idx = (ui16) srcIdxPtr->second.Get(type);
                                    factors[idx] = Max(sumWeightedDwellTimesPrev / sumWeights, factors[idx]);
                                }
                                if (type == EFactorType::Url) {
                                    ui16 idx = (ui16) ERfi::UserUrlReduceToOldness;
                                    factors[idx] = Max(deltaTimestamp, factors[idx]);
                                }
                            }
                        }
                    }
                }
            }
            if (isClick) {
                for (int i = 0; i < Min<int>(DISCOUNT_INDEXES.size(), countersArray.size()); ++i) {
                    factors[(ui16) DISCOUNT_INDEXES[i].Indexes.Get(type)] += countersArray[i];
                }
            }
        }
    }
}

void TRch::ExtractFactors(const TIznankaPersonalCounters& personalCounters, const TString& sourceUrl, ERequestType reqType, i64 curTimestamp, TVector<float>& factors) {
    factors.resize(GetEnumItemsCount<ERfi>(), 0.f);
    const auto& sourceUrlCountersMap = personalCounters.GetSourceUrlCounters();
    auto itSourceUrl = sourceUrlCountersMap.find(sourceUrl);
    if (itSourceUrl != sourceUrlCountersMap.end()) {
        ExtractKeyFactors(itSourceUrl->second.GetActionPathCounters(), EFactorType::Url, curTimestamp, factors);
    }

    const auto& reqTypeCountersMap = personalCounters.GetReqTypeCounters();
    TVector<TString> reqTypeCounters;
    for (const auto& reqTypeCounter : reqTypeCountersMap) {
        reqTypeCounters.push_back(reqTypeCounter.first);
    }
    Sort(reqTypeCounters);

    for (const auto& reqTypeCounter : reqTypeCounters) {
        const auto& counter = reqTypeCountersMap.at(reqTypeCounter);
        EFixedReqType currentReqType;
        if (TryFromString<EFixedReqType>(reqTypeCounter, currentReqType)) {
            if (FIXED_REQ_TYPE_MAP.Value(currentReqType, ERequestType::Unknown) == reqType) {
                ExtractKeyFactors(counter.GetActionPathCounters(), EFactorType::ReqType, curTimestamp, factors);
            }
        }
        ExtractKeyFactors(counter.GetActionPathCounters(), EFactorType::User, curTimestamp, factors);
    }

    // Normalize factors
    ui16 index = 0;
    for (index = (ui16) ERfi::UserNothing; index < (ui16) ERfi::UserReduceToTeaser; ++index) {
        factors[index] = NGeosearch::NormLog(factors[index], USER_FACTORS_NORMALIZER);
    }
    for (index = (ui16) ERfi::UserReqTypeNothing; index < (ui16) ERfi::UserReqTypeReduceToTeaser; ++index) {
        factors[index] = NGeosearch::NormLog(factors[index], USER_FACTORS_NORMALIZER);
    }
    for (index = (ui16) ERfi::UserUrlNothing; index < (ui16) ERfi::UserUrlReduceToTeaser; ++index) {
        factors[index] = NGeosearch::NormLog(factors[index], USER_URL_FACTORS_NORMALIZER);
    }
    for (const auto& indexERfi: DWELLTIME_INDEXES) {
        index = (ui16) indexERfi;
        factors[index] = NGeosearch::NormLog(factors[index], DWELLTIME_NORMALIZER);
    }
    index = (ui16) ERfi::UserUrlReduceToOldness;
    factors[index] = NGeosearch::NormLog(factors[index], FIFTEEN_MINUTES);
}

}   // namespace NIznanka
