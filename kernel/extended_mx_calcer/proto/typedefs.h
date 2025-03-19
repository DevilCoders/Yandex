#pragma once

#include <kernel/extended_mx_calcer/proto/scheme.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>

#include <utility>

namespace NExtendedMx {

    using TResultTransformConstProto = NSCProto::TResultTransformConst<TSchemeTraits>;

    using TStumpProto = NSCProto::TStumpConst<TSchemeTraits>;
    using TStumpsConstProto = NSCProto::TStumpsConst<TSchemeTraits>;

    using TFmlProto = NSCProto::TFml<TSchemeTraits>;
    using TFmlConstProto = NSCProto::TFmlConst<TSchemeTraits>;

    using TBundleProto = NSCProto::TBundle<TSchemeTraits>;
    using TBundleConstProto = NSCProto::TBundleConst<TSchemeTraits>;

    using TAbsoluteRelevanceProto = NSCProto::TAbsoluteRelevance<TSchemeTraits>;
    using TAbsoluteRelevanceConstProto = NSCProto::TAbsoluteRelevanceConst<TSchemeTraits>;

    using TClickIntProto = NSCProto::TClickInt<TSchemeTraits>;
    using TClickIntConstProto = NSCProto::TClickIntConst<TSchemeTraits>;

    using TClickRegTreeProto = NSCProto::TClickRegTree<TSchemeTraits>;
    using TClickRegTreeConstProto = NSCProto::TClickRegTreeConst<TSchemeTraits>;

    using TClickIntRandomProto = NSCProto::TClickIntRandom<TSchemeTraits>;
    using TClickIntRandomConstProto = NSCProto::TClickIntRandomConst<TSchemeTraits>;

    using TFactorFilterProto = NSCProto::TFactorFilter<TSchemeTraits>;
    using TFactorFilterConstProto = NSCProto::TFactorFilterConst<TSchemeTraits>;

    using TSimpleProto = NSCProto::TSimple<TSchemeTraits>;
    using TSimpleConstProto = NSCProto::TSimpleConst<TSchemeTraits>;

    using TMultiBundleConstProto = NSCProto::TMultiBundleConst<TSchemeTraits>;

    using TMultiBundleByFactorsConstProto = NSCProto::TMultiBundleByFactorsConst<TSchemeTraits>;

    using TMultiBundleWithExternalViewTypeMxConstProto =
            NSCProto::TMultiBundleWithExternalViewTypeMxConst<TSchemeTraits>;

    using TCalcContextProto = NSCProto::TCalcContext<TSchemeTraits>;
    using TCalcContextMetaProto = NSCProto::TMeta<TSchemeTraits>;
    using TCalcContextMetaConstProto = NSCProto::TMetaConst<TSchemeTraits>;
    using TCalcContextResultProto = NSCProto::TResult<TSchemeTraits>;
    using TCalcContextResultConstProto = NSCProto::TResultConst<TSchemeTraits>;

    using TAllCategScoresProto = decltype(std::declval<TCalcContextMetaProto>().RawPredictions().AllCategScores());
    using TAllCategScoresConstProto = decltype(std::declval<TCalcContextMetaConstProto>().RawPredictions().AllCategScores());

    using TScoredCategsProto = NSCProto::TScoredCategs<TSchemeTraits>;
    using TScoredCategsConstProto = NSCProto::TScoredCategsConst<TSchemeTraits>;
    using TMultiPredictProto = NSCProto::TMultiPredict<TSchemeTraits>;
    using TMultiPredictConstProto = NSCProto::TMultiPredictConst<TSchemeTraits>;

    using TMultiFeatureWinLossMcInfoProto = NSCProto::TMultiFeatureWinLossMcInfo<TSchemeTraits>;
    using TMultiFeatureWinLossMcInfoConstProto = NSCProto::TMultiFeatureWinLossMcInfoConst<TSchemeTraits>;

    using TFeatureResultProto = decltype(std::declval<TCalcContextResultProto>().FeatureResult());
    using TFeatureResultConstProto = decltype(std::declval<TCalcContextResultConstProto>().FeatureResult());

    using TWinLossLinearProto = NSCProto::TWinLossLinear<TSchemeTraits>;
    using TWinLossLinearConstProto  = NSCProto::TWinLossLinearConst<TSchemeTraits>;

    using TFeatureInfoProto = NSCProto::TFeatureInfo<TSchemeTraits>;
    using TFeatureInfoConstProto  = NSCProto::TFeatureInfoConst<TSchemeTraits>;

    using TFeatureContextProto = NSCProto::TFeatureContext<TSchemeTraits>;
    using TFeatureContextConstProto  = NSCProto::TFeatureContextConst<TSchemeTraits>;

