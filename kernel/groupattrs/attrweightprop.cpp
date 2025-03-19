#include "attrweightprop.h"

#include "docsattrs.h"
#include "metainfo.h"
#include "metainfos.h"

namespace NGroupingAttrs {

TAttrWeightPropagator::TAttrWeightPropagator(bool isThreadSafe)
    : IsThreadSafe(isThreadSafe)
{
}

void TAttrWeightPropagator::Clear() {
    T2WData.Clear();
    Data.Clear();
    Meta = nullptr;
}

void TAttrWeightPropagator::Init(const TMetainfo* meta, const TAttrWeights& t2w, bool reverse) {
    Clear();

    Meta = meta;
    Reverse = reverse;

    if (!t2w) {
        return;
    }

    for (size_t i = 0; i < t2w.size(); ++i) {
        TCateg cat = t2w[i].AttrId;
        T2WData.Set(cat, Max(T2WData.Get(cat, false), t2w[i].Weight), false);
        if (Reverse) {
            Data.Set(cat, t2w[i].Weight, false);
        }
    }

    if (!meta) {
        return;
    }

    if (Reverse) {
        // propagate weights up
        for (size_t i = 0; i < t2w.size(); ++i) {
            float weight = t2w[i].Weight;
            TCateg cat = meta->Categ2Parent(t2w[i].AttrId);
            while (cat != END_CATEG && weight > Data.Get(cat, false)) {
                Data.Set(cat, weight, false);
                cat = meta->Categ2Parent(cat);
            }
        }
    }
}

void TAttrWeightPropagator::Init(const TDocsAttrs& docsAttrs, const TString& attrName, const TAttrWeights& t2w, bool reverse) {
    const TMetainfo* attr = docsAttrs.Metainfos().Metainfo(attrName.data());
    return Init(attr, t2w, reverse);
}

void TAttrWeightPropagator::Init(const TDocsAttrs& docsAttrs, const TString& attrName, TCateg attrId, float value, bool reverse) {
    if (attrId == END_CATEG) {
        Reverse = reverse;
        Clear();
    } else {
        const TMetainfo* meta = docsAttrs.Metainfos().Metainfo(attrName.data());
        Init(meta, attrId, value, reverse);
    }
}

void TAttrWeightPropagator::Init(const TMetainfo* meta, TCateg attrId, float value, bool reverse) {
    if (attrId == END_CATEG) {
        Reverse = reverse;
        Clear();
    } else {
        TAttrWeights t2w(1, TAttrWeight(attrId, value));
        Init(meta, t2w, reverse);
    }
}

float TAttrWeightPropagator::Get(TCateg categ) const {
    if (Reverse) {
        return Data.Get(categ, IsThreadSafe);
    }

    float res = 0.f;
    if (Data.Has(categ, &res, IsThreadSafe)) {
        return res;
    }

    res = T2WData.Get(categ, false);
    if (Meta) {
        TCateg parent = Meta->Categ2Parent(categ);
        if (parent != END_CATEG) {
            res = Max(Get(parent), res);
        }
    }
    Data.Set(categ, res, IsThreadSafe);
    return res;
}

// Returns max Get(c) for c in interval[beg, end).
float TAttrWeightPropagator::GetMax(const TCateg *beg, const TCateg *end) const {
    float max = 0.0f;
    for (const TCateg *toCateg = beg; toCateg != end; ++toCateg) {
        max = Max(Get(*toCateg), max);
    }
    return max;
}

// Returns true if Init() was called with an empty vector of weights.
bool TAttrWeightPropagator::IsEmpty() const {
    return T2WData.IsEmpty();
}


TAttrWeightPropagator::TData::TData() {
}

namespace {
    // Used in TGuard<TSpinLock> that can be disabled
    template <class T>
    struct TSafeLockOps {
        static inline void Acquire(T* t) noexcept {
            if (t) {
                t->Acquire();
            }
        }

        static inline void Release(T* t) noexcept {
            if (t) {
                t->Release();
            }
        }
    };
}

bool TAttrWeightPropagator::TData::Has(TCateg id, float* res, bool isReentrant) const {
    TGuard< TSpinLock, TSafeLockOps<TSpinLock> > g(isReentrant ? &SpinLock : (TSpinLock *)nullptr);
    if (id > MaxId) {
        return false;
    }
    if (id >= 0 && id < SMALL_BOUND) {
        if (-1.f != SmallWeights[id]) {
            *res = SmallWeights[id];
            return true;
        }
        return false;
    }
    TWeights::const_iterator toWeight = Weights.find(id);
    if (toWeight != Weights.end()) {
        *res = toWeight->second;
        return true;
    }
    return false;
}

float TAttrWeightPropagator::TData::Get(TCateg id, bool isReentrant) const {
    float res = 0.f;
    if (Has(id, &res, isReentrant)) {
        return res;
    }
    return 0.f;
}

void TAttrWeightPropagator::TData::Set(TCateg id, float value, bool isReentrant) {
    TGuard< TSpinLock, TSafeLockOps<TSpinLock> > g(isReentrant ? &SpinLock : (TSpinLock *)nullptr);
    if (id >= 0 && id < SMALL_BOUND) {
        SmallWeights[id] = value;
    } else {
        Weights[id] = value;
    }

    MaxId = Max(MaxId, id);
}

void TAttrWeightPropagator::TData::Clear() {
    std::fill_n(SmallWeights, SMALL_BOUND, -1.0f);
    Weights.clear();
    MaxId = -1;
}

bool TAttrWeightPropagator::TData::IsEmpty() const {
    return (-1 == MaxId);
}

}
