#pragma once

#include "time_cached.h"

#include <kernel/common_server/library/storage/abstract.h>

#include <util/generic/hash.h>
#include <util/system/rwlock.h>
#include <util/datetime/base.h>
#include "config.h"
#include <library/cpp/zookeeper/zookeeper.h>
#include "owner.h"

namespace NRTProc {

    class TZooLock: public TAbstractLock {
    private:
        class TCachedIsLocked: public ITimeCachedValue<bool> {
        public:
            TCachedIsLocked(const TZooLock& lock)
                : ITimeCachedValue(TDuration::Seconds(5))
                , Lock(lock)
            {
            }

            bool DoGet() override {
                return Lock.IsLockedImpl();
            }

        private:
            const TZooLock& Lock;
        };

        bool BuildLock();
        bool ReleaseLock();

    private:
        const TZooOwner* ZK;
        ui32 CurrentIdx;
        TString Id;
        bool WriteLock;
        TString LockName;
        mutable TCachedIsLocked IsLocked_;
        TVector<TString> BeforeNodes;
    public:
        TZooLock(const TZooOwner* zk, const TString& id, bool writeLock);
        virtual ~TZooLock() override;

        virtual bool IsLockTaken() const override;
        virtual bool IsLocked() const override;
        void WaitLock(const TDuration timeout);
    private:
        bool IsLockedImpl() const;
    };

}
