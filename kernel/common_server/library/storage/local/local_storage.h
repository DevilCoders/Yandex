#pragma once

#include "config.h"

#include <kernel/common_server/library/storage/abstract.h>

#include <util/folder/path.h>
#include <util/system/rwlock.h>

namespace NRTProc {
    class TFSStorage : public IVersionedStorage {
    public:
        TFSStorage(const TOptions& options, const TLocalStorageOptions& storageOptions);

        virtual bool RemoveNode(const TString& key, bool withHistory = false) const override;
        virtual bool ExistsNode(const TString& key) const override;
        virtual bool GetVersion(const TString& key, i64& version) const override;
        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool withDirs = false) const override;
        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override;
        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override;

    protected:
        virtual TAbstractLock::TPtr NativeReadLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const override;
        virtual TAbstractLock::TPtr NativeWriteLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const override;

    private:
        TFsPath GetPath(const TString& key) const;
        bool IsData(const TString& key) const;
        bool IsData(const TFsPath& path) const;

    private:
        const TLocalStorageOptions StorageOptions;

        TFsPath Root;
        TRWMutex Lock;
    };
}
