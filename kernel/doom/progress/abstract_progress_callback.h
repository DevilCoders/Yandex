#pragma once

#include <util/generic/string.h>

#include "progress.h"

namespace NDoom {


class TAbstractProgressCallback {
public:
    virtual ~TAbstractProgressCallback() {}

    virtual void Restart(const TString& name) = 0;
    virtual void Step(ui64 current, ui64 total) = 0;
    virtual void Step() = 0;

    void Restart(const char* name) {
        Restart(TString(name));
    }

    void Step(const TProgress& progress) {
        Step(progress.Current(), progress.Total());
    }
};


} // namespace NDoom