    using TFeatureContextDictProto = decltype(std::declval<TCalcContextMetaProto>().FeatureContext());
    using TFeatureContextDictConstProto  = decltype(std::declval<TCalcContextMetaConstProto>().FeatureContext());

    using TAdditionalFeaturesProto = decltype(std::declval<TWinLossLinearProto>().Params().AdditionalFeatures());
    using TAdditionalFeaturesConstProto = decltype(std::declval<TWinLossLinearConstProto>().Params().AdditionalFeatures());

    using TMultiTargetMxProto = NSCProto::TMultiTargetMx<TSchemeTraits>;
    using TMultiTargetMxConstProto  = NSCProto::TMultiTargetMxConst<TSchemeTraits>;

    using TCascadeMxProto = NSCProto::TCascadeMx<TSchemeTraits>;
    using TCascadeMxConstProto  = NSCProto::TCascadeMxConst<TSchemeTraits>;

    using TMxWithMetaMxsProto = NSCProto::TMxWithMetaMxs<TSchemeTraits>;
    using TMxWithMetaMxsConstProto = NSCProto::TMxWithMetaMxsConst<TSchemeTraits>;

    using TSurplusPredictProto = NSCProto::TSurplusPredict<TSchemeTraits>;
    using TSurplusPredictConstProto = NSCProto::TSurplusPredictConst<TSchemeTraits>;

    using TMultiClassWithFilterProto = NSCProto::TMultiClassWithFilter<TSchemeTraits>;
    using TMultiClassWithFilterConstProto = NSCProto::TMultiClassWithFilterConst<TSchemeTraits>;

    using TClickIntBruteForceProto = NSCProto::TClickIntBruteForce<TSchemeTraits>;
    using TClickIntBruteForceConstProto = NSCProto::TClickIntBruteForceConst<TSchemeTraits>;

    using TTrimmedFmlGroupProto = NSCProto::TTrimmedFmlGroup<TSchemeTraits>;
    using TTrimmedFmlGroupConstProto  = NSCProto::TTrimmedFmlGroupConst<TSchemeTraits>;

    using TMultiTargetMxFastProto = NSCProto::TMultiTargetMxFast<TSchemeTraits>;
    using TMultiTargetMxFastConstProto  = NSCProto::TMultiTargetMxFastConst<TSchemeTraits>;

    using TIdxRandomProto = NSCProto::TIdxRandom<TSchemeTraits>;
    using TIdxRandomConstProto  = NSCProto::TIdxRandomConst<TSchemeTraits>;

    using TThompsonSamplingProto = NSCProto::TThompsonSampling<TSchemeTraits>;
    using TThompsonSamplingConstProto = NSCProto::TThompsonSamplingConst<TSchemeTraits>;

    using TMultiFeatureLocalRandomProto = NSCProto::TMultiFeatureLocalRandom<TSchemeTraits>;
    using TMultiFeatureLocalRandomConstProto = NSCProto::TMultiFeatureLocalRandomConst<TSchemeTraits>;
    using TMultiFeatureLocalRandomParamsConstProto = TMultiFeatureLocalRandomConstProto::TMultiFeatureLocalRandomParamsConst;

    using TWeightRandomProto = NSCProto::TWeightRandom<TSchemeTraits>;
    using TWeightRandomConstProto  = NSCProto::TWeightRandomConst<TSchemeTraits>;

    using TViewPlacePosRandomProto = NSCProto::TViewPlacePosRandom<TSchemeTraits>;
    using TViewPlacePosRandomConstProto  = NSCProto::TViewPlacePosRandomConst<TSchemeTraits>;

    using TMultiShowRandomProto = NSCProto::TMultiShowRandom<TSchemeTraits>;
    using TMultiShowRandomConstProto  = NSCProto::TMultiShowRandomConst<TSchemeTraits>;
    using TMultiShowRandomParamsConstProto  = TMultiShowRandomProto::TMultiShowRandomParams;

    using TPositionalClickIntProto = NSCProto::TPositionalClickInt<TSchemeTraits>;
    using TPositionalClickIntConstProto = NSCProto::TPositionalClickIntConst<TSchemeTraits>;

    using TPositionalMultiClassSAMClickIntProto = NSCProto::TPositionalMultiClassSAMClickInt<TSchemeTraits>;
    using TPositionalMultiClassSAMClickIntConstProto = NSCProto::TPositionalMultiClassSAMClickIntConst<TSchemeTraits>;

    using TPositionalSoftmaxWithSubtargetsProto = NSCProto::TPositionalSoftmaxWithSubtargets<TSchemeTraits>;
    using TPositionalSoftmaxWithSubtargetsConstProto = NSCProto::TPositionalSoftmaxWithSubtargetsConst<TSchemeTraits>;

