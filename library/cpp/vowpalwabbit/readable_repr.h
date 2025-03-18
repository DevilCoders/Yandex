#pragma once

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <library/cpp/deprecated/split/split_iterator.h>

namespace NVowpalWabbit {
    using TModelAsHash = THashMap<TString, float>;

    TModelAsHash LoadReadableModel(const TString& modelFile);
    float ApplyReadableModel(const TModelAsHash& model, const TVector<TString>& sample, size_t order);

}
