#include "dssm_url_prediction.h"

#include <library/cpp/string_utils/quote/quote.h>

#include <library/cpp/charset/wide.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>

namespace NNeuralNetOps {

    void TDssmUrlPredictionModel::Init(const TString& modelPath, bool isGPUDSSMClassifier) {
        const TBlob& blob = TBlob::FromFile(modelPath);
        Model.Load(blob);
        Model.Init();
        IsGPUDSSMClassifier = isGPUDSSMClassifier;
    }

    TVector<float> TDssmUrlPredictionModel::Predict(const TString& url) const {
        TAtomicSharedPtr<NNeuralNetApplier::ISample> sample = IsGPUDSSMClassifier ? ConstructSampleGPUDSSMClassifier(url) : ConstructSample(url);
        TVector<float> modelResult;
        Model.Apply(std::move(sample), modelResult);
        return modelResult;
    }

    float TDssmUrlPredictionModel::PredictFloat(const TString& url) const {
        return Predict(url).front();
    }

    TAtomicSharedPtr<NNeuralNetApplier::ISample> TDssmUrlPredictionModel::ConstructSample(const TString& url) const {
        static const TVector<TString> annotations = {"url", "fakequery"};
        const TVector<TString> values = {url, TString()};
        return new NNeuralNetApplier::TSample(annotations, values);
    }

    TAtomicSharedPtr<NNeuralNetApplier::ISample> TDssmUrlPredictionModel::ConstructSampleGPUDSSMClassifier(const TString& url) const {
        static const TVector<TString> annotations = {"url"};
        const TVector<TString> values = {url};
        return new NNeuralNetApplier::TSample(annotations, values);
    }

} // NNeuralNetOps
