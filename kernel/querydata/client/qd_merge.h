#pragma once

#include "qd_key.h"

#include <util/generic/deque.h>
#include <util/generic/vector.h>

namespace NQueryData {

    class TQueryData;
    class TSourceFactors;

    void MergeQueryDataSimple(TDeque<NQueryData::TSourceFactors>&, TDeque<NQueryData::TSourceFactors>&);

    void MergeQueryDataSimple(TQueryData& res, const TVector<TQueryData>& qd);

    void MergeQueryDataSimple(TQueryData& res, TVector<TQueryData>&& qd);

    void MergeQueryDataSimple(TQueryData& res, TDeque<TQueryData>&& qd);

    using TMergeMethod = std::function<void (TSourceFactors& , const TSourceFactors& )>;

    void MergeQueryDataCustom(TQueryData& result, TDeque<TQueryData>&& input, TMergeMethod&& doMerge);
    void MergeQueryDataCustom(TQueryData& result, const TVector<const TQueryData*>& input, const TMergeMethod& doMerge);

    TQueryData MergeQueryDataSimple(const TVector<TQueryData>& qd);

    void MergeMethodSimple(TSourceFactors& to, const TSourceFactors& from);
}
