#pragma once

#include <search/wizard/config/config.h>

class TDefaultPrintwzrdCfg: public TWizardConfig {
public:
    TDefaultPrintwzrdCfg(const TString& workDir, const TString& customRuntimeData, TString rulesList);
};
