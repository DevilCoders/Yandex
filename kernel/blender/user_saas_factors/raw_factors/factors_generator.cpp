#include "factors_generator.h"

#include <kernel/blender/user_saas_factors/protos/user_saas_factors.pb.h>
#include <kernel/dssm_applier/nn_applier/lib/states.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/hash.h>
#include <util/string/cast.h>
#include <util/string/builder.h>

namespace {
    float SafeDivision(const float a, const float b) {
        if (abs(b) < std::numeric_limits<float>::epsilon()) {
            return 0;
        } else {
            return a / b;
        }
    }

    bool IsIntentFactorsEnabled(const NBlenderUserFactors::NProto::TIntentFactors& intentFactors) {
        if (!intentFactors.HasIntent()) {
            return false;
        }
        if (intentFactors.HasDisabled()) {
            return !intentFactors.GetDisabled();
        } else {
            return true;
        }
    }

    NBlender::NUserFactors::TRawIntentFactors GenerateRawIntentFactors(const NBlenderUserFactors::NProto::TIntentFactors& intentFactors) {
        using namespace NBlenderUserFactors::NProto;
        THashMap<int, float> factorsByType;
        for (const auto& factor : intentFactors.GetFactors()) {
            if (factor.HasType()) {
                factorsByType[factor.GetType()] = factor.GetValue();
            }
        }
        // new factors should be added only in the end of orderedFactors
        static const TVector orderedFactors = {
            TFactor::FT_CLICK_TYPE,
            TFactor::FT_CLICK_WIN_TYPE,
            TFactor::FT_CLICK_LOSS_TYPE,
            TFactor::FT_NOCLICK_WIN_TYPE,
            TFactor::FT_CROSS_LOSS_TYPE,
            TFactor::FT_INVERSION_LOSS_TYPE,
            TFactor::FT_RERE_LOSS_TYPE,
            TFactor::FT_SHORT_CLICK_PENALTY_TYPE,
            TFactor::FT_WIN_TYPE,
            TFactor::FT_LOSS_TYPE
        };
        NBlender::NUserFactors::TRawIntentFactors result(intentFactors.GetIntent(), intentFactors.GetVersion());
        result.Factors.reserve(3 + 2 * orderedFactors.size());
        const float shows = factorsByType.Value(TFactor::FT_SHOW_TYPE, 0);
        const float surplus = factorsByType.Value(TFactor::FT_WIN_TYPE, 0) - factorsByType.Value(TFactor::FT_LOSS_TYPE, 0);
        result.Factors.push_back(shows);
        result.Factors.push_back(surplus);
        result.Factors.push_back(SafeDivision(surplus, shows));
        for (const auto& factorType : orderedFactors) {
            const float factorValue = factorsByType.Value(factorType, 0);
            result.Factors.push_back(factorValue);
            result.Factors.push_back(SafeDivision(factorValue, shows));
        }
        return result;
    }
}

namespace NBlender::NUserFactors {
    TString TRawIntentFactors::GetFactorsId() const {
        return TStringBuilder() << Intent << ".v" << ToString<int>(Version);
    }

    TRawFactors GenerateRawFactors(const NBlenderUserFactors::NProto::TUserFactors& userFactors) {
        NBlender::NUserFactors::TRawFactors result;
        result.reserve(userFactors.IntentFactorsSize());
        for (const auto& intentFactors : userFactors.GetIntentFactors()) {
            if (IsIntentFactorsEnabled(intentFactors)) {
                result.push_back(GenerateRawIntentFactors(intentFactors));
            }
        }
        return result;
    }

    TRawEmbeds GenerateRawEmbeds(const NBlenderUserFactors::NProto::TUserFactors& userFactors) {
        return TRawEmbeds(userFactors.GetEmbeds().begin(), userFactors.GetEmbeds().end());
    }

    TEmbedsValuesMap RawEmbedsToEmbedsValuesMap(const TRawEmbeds& rawEmbeds) {
        TEmbedsValuesMap result;
        for (const auto& embed : rawEmbeds) {
            TIntrusivePtr<NNeuralNetApplier::TCharMatrix>  matrixPtr = new NNeuralNetApplier::TCharMatrix();
            matrixPtr->Resize(1, embed.GetEmbed().Size());
            matrixPtr->SetData(TVector<ui8>(embed.GetEmbed().begin(), embed.GetEmbed().end()));
            result[embed.GetFactorId()] = matrixPtr;
        }
        return result;
    }
}
