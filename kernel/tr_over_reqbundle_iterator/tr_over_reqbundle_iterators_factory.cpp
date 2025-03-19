#include "tr_over_reqbundle_iterators_factory.h"
#include "helpers.h"

#include <kernel/reqbundle/request_pure.h>
#include <kernel/reqbundle/request_splitter.h>

#include <kernel/tr_over_reqbundle_iterator/proto/factory_data.pb.h>

#include <library/cpp/pop_count/popcount.h>

namespace NTrOverReqBundleIterator {
    //
    //  TFactoryData
    //

    TFactoryData::TFactoryData(
        NReqBundle::TReqBundlePtr bundle,
        ui32 wordCount,
        const NReqBundleIterator::TGlobalOptions& globalOptions,
        bool useConstraintChecker)
        : UseConstraintChecker(useConstraintChecker)
    {
        if (!bundle) {
            return;
        }

        bool hasRequestWithTrInfo = false;
        NReqBundle::TConstRequestAcc originalRequest;
        for (NReqBundle::TConstRequestAcc request : bundle->GetRequests()) {
            if (request.GetTrCompatibilityInfo().Defined()) {
                if (globalOptions.RequestExpansionType.Defined() && !request.HasExpansionType(globalOptions.RequestExpansionType.GetRef())) {
                    continue;
                }
                originalRequest = request;
                hasRequestWithTrInfo = true;
                break;
            }
        }

        if (UseConstraintChecker) {
            UseConstraintChecker = false;
            for (NReqBundle::TConstConstraintAcc constraint : bundle->GetConstraints()) {
                NLingBoost::EConstraintType type = constraint.GetType();
                if (type == NLingBoost::TConstraint::Quoted
                    || type == NLingBoost::TConstraint::MustNot
                    || type == NLingBoost::TConstraint::Must)
                {
                    UseConstraintChecker = true;
                    break;
                }
            }
        }

        if (!hasRequestWithTrInfo) {
            return;
        }
        const NReqBundle::TRequestTrCompatibilityInfo& trCompatibilityInfo = *originalRequest.GetTrCompatibilityInfo();
        // Y_ASSERT(wordCount == trCompatibilityInfo.WordCount); TODO(danlark@) not true anymore?
        TrIteratorWordCount = Min(trCompatibilityInfo.WordCount, wordCount);

        const ui32 limitedWordCount = Min(TrIteratorWordCount, ui32(64));

        Y_ASSERT(originalRequest.GetNumMatches() == trCompatibilityInfo.MainPartsWordMasks.size() + trCompatibilityInfo.MarkupPartsBestFormClasses.size());
        const size_t numMatches = Min(trCompatibilityInfo.MainPartsWordMasks.size() + trCompatibilityInfo.MarkupPartsBestFormClasses.size(), originalRequest.GetNumMatches());

        TVector<ui64> reqBundleWordsMask(originalRequest.GetNumWords(), 0);
        size_t offset = 0;
        for (size_t i = 0; i < trCompatibilityInfo.MainPartsWordMasks.size() && i < numMatches; ++i) {
            NReqBundle::TConstMatchAcc match = originalRequest.GetMatch(i);
            const size_t numWords = match.GetWordIndexLast() - match.GetWordIndexFirst() + 1;
            Y_ASSERT(numWords <= reqBundleWordsMask.size() - offset);
            const size_t nextOffset = Min(offset + numWords, reqBundleWordsMask.size());
            Fill(reqBundleWordsMask.begin() + offset, reqBundleWordsMask.begin() + nextOffset, trCompatibilityInfo.MainPartsWordMasks[i]);
            offset = nextOffset;
        }
        Y_ASSERT(offset == reqBundleWordsMask.size());

        BlockTrIteratorWordIdxs.resize(bundle->GetSequence().GetNumElems());

        for (size_t i = 0; i < numMatches; ++i) {
            NReqBundle::TConstMatchAcc match = originalRequest.GetMatch(i);
            EFormClass form;
            if (i < trCompatibilityInfo.MainPartsWordMasks.size()) {
                Y_ASSERT(match.GetType() == NReqBundle::EMatchType::OriginalWord);
                form = EQUAL_BY_STRING;
            } else {
                Y_ASSERT(match.GetType() == NReqBundle::EMatchType::Synonym);
                const size_t markupPartIndex = i - trCompatibilityInfo.MainPartsWordMasks.size();
                form = trCompatibilityInfo.MarkupPartsBestFormClasses[markupPartIndex];
            }
            for (size_t wordIdx : xrange(match.GetWordIndexFirst(), Min<size_t>(match.GetWordIndexLast() + 1, limitedWordCount))) {
                BlockTrIteratorWordIdxs[match.GetBlockIndex()].emplace_back(wordIdx, form);
            }
        }

        if (bundle->GetNumConstraints() > 0) {
            BlockMustNot.resize(BlockTrIteratorWordIdxs.size(), false);
            for (auto constraint : bundle->GetConstraints()) {
                if (constraint.GetType() == NReqBundle::EConstraintType::MustNot) {
                    for (const size_t blockIndex : constraint.GetBlockIndices()) {
                        BlockMustNot[blockIndex] = true;
                    }
                }
            }
        }

    }

