#include "apply.h"

#include <util/string/split.h>


namespace NNeuralNetApplier {

    void ApplyModel(const TModel& model, const TString& dataset, const TVector<TString>& headerFields,
        const TVector<TString>& outputVariables, TVector<TVector<float>>& result, const ui32 batchSize)
    {
        result.clear();
        TString line;
        TFileInput file(dataset);
        TVector<TAtomicSharedPtr<ISample>> samples;
        while (file.ReadLine(line)) {
            TVector<TString> values;
            StringSplitter(line).Split('\t').AddTo(&values);
            Y_ENSURE(values.size() <= headerFields.size(), values.size() << " > " << headerFields.size());
            samples.emplace_back(new TSample(headerFields, values));
            if (samples.size() == batchSize) {
                TVector<TVector<float>> batchResult;
                model.Apply(samples, outputVariables, batchResult);
                samples.clear();
                std::move(batchResult.begin(), batchResult.end(), std::back_inserter(result));
            }
        }
        if (!samples.empty()) {
            TVector<TVector<float>> batchResult;
            model.Apply(samples, outputVariables, batchResult);
            std::move(batchResult.begin(), batchResult.end(), std::back_inserter(result));
        }
    }

    void ApplyModel(const TString& modelFile, const TString& dataset, const TVector<TString>& headerFields,
        const TVector<TString>& outputVariables, TVector<TVector<float>>& result, const ui32 batchSize)
    {
        TBlob blob = TBlob::FromFile(modelFile);
        TModel model;
        model.Load(blob);
        if (outputVariables.size() == 1) {
            model = *(model.GetSubmodel(outputVariables[0]));
        }
        ApplyModel(model, dataset, headerFields, outputVariables, result, batchSize);
    }

} // namespace NNeuralNetApplier
