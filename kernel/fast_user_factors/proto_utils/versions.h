#pragma once

#include <util/generic/vector.h>
#include <util/generic/yexception.h>

#include <type_traits>

namespace NFastUserFactors {

    class TCountersProto;
    class TQueryAggregation;
    class TVersionDictionaryProto;

    class TVersionDictionary {
    public:
        void Init(const TVersionDictionaryProto& proto);

        TCountersProto FillNames(const TCountersProto& countersProto) const;
        TQueryAggregation FillNames(const TQueryAggregation& queryAggregation) const;

        TCountersProto FillVersion(const TCountersProto& countersProto, const ui32 versionId) const;

        ui64 GetCountersSize(const i32 versionId) const {
            Y_ENSURE(versionId >= 0);
            Y_ENSURE(static_cast<ui32>(versionId) < Versions.size());
            return Versions.at(versionId).size();
        }

    private:
        template<class TVersionId>
        const TVector<TString>& GetCounterNamesByVersionId(const TVersionId versionId) const {
            if constexpr (std::is_signed<TVersionId>::value) {
                Y_ENSURE(versionId >= 0);
            }
            Y_ENSURE(static_cast<ui32>(versionId) < Versions.size());
            return Versions.at(static_cast<ui32>(versionId));
        }

    private:
        static constexpr ui32 MaxVersionId = 1000UL;
        TVector<TVector<TString>> Versions;
    };

}
