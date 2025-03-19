#include "lock.h"

#include <library/cpp/zookeeper/zookeeper.h>
#include <library/cpp/logger/global/global.h>

#include <kernel/common_server/util/logging/trace.h>

#include <util/folder/path.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/string/subst.h>
#include <util/system/guard.h>
#include <util/string/printf.h>

namespace NRTProc {

    TZooLock::~TZooLock() {
        ReleaseLock();
    }

    void TZooLock::WaitLock(const TDuration timeout) {
        const TInstant deadline = Now() + timeout;
        while (!BeforeNodes.empty() && deadline > Now()) {
            const TString lockPath = "/locks/abstract_locks/" + Id + "/" + BeforeNodes.back();
            const auto actor = [this, &lockPath](NZooKeeper::TZooKeeper& zk) ->bool {
                if (!zk.Exists(lockPath)) {
                    BeforeNodes.pop_back();
                    return true;
                }
                return false;
            };
            if (!ZK->DoActionSafe<bool>(actor)) {
                Sleep(TDuration::MilliSeconds(10));
            }
        }
    }

    bool TZooLock::BuildLock() {
        const auto actor = [this](NZooKeeper::TZooKeeper& zk) {
            VERIFY_WITH_LOG(!LockName, "Incorrect usage");
            TFsPath pathLocks = TFsPath("/locks/abstract_locks/" + Id).Fix();
            zk.CreateHierarchy(pathLocks, NZooKeeper::ACLS_ALL);
            const TFsPath lockPath = pathLocks / (WriteLock ? "w" : "r");
            LockName = zk.Create(lockPath.GetPath(), "", NZooKeeper::ACLS_ALL, NZooKeeper::CM_EPHEMERAL_SEQUENTIAL);
            CurrentIdx = FromString<ui32>(LockName.substr(LockName.size() - 10, 10));
            {
                TVector<TString> locks = zk.GetChildren(pathLocks.GetPath(), false);
                for (ui32 i = 0; i < locks.size(); ++i) {
                    const ui32 idx = FromString<ui32>(locks[i].substr(locks[i].size() - 10, 10));
                    if (idx < CurrentIdx) {
                        bool isWriteLock = locks[i].StartsWith("w");
                        if (isWriteLock || WriteLock) {
                            BeforeNodes.push_back(locks[i]);
                        }
                    }
                }
            }
            return true;
        };

        return ZK->DoActionSafe<bool>(actor);
    }

    bool TZooLock::IsLockTaken() const {
        return BeforeNodes.empty();
    }

    bool TZooLock::ReleaseLock() {
        if (!!LockName) {
            const auto actor = [this](NZooKeeper::TZooKeeper& zk) ->bool {
                try {
                    zk.Delete(LockName, -1);
                    LockName = "";
                } catch (NZooKeeper::TNodeNotExistsError&) {
                }
                return true;
            };
            return ZK->DoActionSafe<bool>(actor);
        }
        return false;
    }

    bool TZooLock::IsLockedImpl() const {
        VERIFY_WITH_LOG(!!LockName, "Lock should be acquired via TryLock first");
        const auto actor = [this](NZooKeeper::TZooKeeper& zk) ->bool {
            return !!zk.Exists(LockName);
        };
        return ZK->DoActionSafe<bool>(actor);
    }

    bool TZooLock::IsLocked() const {
        return IsLocked_.Get();
    }

    TZooLock::TZooLock(const TZooOwner* zk, const TString& id, bool writeLock)
        : ZK(zk)
        , Id(id)
        , WriteLock(writeLock)
        , IsLocked_(*this)
    {
        SubstGlobal(Id, '/', '_');
        BuildLock();
    }

}
