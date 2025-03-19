#pragma once

#include "patcher.h"

#include <library/cpp/getopt/small/last_getopt.h>

class TDaemonOptions {
public:
    TDaemonOptions(bool validateOnly = false)
        : ValidateOnly(validateOnly)
    {}
    void Parse(int argc, char* argv[]);
    void BindToOpts(NLastGetopt::TOpts& opts);

    TString RunPreprocessor() {
        return Preprocessor.ReadAndProcess(ConfigFileName);
    }
    TConfigPatcher& GetPreprocessor() {
        return Preprocessor;
    }
    bool GetValidateOnly() const {
        return ValidateOnly;
    }
    const TString& GetConfigFileName() const {
        return ConfigFileName;
    }
    void SetConfigFileName(const TString& configFileName) {
        ConfigFileName = configFileName;
    }
private:
    TConfigPatcher Preprocessor;
    bool ValidateOnly;
    TString ConfigFileName;
    TString SecdistPath;
    TString SecdistService;
    TString SecdistDbName;
    THashSet<TString> SecdistEnvPaths;
    THashMap<TString, TString> SecdistVarsPaths;
};
