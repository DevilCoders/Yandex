#pragma once

#include <kernel/iznanka/rtmr_factors/indices.h>
#include <kernel/iznanka/rtmr_factors/indices.h_serialized.h>
#include <kernel/iznanka/rtmr_factors/protos/messages.pb.h>

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/string/join.h>

namespace NIznanka {

class TRtmrCountersHandler {
private:
    struct TDiscountIndexes {
        float Discount;
        TTripleIndex Indexes;
        TDiscountIndexes(float discount, ERtmrFactorIndex user, ERtmrFactorIndex reqType, ERtmrFactorIndex url);
    };

    static const TVector<TDiscountIndexes> DISCOUNT_INDEXES;
    static const THashMap<ENavigPath, TTripleIndex> NAVIG_PATH_INDEXES;
    static const THashMap<ENavigAction, THashMap<ENavigPath, THashMap<ENavigPath, std::pair<TTripleIndex, TTripleIndex>>>> NAVIG_ACTION_INDEXES;

    static const THashMap<EFixedReqType, ERequestType> FIXED_REQ_TYPE_MAP;

    static void ExtractKeyFactors(const NProtoBuf::Map<NProtoBuf::string, TIznankaActionPathCounters>& actionPathCounters,
                                    EFactorType type, i64 curTimestamp, TVector<float>& factors);
public:
    static size_t GetNumDiscounts() {
        return DISCOUNT_INDEXES.size();
    }

    static float Increment(float counter, float discount, i64 delta, float value = 1.f);

    static void IncrementTimestamp(TIznankaActionPathCounters& actionCounter, i64 curTimestamp,
                                    float curDwelltime, float curDwelltimePrev, float value = 1.f);

    static void IncrementTimestamps(TIznankaActionPathCounters& actionCounter, const TVector<i64>& timestamps,
                                        const TVector<float>& dwelltimes, const TVector<float>& dwelltimesPrev, float value = 1.f);

    static void ExtractFactors(const TIznankaPersonalCounters& personalCounters, const TString& sourceUrl, ERequestType reqType, i64 curTimestamp, TVector<float>& factors);
};

}   // namespace NIznanka
