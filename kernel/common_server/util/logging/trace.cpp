#include "trace.h"

#include <util/system/tls.h>

using namespace NUtil;

Y_POD_STATIC_THREAD(TTracer*) CurrentTracer(nullptr);

TTracer::TTracer(const TString& function)
    : TraceStart(Now())
    , FunctionName(function)
    , Paused(false)
    , CallerTracer(CurrentTracer)
{
    DEBUG_LOG << FunctionName << " execution started" << Endl;
    if (CallerTracer)
        CallerTracer->Pause();
    CurrentTracer = this;
}

TTracer::~TTracer() {
    VERIFY_WITH_LOG(CurrentTracer == this, "Incorrect order of tracer destruction");
    TraceEnd = Now();
    if (!Pauses.ysize())
        DEBUG_LOG << FunctionName << " execution time: " << TraceEnd - TraceStart << Endl;
    else {
        TDuration pauses = TDuration::MicroSeconds(0);
        for (TVector<TDuration>::const_iterator i = Pauses.begin(); i != Pauses.end(); ++i)
            pauses += *i;
        DEBUG_LOG << FunctionName << " execution time: self " << TraceEnd - TraceStart - pauses << ", callee " << pauses << Endl;
    }
    if (CallerTracer)
        CallerTracer->Unpause();
    CurrentTracer = CallerTracer;
}

void TTracer::Pause() {
    VERIFY_WITH_LOG(!Paused, "Incorrect Pause method usage");
    Paused = true;
    PauseStart = Now();
}

void TTracer::Unpause() {
    VERIFY_WITH_LOG(Paused, "Incorrect Unpause method usage");
    Paused = false;
    PauseEnd = Now();
    Pauses.push_back(PauseEnd - PauseStart);
}
