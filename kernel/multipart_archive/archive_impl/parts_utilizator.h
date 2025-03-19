#pragma once

#include <util/thread/pool.h>

namespace NRTYArchive {
    class TPartsUtilizator : public TThreadPool {
    public:
        using TThreadPool::TThreadPool;

        virtual void* CreateThreadSpecificResource() override;
    };
}
