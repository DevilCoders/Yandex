#include "qd_client_utils.h"

#include <kernel/querydata/idl/querydata_structs.pb.h>
#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/is_in.h>
#include <util/string/split.h>
#include <util/string/cast.h>
#include <algorithm>

namespace NQueryData {

    template <class Tkt>
    void DoGetAllKeys(Tkt& res, const TSourceFactors& sf) {
        res.clear();
        res.reserve(sf.SourceSubkeysSize() + 1);
        if (sf.HasSourceKeyType()) {
            res.emplace_back(sf.GetSourceKey(), sf.GetSourceKeyType());
        }
        for (const auto& k : sf.GetSourceSubkeys()) {
            res.emplace_back(k.GetKey(), k.GetType());
        }
    }

    void GetAllKeys(TKeyAndTypeVec& res, const TSourceFactors& sf) {
        DoGetAllKeys(res, sf);
    }

    TKeyAndTypeVec GetAllKeys(const TSourceFactors& sf) {
        TKeyAndTypeVec res;
        GetAllKeys(res, sf);
        return res;
    }

    void GetAllKeyViews(TKeyViewAndTypeVec& res, const TSourceFactors& sf) {
        DoGetAllKeys(res, sf);
    }

    TKeyViewAndTypeVec GetAllKeyViews(const TSourceFactors& sf) {
        TKeyViewAndTypeVec res;
        GetAllKeyViews(res, sf);
        return res;
    }

    void GetAllKeyTypes(TKeyTypeVec& res, const TSourceFactors& sf) {
        res.clear();
        res.reserve(sf.SourceSubkeysSize() + 1);
        if (sf.HasSourceKeyType()) {
            res.emplace_back(sf.GetSourceKeyType());
        }
        for (const auto& k : sf.GetSourceSubkeys()) {
            res.emplace_back(k.GetType());
        }
    }

    TKeyTypeVec GetAllKeyTypes(const TSourceFactors& sf) {
        TKeyTypeVec res;
        GetAllKeyTypes(res, sf);
        return res;
    }

    TKeyTypeVec GetAllKeyTypes(const TFileDescription& fd) {
        TKeyTypeVec res;
        if (fd.HasKeyType()) {
            res.emplace_back(fd.GetKeyType());
        }
        for (auto kt : fd.GetSubkeyTypes()) {
            res.emplace_back((EKeyType)kt);
        }
        return res;
    }
}

