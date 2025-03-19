#pragma once

#include <kernel/common_server/library/storage/abstract.h>
#include <util/system/rwlock.h>
#include <util/generic/map.h>
#include "config.h"

namespace NRTProc {
    class TMemoryStorage : public IVersionedStorage {
    private:
        const TMemoryStorageOptions SelfOptions;
        mutable TMap<TString, TString> Data;
        TRWMutex RWMutex;
    public:
        TMemoryStorage(const TOptions& options, const TMemoryStorageOptions& selfOptions)
            : IVersionedStorage(options)
            , SelfOptions(selfOptions)
        {

        }
        ~TMemoryStorage() {

        }
        virtual bool RemoveNode(const TString& key, bool withHistory = false) const override {
            TWriteGuard wg(RWMutex);
            auto it = Data.find(TFsPath(key).Fix().GetPath());
            if (it != Data.end()) {
                Data.erase(it);
                return true;
            }
            Y_UNUSED(withHistory);
            return false;
        };

        virtual bool ExistsNode(const TString& key) const override {
            TReadGuard wg(RWMutex);
            return Data.contains(TFsPath(key).Fix().GetPath());
        }
        virtual bool GetVersion(const TString& /*key*/, i64& version) const override {
            version = 0;
            return true;
        }
        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool withDirs = false) const override {
            TReadGuard wg(RWMutex);
            auto it = Data.lower_bound(TFsPath(key).Fix().GetPath());
            for (; it != Data.end() && (it->first <= key || it->first.StartsWith(key)); ++it) {
                if (it->first.StartsWith(key) && it->first != key) {
                    result.push_back(it->first);
                }
            }
            Y_UNUSED(withDirs);
            return true;
        }

        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override {
            TReadGuard wg(RWMutex);
            auto it = Data.find(TFsPath(key).Fix().GetPath());
            if (it != Data.end()) {
                result = it->second;
                return true;
            }
            Y_UNUSED(version);
            Y_UNUSED(lock);
            return false;
        }

        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override {
            TWriteGuard wg(RWMutex);
            Data[TFsPath(key).Fix().GetPath()] = value;
            Y_UNUSED(storeHistory);
            Y_UNUSED(lock);
            Y_UNUSED(version);
            return true;
        }

    protected:
        TAbstractLock::TPtr NativeWriteLockNode(const TString& /*path*/, TDuration timeout = TDuration::Seconds(100000)) const override {
            Y_UNUSED(timeout);
            S_FAIL_LOG << "Incorrect using" << Endl;
            return nullptr;
        }

        TAbstractLock::TPtr NativeReadLockNode(const TString& /*path*/, TDuration timeout = TDuration::Seconds(100000)) const override {
            Y_UNUSED(timeout);
            S_FAIL_LOG << "Incorrect using" << Endl;
            return nullptr;

        }
    private:
    };
}
