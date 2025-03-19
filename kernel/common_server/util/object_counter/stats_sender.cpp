#include "stats_sender.h"

#include <kernel/common_server/library/logging/events.h>

namespace NCSUtil {

    TStatsSender::TStatsSender() {
        Worker = SystemThreadFactory()->Run([this](){
            while (AtomicGet(Running)) {
                SendStats();
                Sleep(kSendPeriod);
            }
        });
    }

    TStatsSender::~TStatsSender() {
        AtomicSet(Running, 0);
        if (Worker) {
            Worker->Join();
        }
    }

    void TStatsSender::SendStats() {
        TGuard<TMutex> g(Mutex);
        for (auto&& [type, count] : Counters) {
            TCSSignals::LTSignal("objects_count", *count)("object_type", type);
        }
    }

}