    using TPositionalSoftmaxWithPositionalSubtargetsProto = NSCProto::TPositionalSoftmaxWithPositionalSubtargets<TSchemeTraits>;
    using TPositionalSoftmaxWithPositionalSubtargetsConstProto = NSCProto::TPositionalSoftmaxWithPositionalSubtargetsConst<TSchemeTraits>;

    using TMultiFeatureSoftmaxProto = NSCProto::TMultiFeatureSoftmax<TSchemeTraits>;
    using TMultiFeatureSoftmaxConstProto = NSCProto::TMultiFeatureSoftmaxConst<TSchemeTraits>;

    using TMultiFeatureWinLossMultiClassProto = NSCProto::TMultiFeatureWinLossMultiClass<TSchemeTraits>;
    using TMultiFeatureWinLossMultiClassConstProto = NSCProto::TMultiFeatureWinLossMultiClassConst<TSchemeTraits>;

    using TPositionalPerceptronProto = NSCProto::TPositionalPerceptron<TSchemeTraits>;
    using TPositionalPerceptronConstProto = NSCProto::TPositionalPerceptronConst<TSchemeTraits>;

    using TNeuralNetInputInfoConstProto = NSCProto::TNeuralNetInputInfoConst<TSchemeTraits>;

    using TNeuralNetProto = NSCProto::TNeuralNet<TSchemeTraits>;
    using TNeuralNetConstProto = NSCProto::TNeuralNetConst<TSchemeTraits>;

    using TPositionalMajorityVoteProto = NSCProto::TPositionalMajorityVote<TSchemeTraits>;
    using TPositionalMajorityVoteConstProto = NSCProto::TPositionalMajorityVoteConst<TSchemeTraits>;

    using TPositionalBinaryCompositionProto = NSCProto::TPositionalBinaryComposition<TSchemeTraits>;
    using TPositionalBinaryCompositionConstProto = NSCProto::TPositionalBinaryCompositionConst<TSchemeTraits>;

    using TOnePositionBinaryProto = NSCProto::TOnePositionBinary<TSchemeTraits>;
    using TOnePositionBinaryConstProto = NSCProto::TOnePositionBinaryConst<TSchemeTraits>;

    using TBinaryWithArbitraryResultProto = NSCProto::TBinaryWithArbitraryResult<TSchemeTraits>;
    using TBinaryWithArbitraryResultConstProto = NSCProto::TBinaryWithArbitraryResultConst<TSchemeTraits>;

    using TClickIntIncutMixProto = NSCProto::TClickIntIncutMix<TSchemeTraits>;
    using TClickIntIncutMixConstProto = NSCProto::TClickIntIncutMixConst<TSchemeTraits>;

    using TRandomIncutMixProto = NSCProto::TRandomIncutMix<TSchemeTraits>;
    using TRandomIncutMixConstProto = NSCProto::TRandomIncutMixConst<TSchemeTraits>;

    using TRandomMaskMixProto = NSCProto::TRandomMaskMix<TSchemeTraits>;
    using TRandomMaskMixConstProto = NSCProto::TRandomMaskMixConst<TSchemeTraits>;

    using TAvailVTCombinations = decltype(std::declval<TCalcContextMetaProto>().AvailableVTCombinations());
    using TAvailVTCombinationsConst = decltype(std::declval<TCalcContextMetaConstProto>().AvailableVTCombinations());

    using TCombinationsInfoProto = NSCProto::TCombinationsInfo<TSchemeTraits>;
    using TCombinationsInfoConstProto = NSCProto::TCombinationsInfoConst<TSchemeTraits>;

    using TMultiTargetCombinationsProto = NSCProto::TMultiTargetCombinations<TSchemeTraits>;
    using TMultiTargetCombinationsConstProto = NSCProto::TMultiTargetCombinationsConst<TSchemeTraits>;

    using TAnyWithRandomProto = NSCProto::TAnyWithRandom<TSchemeTraits>;
    using TAnyWithRandomConstProto = NSCProto::TAnyWithRandomConst<TSchemeTraits>;

    using TAnyWithFilterProto = NSCProto::TAnyWithFilter<TSchemeTraits>;
    using TAnyWithFilterConstProto = NSCProto::TAnyWithFilterConst<TSchemeTraits>;

    using TTwoCalcersUnionProto = NSCProto::TTwoCalcersUnion<TSchemeTraits>;
    using TTwoCalcersUnionConstProto = NSCProto::TTwoCalcersUnionConst<TSchemeTraits>;

} // NExtendedMx
