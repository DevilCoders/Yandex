#pragma once
#include <util/generic/string.h>

namespace NUtil {
    TString DetectDatacenterCode(const TString& hostName, const TString& defaultValue = "MSK");

}
