#include "common.h"

#include <kernel/vector_machine/codegen/config.pb.h>

#include <kernel/vector_machine/transforms.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NVectorMachine {
    TGeneratedCalcer::TGeneratedCalcer(const TCalcerConfig& config)
        : CalcerConfig_(config)
    {
        size_t aggrNum = 0;
        size_t weightsPos = 0;
        size_t slicerNum = 0;

        for (const TAggregatorListConfig& aggrListConfig : config.GetAggregatorLists()) {
            TGeneratedSlicer slicer(aggrListConfig.GetSlicer(), slicerNum);
            Y_ENSURE(aggrListConfig.GetWeightsIndex() == -1 || aggrListConfig.GetWeightsIndex() >= 0);
            size_t weightIndex = aggrListConfig.GetWeightsIndex() == -1 ? weightsPos : aggrListConfig.GetWeightsIndex();
            TGeneratedAggregatorList aggrList(slicer, weightIndex);
            for (const TAggregatorConfig& aggrConfig : aggrListConfig.GetAggregators()) {
                TString name = TString::Join("Aggr", ToString(aggrNum));
                const TString& prefix = aggrListConfig.GetName();

                TGeneratedAggregator aggregator(aggrConfig, name, prefix);

                const TVector<TString>& featureNames = aggregator.GetFeatureNames();
                FeatureNames_.insert(FeatureNames_.end(), featureNames.begin(), featureNames.end());

                aggrList.AddAggregator(std::move(aggregator));

                aggrNum++;
            }
            AggregatorLists_.emplace_back(std::move(aggrList));
            NWeights_ = ::Max(NWeights_, aggrList.GetWeightsPos() + 1);

            ++slicerNum;
            ++weightsPos;
        }

        if (config.HasFeatureNamePrefix()) {
            for (TString& featureName : FeatureNames_) {
                featureName = TString::Join(config.GetFeatureNamePrefix(), "_", featureName);
            }
        }

        Y_ENSURE(FeatureNames_.size() > 0);
    }

    void TGeneratedCalcer::Output(IOutputStream& out) const {
        out << "class " << CalcerConfig_.GetClassName() << " {" << Endl;
        out << "private:" << Endl;
        OutputAggregatorListMembers(out);
        out << Endl;
        out << "public:" << Endl;
        OutputConstructor(out);
        out << Endl;
        OutputCalcSimilarity(out);
        out << Endl;
        OutputCalcFeatures(out);
        out << "};" << Endl;
    }

    void TGeneratedCalcer::OutputTransform(IOutputStream& out, const TTransformConfig& transform, const TString& varName) const {
        out << varName << " = ";
        switch (transform.GetType()) {
            case ETransformType::TT_LINEAR: {
                out << "TLinearTransform(";
                out << transform.GetMultiplier() << ", " << transform.GetBias() << ")";
                break;
            }
            case ETransformType::TT_SIGMOID: {
                out << "TSigmoidTransform()";
                break;
            }
            case ETransformType::TT_CLAMP: {
                out << "TClampTransform(";
                out << transform.GetMinValue() << ", " << transform.GetMaxValue() << ")";
                break;
            }
            default:
                ythrow yexception() << "Unknown transform type";
        }
        out << ".Transform(" << varName << ");" << Endl;
    }

    void TGeneratedCalcer::OutputAggregatorListMembers(IOutputStream& out) const {
        for (const TGeneratedAggregatorList& aggrList : GetAggregatorLists()) {
            out << "    ";
            out << aggrList.GetSlicer().GetClassName() << " " << aggrList.GetSlicer().GetName() << ";" << Endl;
            for (const TGeneratedAggregator& aggregator : aggrList.GetAggregators()) {
                out << "    ";
                out << aggregator.GetClassName() << " " << aggregator.GetName() << ";" << Endl;
            }
        }
    }

    void TGeneratedCalcer::OutputConstructor(IOutputStream& out) const {
        out << "    " << CalcerConfig_.GetClassName() << "()" << Endl;
        bool first = true;
        for (const TGeneratedAggregatorList& aggrList : GetAggregatorLists()) {
            out << "        ";
            if (first) {
                out << ": ";
                first = false;
            } else {
                out << ", ";
            }
            out << aggrList.GetSlicer().GetName() << "(";
            aggrList.GetSlicer().OutputParams(out);
            out << ")" << Endl;
            for (const TGeneratedAggregator& aggregator : aggrList.GetAggregators()) {
                out << "        " << ", ";
                out << aggregator.GetName() << "(";
                aggregator.OutputParamsList(out);
                out << ")" << Endl;
            }
        }
        out << "    {" << Endl << "    }" << Endl;
    }

    void TGeneratedCalcer::OutputCalcSimilarity(IOutputStream& out) const {
        out << "    float CalcSimilarity(const TEmbed& first, const TEmbed& second) const {" << Endl;
        out << "        float value = ";
        switch (CalcerConfig_.GetSimilarity().GetType()) {
            case ESimilarityType::ST_DOT:
                out << "TDotSimilarity";
                break;
            case ESimilarityType::ST_COS:
                out << "TCosSimilarity";
                break;
            case ESimilarityType::ST_DOT_NO_SIZE_CHECK:
                out << "TDotSimilarityNoSizeCheck";
                break;
            default:
                ythrow yexception() << "Unknown similarity type";
        }
        out << "::CalcSim(first, second);" << Endl;
        for (const TTransformConfig& transform : CalcerConfig_.GetSimilarity().GetTransforms()) {
            out << "        ";
            OutputTransform(out, transform, "value");
        }
        out << "        return value;" << Endl;
        out << "    }" << Endl;
    }

    void TGeneratedCalcer::OutputCalcFeatures(IOutputStream& out) const {
        out << "    void CalcFeatures(" << Endl;
        out << "        const TEmbed& queryEmbed," << Endl;
        out << "        const TVector<TEmbed>& docEmbeds," << Endl;
        out << "        const TVector<TWeights>& weights," << Endl;
        out << "        const TArrayRef<float> result," << Endl;
        out << "        const TVector<float>& thresholdValues = {}) const" << Endl;
        out << "    {" << Endl;
        out << "        Y_ENSURE(result.size() == " << FeatureNames_.size() << ");" << Endl;
        out << "        Y_ENSURE(weights.size() == " << NWeights_ << ");" << Endl;
        out << "        TVector<float> scores(Reserve(docEmbeds.size()));" << Endl;
        out << "        for (const TEmbed& docEmbed : docEmbeds) {" << Endl;
        out << "            float value = CalcSimilarity(queryEmbed, docEmbed);" << Endl;
        out << "            scores.push_back(value);" << Endl;
        out << "        }" << Endl;
        size_t featureOffset = 0;
        for (const TGeneratedAggregatorList& aggrList : GetAggregatorLists()) {
            out << "        {" << Endl;
            out << "            const auto [sliceBegin, sliceSize] = ";
            out << aggrList.GetSlicer().GetName() << ".GetBeginAndSize(thresholdValues, scores.size());" << Endl;
            out << "            const TArrayRef<const float> scoresSlice(scores.data() + sliceBegin, sliceSize);" << Endl;
            out << "            const TArrayRef<const float> weightsSlice(weights[" << aggrList.GetWeightsPos() << "].data() + sliceBegin, sliceSize);" << Endl;
            for (const TGeneratedAggregator& aggregator : aggrList.GetAggregators()) {
                out << "            ";
                out << aggregator.GetName() << ".CalcFeatures(scoresSlice, weightsSlice, ";
                out << "result.Slice(" << featureOffset << ", " << aggregator.GetFeatureCount() << ")";
                out <<");" << Endl;
                featureOffset += aggregator.GetFeatureCount();
            }
            out << "        }" << Endl;
        }
        if (CalcerConfig_.HasFeatureTransform()) {
            out << "        for (size_t i = 0; i < " << FeatureNames_.size() << "; i++) {" << Endl;
            out << "            ";
            OutputTransform(out, CalcerConfig_.GetFeatureTransform(), "result[i]");
            out << "        }" << Endl;
        }
        out << "    }" << Endl;
    }

};
