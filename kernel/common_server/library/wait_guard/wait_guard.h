#pragma once

#include <kernel/multipart_archive/archive_impl/interfaces.h>

class TWaitGuard {
public:
    TWaitGuard();
    virtual ~TWaitGuard();

    virtual void RegisterObject();
    virtual void UnRegisterObject();
    void WaitAllTasks(ui64 maxTasksCount = 0) const;
    bool Check() const;

private:
    THolder<NRTYArchive::TLinksCounter> Counter;
};
