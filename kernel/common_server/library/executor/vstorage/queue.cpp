#include "queue.h"
#include <util/system/mutex.h>

TVSQueueConfig::TFactory::TRegistrator<TVSQueueConfig> TVSQueueConfig::Registrator("VStorage");

IDTasksQueue::TPtr TVSQueueConfig::Construct(IDistributedTaskContext* context) const {
    return MakeAtomicShared<TVSTasksQueue>(*this, SignalQueueSize, context);
}
