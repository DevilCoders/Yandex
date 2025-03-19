#pragma once

#include "parts_utilizator.h"

#include <util/generic/singleton.h>
#include <util/thread/pool.h>

namespace NRTYArchive {

    class TArchiveGlobals {
        Y_DECLARE_SINGLETON_FRIEND();
    private:
        TArchiveGlobals() {
            PartsUtilizator.Start(1);
        }

    public:
        static TArchiveGlobals* Instance() {
            return Singleton<TArchiveGlobals>();
        }

        static void AddTask(IObjectInQueue* task) {
            CHECK_WITH_LOG(Instance()->PartsUtilizator.AddAndOwn(THolder<IObjectInQueue>(task)));
        }

    private:
        TPartsUtilizator PartsUtilizator{"MultPartArcUtil"};
    };
}
