#include "factor_storage_extractor.h"


namespace {
    struct TSliceData {
        float* Data = nullptr;
        size_t Size = 0;
        TString Name;
    };

    TSliceData ExtractData(const THolder<TFactorStorage>& factorStorage, const EFactorSlice& slice) {
        TFactorView factorView = factorStorage->CreateViewFor(slice);
        TString fullSliceName = ToCppString(slice);
        const auto& splittedName = StringSplitter(fullSliceName).Split(':').ToList<TString>();
        Y_ENSURE_EX(
            splittedName.size() > 0,
            yexception() << "empty slice name \"" << fullSliceName << '\"'
        );
        TString lastName = splittedName[splittedName.size() - 1];
        lastName.to_lower();
        return {~factorView, factorView.Size(), lastName};
    }
}

NNeuralRanker::TVectorMap NNeuralRanker::ConvertToVector(
    const THolder<TFactorStorage>& factorStorage)
{
    TVectorMap map;
    TFactorDomain factorDomain = factorStorage->GetDomain();
    for (auto it = factorDomain.Begin(); it.Valid(); it.NextLeaf()) {
        TSliceData sliceData = ExtractData(factorStorage, it.GetLeaf());
        map["Slice_" + sliceData.Name].resize(sliceData.Size);
        MemMove(map["Slice_" + sliceData.Name].begin(), sliceData.Data, sliceData.Size);
    }
    return map;
}


NNeuralRanker::TArrayRefMap NNeuralRanker::ConvertToArrayRef(
    const THolder<TFactorStorage>& factorStorage)
{
    TArrayRefMap map;
    TFactorDomain factorDomain = factorStorage->GetDomain();
    for (auto it = factorDomain.Begin(); it.Valid(); it.NextLeaf()) {
        TSliceData sliceData = ExtractData(factorStorage, it.GetLeaf());
        map["Slice_" + sliceData.Name] = TConstArrayRef<float>(sliceData.Data, sliceData.Size);
    }
    return map;
}
