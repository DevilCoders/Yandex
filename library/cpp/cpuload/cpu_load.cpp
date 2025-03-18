#include "cpu_load.h"

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/thread/factory.h>
#include <util/generic/singleton.h>
#include <util/system/event.h>
#include <util/system/thread.h>

#ifdef _unix_
#include <unistd.h>
#include <util/string/vector.h>
#include <util/stream/file.h>
#endif

using namespace NCpuLoad;

namespace {
    class TCpuLoad: public IThreadFactory::IThreadAble {
        Y_DECLARE_SINGLETON_FRIEND()
    public:
        ~TCpuLoad() override;
        static TInfo Get();

    private:
        TCpuLoad();
        void DoExecute() override;
        union TValue {
            TInfo Info;
            TAtomic Atomic;
        };
        TValue Value;
        TSystemEvent Initialized;
        TAtomic Stopped;
        THolder<IThreadFactory::IThread> Thread;
    };

    class TCpuLoadInitializer {
    public:
        TCpuLoadInitializer() {
            TCpuLoad::Get();
        }
    };

    TCpuLoadInitializer Registrator;
}

TInfo TCpuLoad::Get() {
    TValue value;
    value.Atomic = AtomicGet(Singleton<TCpuLoad>()->Value.Atomic);
    return value.Info;
}

TCpuLoad::TCpuLoad()
    : Stopped(0)
    , Thread(SystemThreadFactory()->Run(this))
{
    AtomicSet(Value.Atomic, 0);
    Initialized.Wait();
}

TCpuLoad::~TCpuLoad() {
    AtomicSet(Stopped, 1);
    Thread->Join();
}

void TCpuLoad::DoExecute() {
    TThread::SetCurrentThreadName("CpuLoad");
    while (!AtomicGet(Stopped)) {
        try {
#ifdef _linux_
            TValue value;
            TString path = "/proc/" + ToString(getpid()) + "/stat";
            TVector<TString> stat = SplitString(TUnbufferedFileInput(path).ReadAll(), " ");
            value.Info.User = FromString<ui32>(stat[13]);
            value.Info.System = FromString<ui32>(stat[14]);
            Sleep(TDuration::Seconds(1));
            stat = SplitString(TUnbufferedFileInput(path).ReadAll(), " ");
            value.Info.User = FromString<ui32>(stat[13]) - value.Info.User;
            value.Info.System = FromString<ui32>(stat[14]) - value.Info.System;
            AtomicSet(Value.Atomic, value.Atomic);
#else
            Sleep(TDuration::Seconds(1));
#endif
            Initialized.Signal();
        } catch (...) {
            AtomicSet(Value.Atomic, 0);
        }
    }
}

TInfo NCpuLoad::Get() {
    return TCpuLoad::Get();
}
