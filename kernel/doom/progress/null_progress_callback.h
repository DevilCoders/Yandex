#pragma once

#include <util/generic/string.h>

#include "progress.h"

namespace NDoom {


class TNullProgressCallback {
public:
    void Restart(const char*) {}
    void Restart(const TString&) {}
    void Step(ui64, ui64) {}
    void Step(const TProgress&) {}
    void Step() {}
};


} // namespace NDoom
