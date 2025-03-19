#include "mn_standalone_binarization.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

namespace {
    TVector<NMatrixnet::TBinaryFeature> InitFeaturesFromModel(const NMatrixnet::TMnSseInfo& model) {
        TVector<NMatrixnet::TBinaryFeature> res;
        NMatrixnet::TFactorsMap factorsMap;
        model.FactorsMap(factorsMap);
        NMatrixnet::TBorders borders;
        model.Borders(borders);
        for (const auto& [index, fs] : borders) {
            auto factorIndex = factorsMap.at(index);
            for (float f : fs) {
                res.push_back({ factorIndex, f });
            }
        }
        return res;
    }

    NMLPool::TFeatureSlices InitSlicesFromFeatures(const TVector<NMatrixnet::TBinaryFeature>& features) {
        NMLPool::TFeatureSlices res;
        for (size_t i : xrange(features.size())) {
            if (i == 0 || features[i].Index.Slice != features[i - 1].Index.Slice) {
                if (i == 0) {
                    res.push_back({ ToString(features[i].Index.Slice), 0, 0 });
                } else {
                    res.push_back({ ToString(features[i].Index.Slice), res.back().End, 0 });
                }
            }
            res.back().End = res.back().Begin + features[i].Index.Index + 1;
        }
        return res;
    }
}

namespace NMatrixnet {

TStandaloneBinarization TStandaloneBinarization::From(const TMnSseInfo& model) {
    TStandaloneBinarization res;
    res.Features = InitFeaturesFromModel(model);
    Sort(res.Features);
    res.Slices = InitSlicesFromFeatures(res.Features);
    return res;
}

void TStandaloneBinarization::MergeWith(const TStandaloneBinarization& other) {
    TVector<TBinaryFeature> resFeatures;
    std::set_union(
        Features.begin(), Features.end(),
        other.Features.begin(), other.Features.end(),
        std::back_inserter(resFeatures)
    );
    Features = resFeatures;
    Slices = InitSlicesFromFeatures(Features);
}

size_t TStandaloneBinarization::GetNumOfFeatures() const {
    return Features.size();
}

const TVector<TBinaryFeature>& TStandaloneBinarization::GetFeatures() const {
    return Features;
}

const NMLPool::TFeatureSlices& TStandaloneBinarization::GetSlices() const {
    return Slices;
}

TMnSseDynamic TStandaloneBinarization::RebuildModel(const TMnSseInfo& model) const {
    auto modelFeatures = InitFeaturesFromModel(model);
    TVector<TFeature> features;
    TVector<float> values;

    {
        auto sliceIt = Slices.begin();
        auto curSlice = FromString<NFactorSlices::EFactorSlice>(sliceIt->Name);
        for (const auto& f : Features) {
            if (f.Index.Slice != curSlice) {
                ++sliceIt;
                Y_ASSERT(sliceIt != Slices.end());
                curSlice = FromString<NFactorSlices::EFactorSlice>(sliceIt->Name);
            }
            ui32 curIndex = sliceIt->Begin + f.Index.Index;
            if (features.empty() || features.back().Index != curIndex) {
                features.push_back({ curIndex, 1 });
            } else {
                ++features.back().Length;
            }
            values.push_back(f.Border);
        }
    }

    TMap<ui32, ui32> remapIndices;
    {
        auto itFrom = Features.begin();
        auto itTo = Features.begin();
        float minusInfinity = -std::numeric_limits<float>::infinity();
        for (size_t i : xrange(modelFeatures.size())) {
            if (i == 0 || !(modelFeatures[i].Index == modelFeatures[i - 1].Index)) {
                TBinaryFeature from { modelFeatures[i].Index, minusInfinity };
                TBinaryFeature to { { modelFeatures[i].Index.Slice, modelFeatures[i].Index.Index + 1 }, minusInfinity };
                itFrom = std::lower_bound(itTo, Features.end(), from);
                itTo = std::lower_bound(itFrom, Features.end(), to);
            }
            auto it = std::lower_bound(itFrom, itTo, modelFeatures[i]);
            Y_ASSERT(it != Features.end() && *it == modelFeatures[i]);
            remapIndices[i] = it - Features.begin();
        }
    }

    const auto& meta = model.GetSseDataPtrs().Meta;
    const auto* multiData = std::get_if<TMultiData>(&model.GetSseDataPtrs().Leaves.Data);
    const auto& leafData = multiData->MultiData[0].Data;
    size_t leafDataSize = multiData->DataSize;

    TVector<ui32> newDataIndices;
    for (size_t i : xrange(meta.DataIndicesSize)) {
        newDataIndices.push_back(4 * remapIndices.at(meta.GetIndex(i)));
    }

    TMnSseDynamic res(
        values.data(), values.size(),
        features.data(), features.size(),
        meta.MissedValueDirections, meta.MissedValueDirectionsSize,
        newDataIndices.data(), newDataIndices.size(),
        meta.SizeToCount, meta.SizeToCountSize,
        leafData, leafDataSize,
        model.Bias(), model.Scale()
    );
    if (meta.DynamicBundle) {
        res.SetDynamicBundle({ TVector<TDynamicBundleComponent>(meta.DynamicBundle.begin(), meta.DynamicBundle.end()) });
    }
    res.SetInfo("Slices", ToString(Slices));
    res.SetSlicesFromInfo();

    return res;
}

void TStandaloneBinarization::Load(IInputStream* in) {
    Features.clear();

    auto json = NJson::ReadJsonTree(in);
    const auto& ar = json["features"].GetArray();
    for (const auto& f : ar) {
        const auto& index = f["featureIndex"];
        Features.push_back({
            {
                FromString<NFactorSlices::EFactorSlice>(index["slice"].GetString()),
                static_cast<NFactorSlices::TFactorIndex>(index["index"].GetInteger())
            },
            static_cast<float>(f["border"].GetDouble())
        });
    }
    Slices = InitSlicesFromFeatures(Features);
}

void TStandaloneBinarization::Save(IOutputStream* out) const {
    NJson::TJsonValue features;
    for (const auto& f : Features) {
        auto& featureJson = features.AppendValue({});
        auto& indexJson = featureJson["featureIndex"];
        indexJson["slice"] = ToString<NFactorSlices::EFactorSlice>(f.Index.Slice);
        indexJson["index"] = f.Index.Index;
        featureJson["border"] = f.Border;
    }
    NJson::TJsonValue json;
    NJson::TJsonWriterConfig config;
    config.FloatNDigits = std::numeric_limits<float>::max_digits10;
    json["features"] = features;
    NJson::WriteJson(out, &json, config);
}

} // namespace NMatrixnet
