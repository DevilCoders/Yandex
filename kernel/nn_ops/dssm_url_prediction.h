#pragma once

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <util/generic/ptr.h>

namespace NNeuralNetOps {

    class TDssmUrlPredictionModel {
    public:
        void Init(const TString& modelPath, bool isGPUDSSMClassifier = false);

        TVector<float> Predict(const TString& url) const;

        float PredictFloat(const TString& url) const;

    private:
        TAtomicSharedPtr<NNeuralNetApplier::ISample> ConstructSample(const TString& url) const;
        TAtomicSharedPtr<NNeuralNetApplier::ISample> ConstructSampleGPUDSSMClassifier(const TString& url) const;

    private:
        NNeuralNetApplier::TModel Model;
        bool IsGPUDSSMClassifier = false;
    };

} // NNeuralNetOps
