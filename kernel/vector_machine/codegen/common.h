#pragma once

#include <kernel/vector_machine/codegen/config.pb.h>

#include <library/cpp/protobuf/util/pb_io.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/string/cast.h>

namespace NVectorMachine {
    class TGeneratedSlicer {
    public:
        TGeneratedSlicer(const TSlicerConfig& config, const size_t pos)
        {
            switch (config.GetType()) {
                case ESlicerType::ST_ALL:
                    ClassName_ = "TAllSlicer";
                    break;
                case ESlicerType::ST_LESS:
                    ClassName_ = "TLessSlicer";
                    break;
                case ESlicerType::ST_GREATER_EQUAL:
                    ClassName_ = "TGreaterEqualSlicer";
                    break;
            }
            Name_ = "Slicer" + ToString(pos);
            ThresholdValue_ = config.GetThresholdValue();
        }

        const TString& GetName() const {
            return Name_;
        }

        const TString& GetClassName() const {
            return ClassName_;
        }

        void OutputParams(IOutputStream& stream) const {
            stream << ThresholdValue_;
        }

    private:
        TString ClassName_;
        TString Name_;
        float ThresholdValue_;
    };

    class TGeneratedAggregator {
    public:
        TGeneratedAggregator(const TAggregatorConfig& config, const TString& name, const TString& prefix);

        const TAggregatorConfig& GetConfig() const {
            return Config_;
        }

        const TString& GetPrefix() const {
            return Prefix_;
        }

        const TString& GetName() const {
            return Name_;
        }

        size_t GetFeatureCount() const {
            return FeatureNames_.size();
        }

        const TString& GetClassName() const {
            return ClassName_;
        }

        const TVector<TString>& GetFeatureNames() const {
            return FeatureNames_;
        }

        void OutputParamsList(IOutputStream& stream) const;

        TString GenerateNameForScoreThresholdAggregator(const TAggregatorConfig& config) const;

    private:
        TAggregatorConfig Config_;
        TString Name_;
        TString Prefix_;
        TString ClassName_;
        TVector<TString> FeatureNames_;
    };

    class TGeneratedAggregatorList {
    public:
        TGeneratedAggregatorList(const TGeneratedSlicer& slicer, const size_t weightsPos)
            : WeightsPos_(weightsPos)
            , Slicer_(slicer)
        {
        }

        void AddAggregator(TGeneratedAggregator&& aggr) {
            Aggregators_.emplace_back(std::move(aggr));
        }

        const TGeneratedSlicer& GetSlicer() const {
            return Slicer_;
        }

        const TVector<TGeneratedAggregator>& GetAggregators() const {
            return Aggregators_;
        }

        size_t GetWeightsPos() const {
            return WeightsPos_;
        }

    private:
        TVector<TGeneratedAggregator> Aggregators_;
        size_t WeightsPos_ = 0;
        TGeneratedSlicer Slicer_;
    };

    class TGeneratedCalcer {
    public:
        TGeneratedCalcer(const TCalcerConfig& config);

        const TString& GetClassName() const {
            return CalcerConfig_.GetClassName();
        }

        const TVector<TString>& GetFeatureNames() const {
            return FeatureNames_;
        }

        const TVector<TGeneratedAggregatorList>& GetAggregatorLists() const {
            return AggregatorLists_;
        }

        size_t GetNWeights() const {
            return NWeights_;
        }

        void Output(IOutputStream& out) const;

    private:
        void OutputTransform(IOutputStream& out, const TTransformConfig& transform, const TString& varName) const;
        void OutputAggregatorListMembers(IOutputStream& out) const;
        void OutputConstructor(IOutputStream& out) const;
        void OutputCalcSimilarity(IOutputStream& out) const;
        void OutputCalcFeatures(IOutputStream& out) const;

        TCalcerConfig CalcerConfig_;
        TVector<TGeneratedAggregatorList> AggregatorLists_;
        TVector<TString> FeatureNames_;
        size_t NWeights_ = 0;
    };

    class TBoosterGenerator {
    public:
        TBoosterGenerator(const TBoosterConfig& config, const TVector<TGeneratedCalcer>& calcers);

        void Output(IOutputStream& out) const;

    private:
        void OutputEnum(IOutputStream& out) const;
        void OutputCalcerMembers(IOutputStream& out) const;
        void OutputCalcFeatures(IOutputStream& out) const;
        void OutputCalcQueryFactor(IOutputStream& out) const;
        void OutputSizeGetters(IOutputStream& out) const;

        TBoosterConfig Config_;
        const TVector<TGeneratedCalcer>& Calcers_;
        size_t FeaturesCount_ = 0;
        size_t NWeights_ = 0;
    };

    class TDssmBoostingGenerator {
    public:
        TDssmBoostingGenerator(const TDssmBoostingConfig& config);

        void OutputCommon(IOutputStream& out) const;
        void Output(IOutputStream& out) const;

    private:
        TDssmBoostingConfig Config_;
    };

} // namespace NVectorMachine
