#pragma once

#include <library/cpp/logger/log.h>

#define MangoLog (NMango::TMangoLogSettings::GetLog())
#define MangoDebugLog (NMango::TMangoLogSettings::GetLog() << TLOG_DEBUG)

class IOutputStream;

namespace NMango {
    TString TruncateForLogging(const TString &str);

    class TMangoLogSettings
    {
        TLog Log;
    public:

        TMangoLogSettings();

        static TMangoLogSettings* Instance();

        static TLog& GetLog();

        void SetKosherBackend(IOutputStream &out, bool throwOnError = false, ELogPriority maxPriority = TLOG_INFO);
        void SetTwoWayBackend(IOutputStream &out, IOutputStream &err, bool throwOnError = false, ELogPriority maxPriority = TLOG_INFO);
        void SetFileBackend(const TString &logFile, ELogPriority maxPriority = TLOG_INFO, bool shouldRedirectStandardStreams = false);
    };
}
