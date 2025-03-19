#pragma once

#include "config.h"
#include "owner.h"
#include "time_cached.h"

#include <kernel/common_server/library/storage/abstract.h>

#include <library/cpp/cache/cache.h>

#include <util/generic/hash.h>
#include <util/system/rwlock.h>
#include <util/datetime/base.h>

namespace NZooKeeper {
    class TZooKeeper;
}

namespace NRTProc {

    class TZooStorage : public IVersionedStorage, public TZooOwner {
    private:
        using TValueCache = TLRUCache<TString, TString>;

    private:
        mutable TValueCache ValueCache;

    private:
        TAbstractLock::TPtr NativeLockNode(const TString& path, const bool isWrite, const TDuration timeout = TDuration::Seconds(100000)) const;

    protected:
        TAbstractLock::TPtr NativeWriteLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const override;
        TAbstractLock::TPtr NativeReadLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const override;

    public:
        TZooStorage(const TOptions& options, const TZooStorageOptions& selfOptions);

        bool FastRemoveNode(const TString& key) const;
        void MkDirs(const TString& path) const;
        virtual bool GetVersion(const TString& key, i64& version) const override;
        virtual bool RemoveNode(const TString& key, bool withHistory = false) const override;
        virtual bool ExistsNode(const TString& key) const override;
        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override;
        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override;
        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool withDirs = false) const override;
        NZooKeeper::TZooKeeper* GetZK() const {
            return ZK.Get();
        }
    };

}
