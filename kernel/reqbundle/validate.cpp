#include "validate.h"

#include "restrict.h"
#include "serializer.h"
#include "block_pure.h"
#include "request_pure.h"

#include <util/string/builder.h>

namespace {
    using namespace NReqBundle;
    using TErrorGuard = NLingBoost::TErrorHandler::TGuard;

    inline NReqBundle::NDetail::TValidConstraints MakeValidConstraintsForSearch() {
        NReqBundle::NDetail::TValidConstraints res;
        res.NeedOriginal = true;
        res.NeedNonEmpty = true;
        res.NeedBlocks = true;
        res.NeedBinaries = true;
        return res;
    }
} // namespace

namespace NReqBundle {
namespace NDetail {
    const TValidConstraints ConstraintsForSearch = MakeValidConstraintsForSearch();

    TReqBundleValidater::TReqBundleValidater(const TValidConstraints& constr, const TOptions& options)
        : Constr(constr)
        , Options(options)
        , Handler(NLingBoost::TErrorHandler::EFailMode::SkipOnError)
    {}

    void TReqBundleValidater::Validate(
        TReqBundlePtr& bundle)
    {
        if (Y_UNLIKELY(!bundle)) {
            bundle = nullptr;
            return;
        }

        if (0 == bundle->GetNumRequests()) {
            bundle = new TReqBundle;
        }

        if (Options.RemoveRequestsWithoutMatches && 0 == bundle->GetSequence().GetNumElems()) {
            bundle = new TReqBundle;
        }

        TErrorGuard bundleGuard{Handler, "reqbundle"};

        TReqBundleSubset subset(*bundle);
        bool hasSkippedBlocks = false;

        auto seq = bundle->Sequence();

        {
            TErrorGuard seqGuard{Handler, "sequence"};

            THolder<TReqBundleSerializer> ser;
            if (Constr.NeedBinaries) {
                ser = MakeHolder<TReqBundleSerializer>(TCompressorFactory::NO_COMPRESSION);
            }

            THolder<TReqBundleDeserializer> deser;
            TReqBundleDeserializer::TOptions deserOpts;
            deserOpts.FailMode = TReqBundleDeserializer::EFailMode::SkipOnError;
            deser = MakeHolder<TReqBundleDeserializer>(deserOpts);

            Y_ASSERT(!Constr.IgnoreElems || (!Constr.NeedBlocks && !Constr.NeedBinaries));
            if (Y_LIKELY(!Constr.IgnoreElems)) {
                for (size_t i : xrange(seq.GetNumElems())) {
                    TErrorGuard blockGuard{Handler, "elem", i};

                    auto elem = seq.Elem(i);
                    bool isCreatedBlock = false;
                    bool blockHasErrors = false;

                    if (!elem.HasBlock()) {
                        Y_ASSERT(!!deser);
                        elem.PrepareBlock(*deser);
                        if (deser->IsInErrorState()) {
                            Handler.Error(yexception{}
                                << "errors in block deserialization\n"
                                << deser->GetFullErrorMessage());
                            deser->ClearErrorState();

                            blockHasErrors = true;
                        }
                        isCreatedBlock = true;
                    }

                    auto block = elem.GetBlock();
                    if (!block.IsValid() || !NReqBundle::NDetail::IsValidBlock(block)) {
                        Handler.Error(yexception{} << "block is invalid");
                        subset.KillBlock(i);

                        hasSkippedBlocks = true;
                        continue;
                    }

                    if (Constr.NeedBinaries && (blockHasErrors || !elem.HasBinary())) {
                        // Serialized binaries are needed for reqbundle_iterator cache.
                        // Note however that cache uses raw binary blobs as keys.
                        // Therefore it is sensitive to compression type.
                        //
                        Y_ASSERT(!!ser);
                        elem.DiscardBinary();
                        elem.PrepareBinary(*ser);
                    }

                    if (!Constr.NeedBlocks && isCreatedBlock) {
                        elem.DiscardBlock();
                    }
                }
            }
        }

        bool hasValidOriginal = false;
        bool hasSkippedRequests = false;
        bool hasClearedRequestTrInfo = false;
        size_t originalIndex = Max<size_t>();

        TSet<EExpansionType> foundSingleTypes;

        for (size_t i : xrange(bundle->GetNumRequests())) {
            TErrorGuard requestGuard{Handler, "request", i};

            auto request = bundle->GetRequest(i);

            if (!NDetail::IsValidTrCompatibilityInfo(request)) {
                Handler.Error(yexception() << "invalid tr compatibility info");
                hasClearedRequestTrInfo = true;
                subset.ClearRequestTrInfo(i);
            }

            if (!NDetail::IsValidRequest(request, seq, false)) {
                Handler.Error(yexception{} << "request is invalid");
                hasSkippedRequests = true;
                continue;
            }

            if (Options.RemoveRequestsWithoutMatches && 0 == request.GetNumMatches()) {
                Handler.Error(yexception{} << "request has 0 matches");
                hasSkippedRequests = true;
                continue;
            }

            for (size_t j : xrange(request.GetFacets().GetNumEntries())) {
                TErrorGuard facetGuard{Handler, "facet", j};

                auto entry = request.GetFacets().GetEntry(j);
                const EExpansionType expansionType = entry.GetExpansion();
                const bool isMultiRequest = GetExpansionTraitsByType(expansionType).IsMultiRequest;

                if (isMultiRequest || !foundSingleTypes.contains(expansionType)) {
                    subset.AddRequest(i, entry.GetId());
                    if (entry.NeedTrCompatibilityInfo()) {
                        hasValidOriginal = true;
                        originalIndex = i;
                    }
                    if (!isMultiRequest) {
                        foundSingleTypes.insert(expansionType);
                    }
                } else {
                    Handler.Error(yexception{} << "duplicate request for type " << expansionType);
                    hasSkippedRequests = true;
                }
            }
        }

        for (size_t i = 0; i < bundle->GetNumConstraints(); i++) {
            TErrorGuard constraintsGuard{Handler, "constraint", i};
            TConstConstraintAcc constraint = bundle->GetConstraint(i);
            if (!NReqBundle::NDetail::IsValidConstraint(constraint, seq)) {
                Handler.Error(yexception{} << "constraint is invalid");
                subset.KillConstraint(i);
            }
        }

        if (Constr.NeedOriginal && !hasValidOriginal) {
            Handler.Error(yexception{} << "original request is absent");
            bundle = nullptr;
            return;
        }

        if (hasSkippedBlocks || hasSkippedRequests || hasClearedRequestTrInfo) {
            TRequestsRemapper remap;
            bundle = subset.GetResult(remap);

            if (Constr.NeedOriginal
                && hasValidOriginal
                && Max<size_t>() == remap[originalIndex])
            {
                Handler.Error(yexception{} << "original request was removed");
                bundle = nullptr;
                return;
            }
        }

        if (!bundle || 0 == bundle->GetNumRequests()) {
            if (Constr.NeedNonEmpty || Constr.NeedOriginal) {
                bundle = nullptr;
            } else {
                bundle = new TReqBundle;
            }
            return;
        }

        Y_ASSERT(!!bundle);
        if (!NReqBundle::NDetail::IsValidReqBundle(*bundle, Constr)) {
            Handler.Error(yexception{} << "reqbundle is invalid after validate");
            bundle = nullptr;
            return;
        }
    }

