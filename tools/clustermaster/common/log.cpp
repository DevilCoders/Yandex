#include "log.h"

#include <util/string/cast.h>
#include <util/system/env.h>

TStdErrOutput stderrOutput;

ELogLevel GetLogLevelFromEnvOrDefault(ELogLevel defaultLevel) {
    TString envVar = GetEnv("CLUSTERMASTER_LOG_LEVEL", "");
    if (envVar.empty()) {
        return defaultLevel;
    }
    envVar.to_lower();
    try {
        return FromString<ELogLevel>(envVar);
    } catch (...) {
        Cerr << "Bad env variable: " << envVar << Endl;
        return defaultLevel;
    }
}
