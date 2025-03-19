#pragma once

#include <library/cpp/logger/global/global.h>

#include <util/generic/hash.h>
#include <util/system/thread.h>
#include <util/generic/noncopyable.h>
#include <util/datetime/base.h>

namespace NUtil {

class TTracer: public NNonCopyable::TNonCopyable {
public:
    TTracer(const TString& function);
    ~TTracer();

private:
    void Pause();
    void Unpause();

private:
    TInstant    TraceStart;
    TInstant    TraceEnd;
    TInstant    PauseStart;
    TInstant    PauseEnd;
    TString      FunctionName;
    bool        Paused;
    TVector<TDuration> Pauses;
    TTracer*    CallerTracer;
};
}

using TDebugLogTimer = NUtil::TTracer;

#define TRACE_FUNCTION NUtil::TTracer __function_tracer(__FUNCTION__)

#define TRACE_LOG DEBUG_LOG << __FUNCTION__ << " "
