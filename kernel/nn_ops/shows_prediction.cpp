#include "shows_prediction.h"

#include <library/cpp/string_utils/quote/quote.h>

#include <library/cpp/charset/wide.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>

namespace NNeuralNetOps {

    void TDssmShowsModel::Init(const TString& modelPath) {
        const TBlob& blob = TBlob::FromFile(modelPath);
        Model.Load(blob);
        Model.Init();
    }

    TVector<float> TDssmShowsModel::Predict(const TString& host, const TString& path, const TString& title) const {
        TAtomicSharedPtr<NNeuralNetApplier::ISample> sample = ConstructSample(host, path, title);
        TVector<float> modelResult;
        Model.Apply(std::move(sample), modelResult);
        return modelResult;
    }

    float TDssmShowsModel::PredictLogShows(const TString& host, const TString& path) const {
        return Predict(host, path).front();
    }

    TAtomicSharedPtr<NNeuralNetApplier::ISample> TDssmShowsModel::ConstructSample(const TString& host, const TString& path, const TString& title) const {
        static const TVector<TString> annotations = {"host", "path", "empty", "title"};

        TStringBuf fixedPath(path);
        fixedPath.Skip(1); // skip leading '/' because of model specificity
        TString pathUnscaped(fixedPath.data(), fixedPath.length());
        UrlUnescape(pathUnscaped);

        TVector<wchar16> dest(pathUnscaped.size());
        size_t written = 0;
        UTF8ToWide(pathUnscaped.data(), pathUnscaped.size(), dest.data(), written);
        pathUnscaped = WideToUTF8(dest.data(), written);

        const TVector<TString> values = {host, pathUnscaped, TString(), title};
        return new NNeuralNetApplier::TSample(annotations, values);
    }

} // NNeuralNetOps
