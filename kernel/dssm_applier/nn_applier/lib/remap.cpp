#include "remap.h"


namespace NNeuralNetApplier {

    TVector<TVector<float>> Transpose(const TVector<TVector<float>>& matrix) {
        Y_ENSURE(matrix.size() > 0);
        TVector<TVector<float>> result(matrix[0].size(), TVector<float>(matrix.size()));
        for (size_t i = 0; i < matrix.size(); ++i) {
            for (size_t j = 0; j < matrix[0].size(); ++j) {
                result[j][i] = matrix[i][j];
            }
        }
        return result;
    }

    void Subsample(const TVector<TVector<float>>& from, TVector<TVector<float>>& to, ui64 numSamples) {
        auto transposed = Transpose(from);

        Y_ENSURE(transposed[0].size() >= numSamples);
        Y_ENSURE(numSamples >= 2);

        for (ui64 i = 0; i < transposed.size(); ++i) {
            Sort(transposed[i].begin(), transposed[i].end());
        }
        ui64 maxIdx = transposed[0].size() - 1;
        for(ui64 i = 0; i < transposed[0].size(); i += (numSamples > to[0].size() ? (maxIdx - i) / (numSamples - to[0].size()) : 1)) {
            for (ui64 j = 0; j < transposed.size(); ++j) {
                to[j].push_back(transposed[j][i]);
            }
        }
        Y_ENSURE(to[0].size() == numSamples);
    }

    TModelPtr DoRemap(const TModel& fromModel, const TModel& toModel, const TString& dataset, const TString& fromOutput,
        const TString& toOutput, const TVector<TString>& fromHeaderFields, const TVector<TString>& toHeaderFields,
        const ui64 numPoints)
    {
        Cerr << "Applying first model\n";
        TVector<TVector<float>> fromValues;
        ApplyModel(fromModel, dataset, fromHeaderFields, { fromOutput }, fromValues);

        Cerr << "Applying second model\n";
        TVector<TVector<float>> toValues;
        ApplyModel(toModel, dataset, toHeaderFields, { toOutput }, toValues);

        Cerr << "Remapping outputs\n";
        Y_ENSURE(fromValues.size() == toValues.size());
        for (ui64 output = 0; output < toValues.size(); ++output) {
            Y_ENSURE(fromValues[output].size() == toValues[output].size());
        }

        TVector<TVector<float>> mapFrom(fromValues[0].size());
        TVector<TVector<float>> mapTo(toValues[0].size());

        Subsample(fromValues, mapFrom, numPoints);
        Subsample(toValues, mapTo, numPoints);

        for (ui64 i = 0; i < mapFrom.size(); ++i) {
            int lastPoint = 0;
            for (ui64 j = 1; j < mapFrom[i].size(); ++j) {
                if (mapFrom[i][j] - mapFrom[i][lastPoint] > 1e-5) {
                    ++lastPoint;
                    mapFrom[i][lastPoint] = mapFrom[i][j];
                    mapTo[i][lastPoint] = mapTo[i][j];
                }
            }
            mapFrom[i].resize(lastPoint + 1);
            mapTo[i].resize(lastPoint + 1);
        }

        TStringStream stringStream;
        fromModel.Save(&stringStream);
        TBlob blob = TBlob::FromStream(stringStream);
        TModelPtr resultModel = new TModel();
        resultModel->Load(blob);

        resultModel->RenameVariable(fromOutput, fromOutput + "_unmapped");
        resultModel->Layers.push_back(new TRemapLayer(
                    fromOutput + "_unmapped", fromOutput, mapFrom, mapTo));
        resultModel->Init();
        return resultModel;
    }

} // namespace NNeuralNetApplier
