#include "log.h"

#include <mapreduce/yt/interface/logging/logger.h>

#include <util/generic/singleton.h>
#include <util/stream/printf.h>
#include <util/stream/str.h>

using namespace NYT;
using namespace NYTPython;

namespace {
    i32 PyLogLevel(ILogger::ELevel level) {
        switch (level) {
            case ILogger::FATAL:
                return 50;
            case ILogger::ERROR:
                return 40;
            case ILogger::INFO:
                return 20;
            case ILogger::DEBUG:
                return 10;
        }
    }

    class TNYTPythonLogger: public NYT::ILogger {
        TLoggerCheckLevelCallback* CheckLevelCallback = nullptr;
        TLoggerLogCallback* LogCallback = nullptr;

    public:
        void Log(ELevel level, const TSourceLocation& sourceLocation, const char* format, va_list args) override {
            Y_UNUSED(sourceLocation);

            i32 pyLevel = PyLogLevel(level);
            if (!LogCallback || !CheckLevelCallback || !CheckLevelCallback(pyLevel)) {
                return;
            }

            TStringStream text;
            Printf(text, format, args);
            LogCallback(pyLevel, text.Str());
        }

        void SetCallback(TLoggerCheckLevelCallback* checkLevel, TLoggerLogCallback* log) {
            CheckLevelCallback = checkLevel;
            LogCallback = log;
        }

        static TIntrusivePtr<TNYTPythonLogger> Instance() {
            struct TOwner {
                TIntrusivePtr<TNYTPythonLogger> Ptr = MakeIntrusive<TNYTPythonLogger>();
            };
            return Singleton<TOwner>()->Ptr;
        }
    };
}

void NYTPython::InstallPythonLogger() {
    NYT::SetLogger(TNYTPythonLogger::Instance());
}

void NYTPython::SetLoggerCallback(TLoggerCheckLevelCallback* checkLevel, TLoggerLogCallback* log) {
    TNYTPythonLogger::Instance()->SetCallback(checkLevel, log);
}
