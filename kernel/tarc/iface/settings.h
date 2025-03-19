#pragma once
#include <util/generic/singleton.h>

struct TArchiveSettings {

    bool DocIdCheckerEnabled;

    TArchiveSettings() {
        DocIdCheckerEnabled = true;
    }
};
