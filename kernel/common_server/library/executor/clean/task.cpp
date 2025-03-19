#include "task.h"
#include <kernel/common_server/library/executor/abstract/data.h>
#include <kernel/common_server/library/executor/abstract/task.h>

bool TDataCleanerTask::DoExecute(IDistributedTask::TPtr /*self*/) noexcept {
    Executor->ClearOld();
    return true;
}

const TString TDataCleanerTask::TypeName = "DeprecatedDataCleaner";
TDataCleanerTask::TFactory::TRegistrator<TDataCleanerTask> TDataCleanerTask::Registrator(TDataCleanerTask::TypeName);
