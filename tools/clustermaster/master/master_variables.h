#pragma once

#include "worker_variables.h"

#include <util/generic/string.h>

struct IWorkerPoolVariables {
    virtual ~IWorkerPoolVariables() {}
    virtual const TWorkerVariablesLight& GetVariablesForWorker(const TString& workername) const = 0;
    virtual bool CheckIfWorkerIsAvailable(const TString& workername) const = 0;
};

struct TWorkerPoolVariablesEmpty: public IWorkerPoolVariables {
private:
    TWorkerVariablesLight Variables;

public:

    const TWorkerVariablesLight& GetVariablesForWorker(const TString&) const override {
        return Variables;
    }

    bool CheckIfWorkerIsAvailable(const TString&) const override {
        return true;
    }
};
