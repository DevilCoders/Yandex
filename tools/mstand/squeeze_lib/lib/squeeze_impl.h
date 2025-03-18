#pragma once

#include "common.h"

#include <util/generic/vector.h>
#include <util/generic/string.h>


namespace NMstand {

typedef THashMap<TString, ui32> TVersions;

TVersions GetSqueezeVersions(const TString& service);
TString CreateTempTable(const TString& service);
TString SqueezeDay(
    const TVector<TExperimentForSqueeze>& experiments,
    const TYtParams& ytParams,
    const TFilterMap& filters
);

};
