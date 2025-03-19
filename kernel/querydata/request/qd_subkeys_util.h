#pragma once

#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/vector.h>

namespace NQueryData {

    template <class TStr, class TStr2>
    static void EnsureHierarchy(TVector<TStr>& subkeys, const TStr2& deflt) {
        if (subkeys.empty() || subkeys.back() != deflt) {
            subkeys.push_back(deflt);
        }
    }

    template <class TStr, class TStr2>
    static void AssignNotEmpty(TVector<TStr>& subkeys, const TStr2& item) {
        subkeys.clear();
        if (item) {
            subkeys.emplace_back(item);
        }
    }

    template <class TStr, class TStr2>
    static void AssignNotEmpty(TVector<TStr>& subkeys, const TVector<TStr2>& items) {
        subkeys.clear();
        for (const auto& item : items) {
            if (item) {
                subkeys.emplace_back(item);
            }
        }
    }

    template <class TStr, class TStr2>
    static void FillSerpDeviceTypeHierarchy(TVector<TStr>& subkeys, const TStr2& serpType) {
        AssignNotEmpty(subkeys, serpType);
        if (IsMobileSerpType(serpType)) {
            EnsureHierarchy(subkeys, SERP_TYPE_MOBILE);
        }
        EnsureHierarchy(subkeys, GENERIC_ANY);
    }

    template <class TStr, class TStr2>
    static void FillUserIpOperatorTypeHierarchy(TVector<TStr>& subkeys, const TStr2& mobop) {
        subkeys.clear();
        if (mobop && "0" != mobop) {
            subkeys.push_back(MOBILE_IP_ANY);
        }
        EnsureHierarchy(subkeys, GENERIC_ANY);
    }

}
