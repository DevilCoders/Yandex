#pragma once

#include "tr_over_reqbundle_iterator.h"

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/reqbundle_iterator/constraint_checker.h>


namespace NTrOverReqBundleIterator {

    // Used to move all the heavy work dependent on the request bundle to some
    // other place (possibly, on another machine), and then recreate the factory
    // itself using this precomputed data.
    class TFactoryData {
        friend class TTrOverRBIteratorsFactory;
    public:
        TFactoryData(
            NReqBundle::TReqBundlePtr reqBundle,
            ui32 wordCount,
            const NReqBundleIterator::TGlobalOptions& globalOptions,
            bool useConstraintChecker = false);

        static TFactoryData Deserialize(TStringBuf serialized);

        TString Serialize() const;

    private:
        TFactoryData() = default;

    private:
        bool UseConstraintChecker = false;
        ui32 TrIteratorWordCount = 0;
        TVector<TVector<std::pair<ui32, EFormClass>>> BlockTrIteratorWordIdxs;
        TVector<bool> BlockMustNot;
    };

    class TTrOverRBIteratorsFactory {
    public:
        TTrOverRBIteratorsFactory() = default;
        TTrOverRBIteratorsFactory(const TRichTreeConstPtr& richTree, ui32 wordCount, bool filterOffBadAttribute = false, const NReqBundleIterator::TGlobalOptions& globalOptions = {}, bool useConstraintChecker = false, bool generateTopAndArgs = true);
        TTrOverRBIteratorsFactory(NReqBundle::TReqBundlePtr reqBundle, ui32 wordCount, const NReqBundleIterator::TGlobalOptions& globalOptions = {}, bool useConstraintChecker = false);

        // Constructs the factory from precomputed data.
        // bundle and factory can be ommited if we only intend to use WrapIterator and never OpenIterator
        TTrOverRBIteratorsFactory(
            TFactoryData data,
            NReqBundle::TReqBundlePtr bundle = nullptr,
            THolder<TReqBundleIteratorsFactory> factory = nullptr);

        TTrOverReqBundleIteratorPtr OpenIterator(
            NReqBundleIterator::IRBIteratorBuilder& builder,
            NReqBundleIterator::IRBIteratorsHasher* hasher,
            const NReqBundleIterator::TRBIteratorOptions& options,
            const NReqBundleIterator::TConstraintChecker::TOptions& constraintCheckerOptions,
            const ISentenceLengthsLenReader* sentReader);

        // Wraps an already existing iterator
        TTrOverReqBundleIteratorPtr WrapIterator(
            NReqBundleIterator::TRBIteratorPtr iterator,
            THolder<NReqBundleIterator::TConstraintChecker> checker,
            const ISentenceLengthsLenReader* sentReader);

        ui32 GetTrIteratorWordCount() const {
            return Data.TrIteratorWordCount;
        }

        NReqBundle::TReqBundlePtr GetBundle() const {
            return Bundle;
        }

    private:
        void InitInternal(ui32 wordCount, const NReqBundleIterator::TGlobalOptions& globalOptions);

        NReqBundle::TReqBundlePtr Bundle;
        THolder<TReqBundleIteratorsFactory> Factory;
        TFactoryData Data;
    };
} // namespace NTrOverReqBundleIterator

using TTrOverReqBundleIteratorsFactory = NTrOverReqBundleIterator::TTrOverRBIteratorsFactory;
using TTrOverReqBundleIteratorsFactoryPtr = THolder<TTrOverReqBundleIteratorsFactory>;
