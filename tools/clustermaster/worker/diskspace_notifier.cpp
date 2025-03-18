#include "diskspace_notifier.h"

#include "master_multiplexor.h"
#include "messages.h"

#include <tools/clustermaster/common/thread_util.h>

static void* ThreadProc(void* arg) {
    const TDiskspaceNotifier::TParams &params(*reinterpret_cast<const TDiskspaceNotifier::TParams*>(arg));
    SetCurrentThreadName("disk space notifier");
    while (1) {
        Singleton<TMasterMultiplexor>()->SendToAll(TDiskspaceMessage(params.MountPoint.data()));
        sleep(params.Delay);
    }
    return nullptr;
}

TDiskspaceNotifier::TDiskspaceNotifier(const TParams &params)
:   Params(params),
    Thread(ThreadProc, reinterpret_cast<void*>(const_cast<TParams*>(&Params)))
{
    Thread.Start();
    Thread.Detach();
}
