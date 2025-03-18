#pragma once

#include "log.h"

struct TClustermasterHttpRequestLogger {
    static void Log(const TString& message) {
        DEBUGLOG1(http, message);
    }
    static void ErrorLog(const TString& message) {
        ERRORLOG1(http, message);
    }
};