    TFactoryData TFactoryData::Deserialize(TStringBuf serialized) {
        TFactoryData data;

        NProto::TFactoryData proto;
        Y_ENSURE(proto.ParseFromArray(serialized.data(), serialized.size()));

        data.UseConstraintChecker = proto.GetUseConstraintChecker();
        data.TrIteratorWordCount = proto.GetTrIteratorWordCount();

        data.BlockTrIteratorWordIdxs.reserve(proto.GetBlockTrIteratorWordIdxs().size());
        for (const auto& pairs : proto.GetBlockTrIteratorWordIdxs()) {
            TVector<std::pair<ui32, EFormClass>> indicesForBlock(Reserve(pairs.GetPairs().size()));
            for (const auto& pair : pairs.GetPairs()) {
                indicesForBlock.emplace_back(pair.GetWordIndex(), static_cast<EFormClass>(pair.GetFormClass()));
            }
            data.BlockTrIteratorWordIdxs.push_back(std::move(indicesForBlock));
        }

        data.BlockMustNot.assign(proto.GetBlockMustNot().begin(), proto.GetBlockMustNot().end());

        return data;
    }

    TString TFactoryData::Serialize() const {
        NProto::TFactoryData proto;

        proto.SetUseConstraintChecker(UseConstraintChecker);
        proto.SetTrIteratorWordCount(TrIteratorWordCount);

        proto.MutableBlockTrIteratorWordIdxs()->Reserve(BlockTrIteratorWordIdxs.size());
        for (const auto& indicesForBlock : BlockTrIteratorWordIdxs) {
            NProto::TIndexFormPairs* pairs = proto.MutableBlockTrIteratorWordIdxs()->Add();
            pairs->MutablePairs()->Reserve(indicesForBlock.size());
            for (const auto& pair : indicesForBlock) {
                NProto::TIndexFormPair* protoPair = pairs->MutablePairs()->Add();
                protoPair->SetWordIndex(pair.first);
                protoPair->SetFormClass(pair.second);
            }
        }

        proto.MutableBlockMustNot()->Reserve(BlockMustNot.size());
        for (const bool blockStatus : BlockMustNot) {
            proto.MutableBlockMustNot()->Add(blockStatus);
        }

        return proto.SerializeAsString();
    }

    //
    //  TTrOverRBIteratorsFactory
    //

    TTrOverRBIteratorsFactory::TTrOverRBIteratorsFactory(
        const TRichTreeConstPtr& richTree,
        ui32 wordCount,
        bool filterOffBadAttribute,
        const NReqBundleIterator::TGlobalOptions& globalOptions,
        bool useConstraintChecker,
        bool generateTopAndArgs)
        : Bundle(ConvertRichTreeToReqBundle(richTree, filterOffBadAttribute, useConstraintChecker, generateTopAndArgs))
        , Factory(MakeHolder<TReqBundleIteratorsFactory>(*Bundle, globalOptions))
        , Data(Bundle, wordCount, globalOptions, useConstraintChecker)
    {
    }

    TTrOverRBIteratorsFactory::TTrOverRBIteratorsFactory(NReqBundle::TReqBundlePtr reqBundle, ui32 wordCount, const NReqBundleIterator::TGlobalOptions& globalOptions, bool useConstraintChecker)
        : Bundle(reqBundle)
        , Factory(MakeHolder<TReqBundleIteratorsFactory>(*Bundle, globalOptions))
        , Data(Bundle, wordCount, globalOptions, useConstraintChecker)
    {

    }

    TTrOverRBIteratorsFactory::TTrOverRBIteratorsFactory(
        TFactoryData data,
        NReqBundle::TReqBundlePtr bundle,
        THolder<TReqBundleIteratorsFactory> factory)
        : Bundle(bundle)
        , Factory(std::move(factory))
        , Data(std::move(data))
    {
    }

    TTrOverReqBundleIteratorPtr TTrOverRBIteratorsFactory::OpenIterator(
        NReqBundleIterator::IRBIteratorBuilder& builder,
        NReqBundleIterator::IRBIteratorsHasher* hasher,
        const NReqBundleIterator::TRBIteratorOptions& options,
        const NReqBundleIterator::TConstraintChecker::TOptions& constraintCheckerOptions,
        const ISentenceLengthsLenReader* sentReader)
    {
        THolder<NReqBundleIterator::TConstraintChecker> constraintChecker;
        if (Bundle && Data.UseConstraintChecker) {
            constraintChecker = MakeHolder<NReqBundleIterator::TConstraintChecker>(*Bundle, constraintCheckerOptions);
        }
        return MakeHolder<TTrOverReqBundleIterator>(
            Factory ? Factory->OpenIterator(builder, hasher, options) : TReqBundleIteratorPtr(),
            Data.BlockTrIteratorWordIdxs,
            Data.TrIteratorWordCount,
            std::move(constraintChecker),
            sentReader,
            Data.BlockMustNot);
    }

    TTrOverReqBundleIteratorPtr TTrOverRBIteratorsFactory::WrapIterator(
            NReqBundleIterator::TRBIteratorPtr iterator,
            THolder<NReqBundleIterator::TConstraintChecker> checker,
            const ISentenceLengthsLenReader* sentReader)
    {
        return MakeHolder<TTrOverReqBundleIterator>(
            std::move(iterator),
            Data.BlockTrIteratorWordIdxs,
            Data.TrIteratorWordCount,
            std::move(checker),
            sentReader,
            Data.BlockMustNot);
    }

} // namespace NTrOverReqBundleIterator
