#pragma once

#include "reqbundle.h"
#include "hashed_sequence.h"
#include "hashed_reqbundle.h"

namespace NReqBundle {
    // By default doesn't check for repeated request schemas.
    // E.g. merge([("a b c", "ExpType0")], [("a b c", "ExpType0")])
    //          == [("a b c", "ExpType0"), ("a b c", "ExpType0")]
    //
    // For TExpansion::OriginalRequest take only its first occurrence.
    //
    class TReqBundleMerger {
    public:
        struct TOptions {
            bool HashRequests = false;
            bool NeedAllData = false;
            const NDetail::IDuplicatesResolver* DuplicatesResolver = nullptr;
            TOptions() = default;
        };

    public:
        TReqBundleMerger();
        TReqBundleMerger(const TOptions& options);

        void AddBundle(TConstReqBundleAcc bundle);

        void AddRequestsConstraints(TConstReqBundleAcc bundle);

        template <typename IterType>
        void AddBundles(IterType begin, IterType end) {
            for (IterType iter = begin; iter != end; ++iter) {
                AddBundle(*iter);
            }
        }
        void AddBundles(std::initializer_list<TConstReqBundleAcc> bundles) {
            AddBundles(bundles.begin(), bundles.end());
        }

        TReqBundlePtr GetResult() const;

    private:
        TOptions Options;
        TReqBundlePtr MergedBundle;
        NDetail::THashedSequenceAdapter Seq;
        THolder<NDetail::THashedReqBundleAdapter> HashedBundle;
        TSet<TFacetId> OriginalFacets;
        TVector<bool> SeqElemHasConstraint;
    };

    template <typename IterType>
    inline TReqBundlePtr MergeBundles(
        IterType begin, IterType end,
        const TReqBundleMerger::TOptions& options = TReqBundleMerger::TOptions{})
    {
        TReqBundleMerger merger(options);
        merger.AddBundles(begin, end);
        return merger.GetResult();
    }

    inline TReqBundlePtr MergeBundles(
        std::initializer_list<TConstReqBundleAcc> bundles,
        const TReqBundleMerger::TOptions& options = TReqBundleMerger::TOptions{})
    {
        return MergeBundles(bundles.begin(), bundles.end(), options);
    }
} // NReqBundle
