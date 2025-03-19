#include "bundle.h"
#include <util/generic/algorithm.h>
#include <util/string/builder.h>

TString ToString(const NMatrixnet::TRankModelVector& v) {
    TStringBuilder result;

    for (const auto& it : v) {
        if (it.Matrixnet == nullptr) {
            continue;
        }
        if (!result.empty()) {
            result << '+';
        }
        result << it.Renorm.GetMult() << '*';
        if (auto fmlIdPtr = it.Matrixnet->Info.FindPtr("formula-id")) {
            result << *fmlIdPtr;
        } else {
            result << "_no_id_";
        }

        if (Abs(it.Renorm.GetAdd()) >= 1e-6) {
            result << '+' << it.Renorm.GetAdd();
        }
    }

    return result;
};

namespace NMatrixnet {
    TModelInfo InitBundleInfo(const TRankModelVector& matrixnets, const TModelInfo& info) {
        TModelInfo result(info);
        for (size_t i = 0; i < matrixnets.size(); ++i) {
            const TString prefix = TStringBuilder() << '~' << i << '_';
            if (matrixnets[i].Matrixnet == nullptr) {
                result[prefix + "no_matrixnet"] = "1";
                continue;
            }
            result[prefix + "scale"] = TStringBuilder() << matrixnets[i].Renorm.GetMult();
            result[prefix + "bias"] = TStringBuilder() << matrixnets[i].Renorm.GetAdd();
            const TModelInfo* elementInfo = matrixnets[i].Matrixnet->GetInfo();
            if (elementInfo == nullptr) {
                continue;
            }
            for (const auto& prop : *elementInfo) {
                result[prefix + prop.first] = prop.second;
            }
        }
        return result;
    }

    TBundle::TBundle(const TRankModelVector& matrixnets, const TModelInfo& info)
        : Matrixnets(matrixnets)
        , ModelInfo(InitBundleInfo(Matrixnets, info))
    {
        Verify();
    }

    void TBundle::Verify() {
        for (const auto& element : Matrixnets) {
            Y_ENSURE(element.Matrixnet, "Can't construct bundle with null matrixnet");
        }
    }

    size_t TBundle::GetNumFeats() const {
        size_t feats = 0;
        for (const auto& element : Matrixnets) {
            feats = Max(feats, element.Matrixnet->GetNumFeats());
        }
        return feats;
    }

    double TBundle::DoCalcRelev(const float* features) const {
        double result = 0.0;
        for (const auto& element : Matrixnets) {
            const double score = element.Matrixnet->DoCalcRelev(features);
            result += element.Renorm.Apply(score);
        }
        return result;
    }

    void TBundle::DoCalcRelevs(const float* const* features, double* relevs, const size_t numDocs) const {
        Fill(relevs, relevs + numDocs, 0.0);
        for (const auto& element : Matrixnets) {
            TVector<double> scores(numDocs, 0.0);
            element.Matrixnet->DoCalcRelevs(features, &scores[0], numDocs);
            for (size_t i = 0; i < numDocs; ++i) {
                relevs[i] += element.Renorm.Apply(scores[i]);
            }
        }
    }

    void TBundle::DoSlicedCalcRelevs(const TFactorStorage* const* features, double* relevs, size_t numDocs) const {
        Fill(relevs, relevs + numDocs, 0.0);
        TVector<double> scores(numDocs, 0.0);
        for (const auto& element : Matrixnets) {
            element.Matrixnet->DoSlicedCalcRelevs(features, &scores[0], numDocs);
            for (size_t i = 0; i < numDocs; ++i) {
                relevs[i] += element.Renorm.Apply(scores[i]);
            }
        }
    }

    const TModelInfo* TBundle::GetInfo() const {
        return &ModelInfo;
    }

    void TBundle::UsedFactors(TSet<ui32>& factors) const {
        factors.clear();
        TSet<ui32> tmpFactors;
        for (const auto& mxnet : Matrixnets) {
            mxnet.Matrixnet->UsedFactors(tmpFactors);
            factors.insert(tmpFactors.begin(), tmpFactors.end());
        }
    }

    void TBundle::UsedFactors(TSet<NFactorSlices::TFullFactorIndex>& factors) const {
        factors.clear();
        TSet<NFactorSlices::TFullFactorIndex> tmpFactors;
        for (const auto& mxnet : Matrixnets) {
            mxnet.Matrixnet->UsedFactors(tmpFactors);
            factors.insert(tmpFactors.begin(), tmpFactors.end());
        }
    }

    TString TBundle::GetId() const {
       return ToString(Matrixnets);
    }

    NMatrixnet::TBundleDescription TBundle::ParseBundle(const NJson::TJsonValue::TMapType& sum, const NJson::TJsonValue::TMapType& bundleInfo) {
        NMatrixnet::TBundleDescription bundle;

        const auto getDoubleSafe = [&bundleInfo](const TString& key, const double deflt) {
            if (bundleInfo.count(key) == 0) {
                return deflt;
            }
            return bundleInfo.at(key).GetDoubleSafe(deflt);
        };
        double add = 0.0;
        double mult = 1.0;

        // work with normalized json-bundles
        if (bundleInfo.count("mult") > 0 &&
            bundleInfo.count("add") > 0) {
            mult = getDoubleSafe("mult", 1.0);
            add = getDoubleSafe("add", 0.0);
        } else if (bundleInfo.count("scale") > 0 &&
            bundleInfo.count("native-scale") > 0 &&
            bundleInfo.count("bias") > 0) {

            double scale = getDoubleSafe("scale", 1.0);
            double scaleNative = getDoubleSafe("native-scale", 1.0);
            double bias = getDoubleSafe("bias", 0.0);
            mult = scaleNative / scale;
            add = -bias * mult;
        }

        bool addedBias = false;
        for (const auto& element : sum) {
            const TString& matrixnet = element.first;
            const double weight = element.second.GetDoubleSafe();

            NMatrixnet::TBundleRenorm renorm(weight * mult, 0.0);
            if (!addedBias) {
                addedBias = true;
                renorm.SetAdd(add);
            }

            bundle.Elements.emplace_back(matrixnet, renorm);
        }

        for (const auto& infoProp : bundleInfo) {
            if (infoProp.second.IsString()) {
                bundle.Info[infoProp.first] = infoProp.second.GetString();
            }
        }

        return bundle;
    }
} // NMatrixnet
