#include "common.h"

#include <kernel/vector_machine/codegen/config.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/stream/output.h>

namespace NVectorMachine {

    TBoosterGenerator::TBoosterGenerator(const TBoosterConfig& config, const TVector<TGeneratedCalcer>& calcers)
            : Config_(config)
            , Calcers_(calcers)
        {
            TMaybe<size_t> nWeights;
            for (const TGeneratedCalcer& calcer : Calcers_) {
                FeaturesCount_ += calcer.GetFeatureNames().size();
                Y_ENSURE(!nWeights.Defined() || (calcer.GetNWeights() == *nWeights), "calcer weights size differs");
                NWeights_ = calcer.GetNWeights();
            }
        }

    void TBoosterGenerator::Output(IOutputStream& out) const {
        out << "class " << Config_.GetClassName() << " {" << Endl;
        out << "public:" << Endl;
        OutputEnum(out);
        out << Endl;
        OutputCalcFeatures(out);
        out << Endl;
        OutputCalcQueryFactor(out);
        out << Endl;
        OutputSizeGetters(out);
        out << Endl;
        out << "private:" << Endl;
        OutputCalcerMembers(out);
        out << "};" << Endl;
    }

    void TBoosterGenerator::OutputEnum(IOutputStream& out) const {
        out << "    enum EFeatures {" << Endl;
        size_t featurePos = 0;
        for (const TGeneratedCalcer& calcer : Calcers_) {
            for (const auto& featureName : calcer.GetFeatureNames()) {
                out << "        " << featureName << " = " << featurePos++ << "," << Endl;
            }
        }
        out << "    };" << Endl;
    }

    void TBoosterGenerator::OutputCalcerMembers(IOutputStream& out) const {
        size_t calcerNum = 0;
        for (const TGeneratedCalcer& calcer : Calcers_) {
            out << "    " << calcer.GetClassName() << " Calcer" << calcerNum++ << ";" << Endl;
        }
    };

    void TBoosterGenerator::OutputCalcFeatures(IOutputStream& out) const {
        out << "    void CalcFeatures(const TVector<TEmbed>& queryEmbeds, const TVector<TEmbed>& docEmbeds, const TVector<TWeights>& weights, TArrayRef<float> result) const {" << Endl;
        out << "        Y_ENSURE(result.size() == " << FeaturesCount_ << ");" << Endl;
        out << "        Y_ENSURE(queryEmbeds.size() == " << Calcers_.size() << ");" << Endl;
        size_t offset = 0;
        size_t calcerNum = 0;
        for (const TGeneratedCalcer& calcer : Calcers_) {
            size_t featureCount = calcer.GetFeatureNames().size();
            out << "        if (Y_LIKELY(queryEmbeds[" << calcerNum << "])) {" << Endl;
            out << "            Calcer" << calcerNum << ".CalcFeatures(queryEmbeds[" << calcerNum << "], docEmbeds, weights, result.Slice(" << offset << ", " << featureCount << "));" << Endl;
            out << "        }" << Endl;
            offset += featureCount;
            calcerNum++;
        }
        out << "    }" << Endl;
    }

    void TBoosterGenerator::OutputCalcQueryFactor(IOutputStream& out) const {
        out << "    float CalcQueryFactor(const TEmbed& firstQueryEmbed, const TEmbed& secondQueryEmbed) const {" << Endl;
        out << "        return Calcer0.CalcSimilarity(firstQueryEmbed, secondQueryEmbed);" << Endl;
        out << "    }" << Endl;
    }

    void TBoosterGenerator::OutputSizeGetters(IOutputStream& out) const {
        out << "    size_t GetNFeatures() const {" << Endl;
        out << "         return " << FeaturesCount_ << ";" << Endl;
        out << "    }" << Endl << Endl;
        out << "    size_t GetNWeights() const {" << Endl;
        out << "         return " << NWeights_ << ";" << Endl;
        out << "    }" << Endl << Endl;
        out << "    size_t GetRequiredQueryEmbeddingsCount() const {" << Endl;
        out << "         return " << Calcers_.size() << ";" << Endl;
        out << "    }" << Endl << Endl;
    }

    TDssmBoostingGenerator::TDssmBoostingGenerator(const TDssmBoostingConfig& config)
        : Config_(config)
    {
    }

    void TDssmBoostingGenerator::OutputCommon(IOutputStream& out) const {
        out << "#pragma once" << Endl;
        out << "#include <kernel/dssm_applier/begemot/production_data.h>" << Endl;
        out << "#include <kernel/vector_machine/common.h>" << Endl;
        out << "#include <kernel/vector_machine/transforms.h>" << Endl;
        out << "#include <kernel/vector_machine/similarities.h>" << Endl;
        out << "#include <kernel/vector_machine/slicers.h>" << Endl;
        out << "#include <kernel/vector_machine/aggregators.h>" << Endl;
        out << "#include <util/generic/array_ref.h>" << Endl;
        out << Endl;
    }


    void TDssmBoostingGenerator::Output(IOutputStream& out) const {
        OutputCommon(out);
        out << "namespace NVectorMachine {" << Endl;

        THashMap<TString, TGeneratedCalcer> calcers;
        for (const TCalcerConfig& calcerConfig : Config_.GetCalcerConfigs()) {
            TGeneratedCalcer calcer(calcerConfig);
            calcers.emplace(calcerConfig.GetClassName(), calcer );
            calcer.Output(out);
            out << Endl;
        }

        for (const TBoosterConfig& boosterConfig : Config_.GetBoosterConfigs()) {
            TVector<TGeneratedCalcer> boosterCalcers;
            for (const TString& calcerClassName : boosterConfig.GetCalcerClassNames()) {
                boosterCalcers.push_back(calcers.at(calcerClassName));
            }

            TBoosterGenerator(boosterConfig, boosterCalcers).Output(out);
            out << Endl;
        }

        out << "}" << Endl;
    }

} // namespace NVectorMachine