    void TReqBundleValidater::PrepareForSearch(
        TReqBundlePtr& bundle,
        bool filterOffBadAttributes)
    {
        Validate(bundle);

        if (!bundle) {
            return;
        }

        if (filterOffBadAttributes) {
            for (NReqBundle::TSequenceElemAcc elem : bundle->Sequence().Elems()) {
                static const TUtf16String TelFullAttribute = u"tel_full=";
                static const TUtf16String TelLocalAttribute = u"tel_local=";
                if (elem.HasBlock()) {
                    NReqBundle::TBlockAcc block = elem.Block();
                    if (block.IsWordBlock()
                        && block.GetWord().GetNumLemmas() == 1
                        && block.GetWord().GetLemma(0).IsAttribute()
                        && !block.GetWord().GetLemma(0).GetWideText().StartsWith(TelFullAttribute)
                        && !block.GetWord().GetLemma(0).GetWideText().StartsWith(TelLocalAttribute))
                    {
                        NReqBundle::NDetail::TBlockData& data = NReqBundle::NDetail::BackdoorAccess(block);
                        data.Words[0].Text = u"attr=set_random_value_is_very_very_ugly_hack";
                        data.Words[0].Lemmas[0].Text = u"attr=set_random_value_is_very_very_ugly_hack";
                    }
                }
            }
        }

        NReqBundle::FillRevFreqs(*bundle);
    }
} // NDetail
} // NReqBundle
