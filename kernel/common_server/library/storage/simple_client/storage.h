#pragma once

#include <kernel/common_server/library/storage/abstract.h>
#include <kernel/common_server/library/storage/simple_client/config.h>

namespace NRTProc {
    class IReadValueOnlyStorage : public IVersionedStorage {
    public:
        IReadValueOnlyStorage(const TOptions& options);

        virtual bool RemoveNode(const TString& /*key*/, bool /*withHistory*/ = false) const override;
        virtual bool ExistsNode(const TString& /*key*/) const override;
        virtual bool GetVersion(const TString& /*key*/, i64& version) const override;
        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool /*withDirs*/ = false) const override;
        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override;

    protected:
        virtual TAbstractLock::TPtr NativeWriteLockNode(const TString& /*path*/, TDuration /*timeout*/) const override;
        virtual TAbstractLock::TPtr NativeReadLockNode(const TString& /*path*/, TDuration /*timeout*/) const override;
    };

    class TClientStorage : public IReadValueOnlyStorage {
    public:
        TClientStorage(const TOptions& options, const TClientStorageOptions& selfOptions);

        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override;

    private:
        TAsyncApiImpl Client;
        TDuration RequestTimeout;
    };
}
