#include "lock.h"
#include <library/cpp/logger/global/global.h>

namespace NCS {
    namespace NStorage {
        bool TAbstractLock::IsLockTaken() const {
            S_FAIL_LOG << "Incorrect using" << Endl;
            return false;
        }

    }
}
