#pragma once

#include <kernel/common_server/library/executor/abstract/task.h>

class TDataCleanerTask: public IDistributedTask {
private:
    using TBase = IDistributedTask;

private:
    static TFactory::TRegistrator<TDataCleanerTask> Registrator;

public:
    using TBase::TBase;
    TDataCleanerTask(const TString& id)
        : IDistributedTask(TTaskConstructionContext(TypeName, id))
    {
    }

    static const TString TypeName;

    virtual bool DoExecute(IDistributedTask::TPtr self) noexcept override;
};
