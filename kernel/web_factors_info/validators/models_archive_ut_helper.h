#pragma once

#include "kernel/factor_slices/factor_slices_gen.h"
#include <kernel/generated_factors_info/metadata/factors_metadata.pb.h>
#include <search/formula_chooser/metadata/metadata.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NModelsArchiveValidators {

    void CheckForFactorsByTag(
        const NFactor::ETag& tagToCheck,
        const THashMap<NFormulaChooser::NProto::EVariable, TVector<NFactor::ETag>>& exclusiveRules = {},
        const THashSet<TString>& skipModelIds = {},
        bool onlyDefaultMapping = false
    ) noexcept(false);

    void CheckDependenciesOnDeprecated(const THashSet<NFactorSlices::EFactorSlice>& slicesWillSkip) noexcept(false);

}
