#pragma once

#include "qd_key.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/memory/pool.h>

namespace NQueryData {

    struct TFactorsVec : public TVector<const TFactor*> {
        const TString* Json = nullptr;
        ui64 Timestamp = 0;

        bool GetJson(TStringBuf& sc) const {
            if (Json) {
                sc = *Json;
                return true;
            }

            return false;
        }
    };


    using TFactors = THashMap<TKey, TFactorsVec, THash<TKey>, TEqualTo<TKey>, TPoolAllocator>;
    using TKeyFactors = THashMap<TStringBuf, TSimpleSharedPtr<TFactors>, THash<TStringBuf>, TEqualTo<TStringBuf>, TPoolAllocator>;
    using TCommonFactors = THashMap<TStringBuf, TFactorsVec, THash<TStringBuf>, TEqualTo<TStringBuf>, TPoolAllocator> ;

    const TFactor* FindFactor(const TFactorsVec& fvec, TStringBuf name);

    TKey MakeKey(const TSourceFactors& src);

}
