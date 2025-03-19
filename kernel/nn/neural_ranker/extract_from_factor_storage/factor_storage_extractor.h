#pragma once

#include <kernel/factor_storage/factor_storage.h>
#include <kernel/factor_slices/factor_slices.h>

#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/split.h>


namespace NNeuralRanker {
    using TArrayRefMap = THashMap<TString, TConstArrayRef<float>>;
    using TVectorMap = THashMap<TString, TVector<float>>;

    TArrayRefMap ConvertToArrayRef(const THolder<TFactorStorage>& factorStorage);
    TVectorMap ConvertToVector(const THolder<TFactorStorage>& factorStorage);
}
