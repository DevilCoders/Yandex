#pragma once

#include "config.h"
#include <util/generic/ptr.h>
#include <util/system/rwlock.h>
#include <library/cpp/zookeeper/zookeeper.h>
#include <contrib/libs/zookeeper/include/zookeeper.h>

namespace NRTProc {

    class TZooOwner {
    private:
        const TZooStorageOptions SelfOptions;
        mutable bool AtFirstBuild = true;
    protected:
        TRWMutex RWMutex;
        mutable THolder<NZooKeeper::TZooKeeper> ZK;
        bool BuildZKImpl(const TString& reason, const TString& root) const;
        bool BuildZK(const TString& reason) const;
    public:

        using TPtr = TAtomicSharedPtr<TZooOwner>;

        TZooOwner(const TZooStorageOptions& selfOptions);

        template <class TResult, class TAction>
        TResult DoActionSafe(const TAction& action) const {
            try {
                TReadGuard rg(RWMutex);
                return action(*ZK);
            } catch (const NZooKeeper::TError& zooError) {
                if (zooError.GetZooErrorCode() == ZOO_ERRORS::ZOPERATIONTIMEOUT ||
                    zooError.GetZooErrorCode() == ZOO_ERRORS::ZDATAINCONSISTENCY ||
                    zooError.GetZooErrorCode() == ZOO_ERRORS::ZINVALIDSTATE ||
                    zooError.GetZooErrorCode() == ZOO_ERRORS::ZCONNECTIONLOSS
                    ) {
                    BuildZK("RemoveNode failed: " + CurrentExceptionMessage());
                } else {
                    throw;
                }
                return DoActionSafe<TResult>(action);
            }
        };
    };
}
