#pragma once

#include <kernel/querydata/idl/querydata_common.pb.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <utility>

namespace NQueryData {
    class TSourceFactors;
    class TFileDescription;

    using TKeyAndType = std::pair<TString, EKeyType>;
    using TKeyAndTypeVec = TVector<TKeyAndType>;
    using TKeyViewAndType = std::pair<TStringBuf, EKeyType>;
    using TKeyViewAndTypeVec = TVector<TKeyViewAndType>;
    using TKeyTypeVec = TVector<EKeyType>;

    void GetAllKeys(TKeyAndTypeVec&, const TSourceFactors& sf);
    TKeyAndTypeVec GetAllKeys(const TSourceFactors& sf);

    void GetAllKeyViews(TKeyViewAndTypeVec&, const TSourceFactors& sf);
    TKeyViewAndTypeVec GetAllKeyViews(const TSourceFactors& sf);

    void GetAllKeyTypes(TKeyTypeVec&, const TSourceFactors& sf);
    TKeyTypeVec GetAllKeyTypes(const TSourceFactors& sf);
    TKeyTypeVec GetAllKeyTypes(const TFileDescription& fd);

}
