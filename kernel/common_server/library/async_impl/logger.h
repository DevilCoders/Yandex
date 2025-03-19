#pragma once

#include <library/cpp/http/misc/httpcodes.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/library/logging/events.h>

template <typename TOperationType>
class TRequestLogger {
public:
    using TOperationTypeImpl = TOperationType;

    TRequestLogger(const TString& source)
        : TRequestLogger(source, source)
    {
    }

    TRequestLogger(const TString& errorSource, const TString& signalSource)
        : ErrorSource(errorSource)
        , SignalSource(signalSource)
    {
    }

    virtual ~TRequestLogger() = default;

    void ProcessStart(const TOperationType& type) const {
        TCSSignals::SignalAdd(SignalSource + "-requests", ::ToString(type), 1);
    }

    void ProcessReply(ui32 code) const {
        TCSSignals::SignalAdd(SignalSource + "-reply-codes", ::ToString(code), 1);
    }

    void ProcessReply(const TOperationType& type, ui32 code) const {
        ProcessReply(code);
        TCSSignals::SignalAdd(SignalSource + "-reply-codes", ::ToString(type) + "-" + ::ToString(code), 1);
    }

    void ProcessError(const TOperationType& type, const TString& message) const {
        ProcessError(type);
        ProcessError(message);
    }

    void ProcessError(const TOperationType& type) const {
        TCSSignals::SignalAdd(SignalSource + "-errors", ::ToString(type), 1);
    }

    void ProcessError(const TString& message) const {
        TFLEventLog::Error(message)("source", ErrorSource);
    }

private:
    const TString ErrorSource;
    const TString SignalSource;
};
