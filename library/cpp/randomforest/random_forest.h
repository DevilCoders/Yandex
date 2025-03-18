#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <utility>

class TRandomTree;

class TRandomForest {
public:
    TRandomForest();
    ~TRandomForest();

    bool ReadRF(const TString& prefix, size_t trees = 0);
    bool ReadRF(TBlob blobs[6], size_t trees = 0);
    bool Predict(const TVector<unsigned>& factors, TVector<float>& res) const;

private:
    TVector<TAutoPtr<TRandomTree>> Forest;
    float Invfs;
};
