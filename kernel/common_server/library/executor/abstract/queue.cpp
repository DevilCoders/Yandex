#include "queue.h"
#include "storage.h"

IDTasksQueue::IDTasksQueue(IDistributedQueue::TPtr queue, const IDistributedTasksQueueConfig& config, IDistributedTaskContext* context)
    : QueueView(MakeHolder<TDistributedQueueView>(queue, config))
    , OriginalQueue(queue)
{
    IQueueViewSelector::TPtr selector = IQueueViewSelector::TFactory::Construct(config.GetTasksSelectorName(), context);
    CHECK_WITH_LOG(!!selector);
    QueueView->SetSelector(selector);
}

void IDTasksQueue::ClearOld(IDTasksStorage& tasksStorage, const TAtomic& active, const TDuration borderWaiting) const {
    try {
        const TVector<TString> nodes = GetAllNodes();
        for (auto&& i : nodes) {
            if (AtomicGet(active) == 0) {
                break;
            }
            TAtomicSharedPtr<TTaskData> task;
            try {
                task = OriginalQueue->LoadTask(i);
                if (!!task && task->GetDeadline() + borderWaiting < Now()) {
                    INFO_LOG << "Removing old task " << i << " by deadline " << task->GetDeadline() << Endl;
                    OriginalQueue->RemoveTask(i);
                    tasksStorage.RemoveTask(i);
                } else if (!!task) {
                    INFO_LOG << "Saved task " << i << " with deadline " << task->GetDeadline() << Endl;
                } else {
                    INFO_LOG << "Corrupted task " << i << Endl;
                }
            } catch (...) {
                ERROR_LOG << "Cannot restore task: " << CurrentExceptionMessage() << Endl;
                continue;
            }
        }
    } catch (...) {
        ERROR_LOG << "Cannot take info about full tasks list: " << CurrentExceptionMessage() << Endl;
    }
}
