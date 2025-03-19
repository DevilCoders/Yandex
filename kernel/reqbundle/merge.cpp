#include "merge.h"

#include "blocks_counter.h"

namespace {
    class TRemapContext {
    public:
        TRemapContext(NReqBundle::TConstSequenceAcc srcSeq,
            const NReqBundle::NDetail::TBlocksCounter& blocks,
            NReqBundle::NDetail::THashedSequenceAdapter& dstSeq,
            bool needAllBlocks)
            : Remap(srcSeq.GetNumElems())
        {
            for (size_t elemIndex : xrange(srcSeq.GetNumElems())) {
                if (!needAllBlocks && !blocks[elemIndex]) {
                    continue;
                }
                auto elem = srcSeq.GetElem(elemIndex);
                Remap[elemIndex] = dstSeq.AddElem(elem);
            }
        }

        void RemapRequest(NReqBundle::TRequestAcc request) const {
            Remap(request);
        }

        void RemapConstraint(NReqBundle::TConstraintAcc constraint) const {
            Remap(constraint);
        }

    private:
        NReqBundle::NDetail::TBlocksRemapper Remap;
    };
} // namespace

namespace NReqBundle {
    TReqBundleMerger::TReqBundleMerger()
        : TReqBundleMerger(TOptions{})
    {
    }

    TReqBundleMerger::TReqBundleMerger(const TOptions& options)
        : Options(options)
        , MergedBundle(new TReqBundle)
        , Seq(MergedBundle->Sequence())
    {
        if (Options.HashRequests) {
            NDetail::THashedReqBundleAdapter::TOptions opts;
            opts.DuplicatesResolver = Options.DuplicatesResolver;

            HashedBundle.Reset(new NDetail::THashedReqBundleAdapter(*MergedBundle, opts));
        }
    }

    void TReqBundleMerger::AddBundle(TConstReqBundleAcc bundle)
    {
        NDetail::TBlocksCounter blocks(bundle.GetSequence().GetNumElems());

        for (auto constraint : bundle.GetConstraints()) {
            blocks.Add(constraint);
        }

        TDeque<TRequest*> targetRequests;
        for (auto request : bundle.GetRequests()) {
            THolder<TRequest> targetRequest = MakeHolder<TRequest>(request);
            targetRequest->Facets().Clear();

            bool needTrCompatibilityInfo = false;
            for (auto entry : request.GetFacets().GetEntries()) {
                if (entry.GetId().Get<EFacetPartType::Expansion>() == TExpansion::OriginalRequest
                    && !OriginalFacets.insert(entry.GetId()).second)
                {
                    continue;
                }

                if (entry.NeedTrCompatibilityInfo()) {
                    needTrCompatibilityInfo = true;
                }

                NDetail::BackdoorAccess(targetRequest->Facets()).Entries.push_back(NDetail::BackdoorAccess(entry));
            }

            if (targetRequest->GetFacets().GetNumEntries() > 0) {
                if (!needTrCompatibilityInfo) {
                    targetRequest->ClearTrCompatibilityInfo();
                }
                blocks.Add(*targetRequest);
                targetRequests.push_back(targetRequest.Release());
            }
        }

        if (!Options.NeedAllData && targetRequests.empty()) {
            return;
        }

        TRemapContext ctx(bundle.GetSequence(), blocks, Seq, Options.NeedAllData);

        for (auto request : targetRequests) {
            ctx.RemapRequest(*request);
            if (!!HashedBundle) {
                HashedBundle->AddRequest(request);
            } else {
                MergedBundle->AddRequest(request);
            }
        }

        SeqElemHasConstraint.resize(Seq.GetNumElems(), false);
        for (auto constraint : bundle.GetConstraints()) {
            THolder<TConstraint> newConstraint(new TConstraint(constraint));
            ctx.RemapConstraint(*newConstraint);
            for (const size_t blockIndex : newConstraint->GetBlockIndices()) {
                SeqElemHasConstraint[blockIndex] = true;
            }
            if (!!HashedBundle) {
                HashedBundle->AddConstraint(newConstraint.Release());
            } else {
                MergedBundle->AddConstraint(newConstraint.Release());
            }
        }
    }

    void TReqBundleMerger::AddRequestsConstraints(TConstReqBundleAcc bundle)
    {
        for (auto request : bundle.GetRequests()) {
            THolder<TRequest> targetRequest = MakeHolder<TRequest>(request);
            if (!!HashedBundle) {
                HashedBundle->AddRequest(targetRequest.Release());
            } else {
                MergedBundle->AddRequest(targetRequest.Release());
            }
        }
        for (auto constraint : bundle.GetConstraints()) {
            THolder<TConstraint> newConstraint(new TConstraint(constraint));
            if (!!HashedBundle) {
                HashedBundle->AddConstraint(newConstraint.Release());
            } else {
                MergedBundle->AddConstraint(newConstraint.Release());
            }
        }
    }

    TReqBundlePtr TReqBundleMerger::GetResult() const
    {
        return MergedBundle;
    }
} // NReqBundle
