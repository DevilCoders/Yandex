#include "trace.h"

#include <iomanip>

#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/sighandler/async_signals_handler.h>

#include <util/datetime/base.h>
#include <util/string/strip.h>
#include <util/string/cast.h>
#include <util/system/sigset.h>
#include <util/system/thread.h>
#include <util/string/subst.h>
#include <util/string/printf.h>

namespace {

static inline TString FixMessage(TString msg) {
    if (SubstGlobal(msg, "\n", ", ")) {
        SubstGlobal(msg, ", }", "}");
    }

    return msg;
}

class TTracer {
public:
    explicit inline TTracer(const TString& path, bool debugOutput)
        : EventLog(path, NEvClass::Factory()->CurrentFormat())
        , Frame(EventLog)
        , Count(0)
        , DebugOutput(debugOutput)
    {
    }

    inline void Trace(const NAapi::TProxyEvent& ev) {
        with_lock(Mutex) {
            if (DebugOutput) {
                Cerr << "[" << ToString(Now().MicroSeconds()) << "] "
                     << "[" << Sprintf("%*llu", 20, static_cast<unsigned long long>(TThread::CurrentThreadId())) << "] "
                     << "[" << Sprintf("%*s", 25, ev.Msg->GetTypeName().data()) << "] "
                     << FixMessage("{" + ev.Msg->DebugString() + "}") << '\n';
            }

            Frame.LogEvent(ev);

            if ((++Count % 1024) == 0) {
                Frame.Flush();
            }
        }
    }

    inline void ReopenLog() {
        EventLog.ReopenLog();  // do not need lock, because TFileLogBackend takes write lock for it
    };

private:
    TEventLog EventLog;
    TSelfFlushLogFrame Frame;
    ui64 Count;
    TMutex Mutex;
    bool DebugOutput;
};

class TTraceHolder {
public:
    inline void Init(const TString& path, bool debugOutput) {
        Tracer.Reset(new TTracer(path, debugOutput));
    }

    inline void Trace(const NAapi::TProxyEvent& ev) {
        Tracer->Trace(ev);
    }

    inline void ReopenLog() {
        Tracer->ReopenLog();
    }

    static inline TTraceHolder* Instance() {
        return SingletonWithPriority<TTraceHolder, 128000>();
    }

private:
    THolder<TTracer> Tracer;
};

}  // namespace


void NAapi::InitTrace(const TString& path, bool debugOutput) {
    TTraceHolder::Instance()->Init(path, debugOutput);
    SetAsyncSignalHandler(SIGHUP, [](int) {TTraceHolder::Instance()->ReopenLog();});
}

void NAapi::Trace(const TProxyEvent& ev) {
    TTraceHolder::Instance()->Trace(ev);
}
