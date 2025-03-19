#pragma once

#include <util/generic/string.h>

const TString BASE = "base";
const TString PRE = "pre";
const TString PROD = "prod";
const TString TEST = "test";
const TString DEV = "dev";

namespace NUgc {
    bool IsDeploymentMode(const TString& mode);
} // namespace NUgc
