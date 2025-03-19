#pragma once

#include <kernel/dssm_applier/nn_applier/lib/layers.h>

#include <util/generic/ptr.h>

namespace NNeuralNetOps {

    class TDssmShowsModel {
    public:
        void Init(const TString& modelPath);

        TVector<float> Predict(const TString& host, const TString& path, const TString& title = TString()) const;

        float PredictLogShows(const TString& host, const TString& path) const;

    private:
        TAtomicSharedPtr<NNeuralNetApplier::ISample> ConstructSample(const TString& host, const TString& path, const TString& title = TString()) const;

    private:
        NNeuralNetApplier::TModel Model;
    };

} // NNeuralNetOps
