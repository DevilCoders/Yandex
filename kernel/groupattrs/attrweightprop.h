#pragma once

#include "attrweight.h"

#include <kernel/search_types/search_types.h>

#include <util/generic/noncopyable.h>
#include <util/generic/hash.h>
#include <util/system/spinlock.h>

namespace NGroupingAttrs {

class TDocsAttrs;
class TMetainfo;

class TAttrWeightPropagator : TNonCopyable {
private:
    class TData {
    private:
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

        using TWeights = THashMap<TCateg, float>;
        TWeights Weights;
        static constexpr TCateg SMALL_BOUND = 1000;
        float SmallWeights[SMALL_BOUND];
        TCateg MaxId;
        TSpinLock SpinLock;

    public:
        TData();

        bool Has(TCateg id, float* res, bool isReentrant) const;

        float Get(TCateg id, bool isReentrant) const;

        void Set(TCateg id, float value, bool isReentrant);

        void Clear();

        bool IsEmpty() const;
    };

    mutable TData Data;
    TData T2WData;

    bool Reverse;
    const TMetainfo* Meta;
    bool IsThreadSafe;

public:
    TAttrWeightPropagator(bool isThreadSafe = false);

    // Init() takes as input a tree of categories for attribute attrName, and
    // weights for some of these categories (in t2w), and propagates these
    // weights down or up the tree, depending on the value of flag "reverse".
    //
    // Entries in t2w must have valid, non-negative attrId's. All unspecified
    // weights are assumed to be zero. Init() may be called multiple times.
    void Init(const TDocsAttrs& docsAttrs, const TString& attrName, const TAttrWeights& t2w, bool reverse = false);

    void Init(const TMetainfo* meta, const TAttrWeights&, bool);

    // Same as above, but weight is specified for a single category.
    // attrId may be END_CATEG, in which case the effect is as if you called Init() with empty t2w.
    void Init(const TDocsAttrs& docsAttrs, const TString& attrName, TCateg attrId, float value, bool reverse = false);

    void Init(const TMetainfo* meta, TCateg attrId, float value, bool reverse = false);

    void Clear();

    // Get(c) returns maximum weight of categories which are descendants
    // (if reverse=false) or ancestors (if reverse=true) of category c,
    // including c itself.
    float Get(TCateg categ) const;

    // Returns max Get(c) for c in interval[beg, end).
    float GetMax(const TCateg *beg, const TCateg *end) const;

    // Returns true if Init() was called with an empty vector of weights.
    bool IsEmpty() const;
};

}
