#pragma once

#include "relev_locale.h"

#include <library/cpp/enumbitset/enumbitset.h>

#include <util/generic/vector.h>

namespace NRl {

class TRelevLocaleMask {
    typedef TEnumBitSet<ERelevLocale, ERelevLocale_MIN, (ERelevLocale)ERelevLocale_ARRAYSIZE> TBase;
private:
    TBase Mask; // if some node is 1 then all it's descendants are 1 too.
public:
    TRelevLocaleMask() {
    }

    template <class... Args>
    TRelevLocaleMask(Args... args) {
        Add(args...);
    }

    bool Test(ERelevLocale trg) const {
        return Mask.Test(trg);
    }
    bool Has(ERelevLocale trg) const {
        return Test(trg);
    }

    template <class... Args>
    void Add(ERelevLocale trg, Args... args) {
        Add(trg);
        Add(args...);
    }

    void Add(ERelevLocale trg) {
        ProcessAllChildrenAndSelf(trg, [&](ERelevLocale trg) { this->Mask.Set(trg); });
    }
    void Set(ERelevLocale trg) {
        Add(trg);
    }

    template <class... Args>
    void Exclude(ERelevLocale trg, Args... args) {
        Exclude(trg);
        Exclude(args...);
    }

    void Exclude(ERelevLocale trg) {
        ProcessAllChildrenAndSelf(trg, [&](ERelevLocale trg) {this->Mask.Reset(trg);});
        do {
            trg = GetLocaleParent(trg);
            Mask.Reset(trg);
        } while (trg != RL_UNIVERSE);
    }

    void Clear(ERelevLocale trg) {
        Exclude(trg);
    }

private:
    template <typename TFunc>
    static void ProcessAllChildrenAndSelf(ERelevLocale parent, TFunc&& f) {
        f(parent);
        const TVector<ERelevLocale>& children = GetLocaleChildren(parent);
        for (ERelevLocale child : children) {
            ProcessAllChildrenAndSelf(child, f);
        }
    }
};

}  // namespace NRl
