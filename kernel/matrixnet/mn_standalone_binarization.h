#pragma once

#include "mn_dynamic.h"

namespace NMatrixnet {

struct TBinaryFeature {
    NFactorSlices::TFullFactorIndex Index;
    float Border = 0;

    auto GetKey() const {
        return std::forward_as_tuple(Index, Border);
    }

    bool operator<(const TBinaryFeature& other) const {
        return GetKey() < other.GetKey();
    }

    bool operator==(const TBinaryFeature& other) const {
        return GetKey() == other.GetKey();
    }
};

class TStandaloneBinarization {
private:
    TVector<TBinaryFeature> Features;
    NMLPool::TFeatureSlices Slices;
public:
    static TStandaloneBinarization From(const TMnSseInfo& model);

    void MergeWith(const TStandaloneBinarization& other);

    size_t GetNumOfFeatures() const;

    const TVector<TBinaryFeature>& GetFeatures() const;

    const NMLPool::TFeatureSlices& GetSlices() const;

    TMnSseDynamic RebuildModel(const TMnSseInfo& model) const;

    void Load(IInputStream* in);

    void Save(IOutputStream* out) const;
};

} // namespace NMatrixnet
