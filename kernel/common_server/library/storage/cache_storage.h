#pragma once

#include "abstract.h"
#include <util/system/mutex.h>
#include <util/folder/path.h>

namespace NRTProc {
    class TCachedStorage : public IVersionedStorage {
    public:
        TCachedStorage(IVersionedStorage::TPtr storage)
            : IVersionedStorage(storage->GetOptions())
            , Storage(storage)
        {}
        virtual bool RemoveNode(const TString& key, bool withHistory = false) const override {
            if (withHistory) {
                RemoveAllCache(key);
            }
            return Storage->RemoveNode(key, withHistory);
        }

        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const override {
            if (version == -1) {
                return Storage->GetValue(key, result, version, lock);
            }
            TString path = TFsPath("/" + key).Fix().GetPath();
            {
                TReadGuard rg(Mutex);
                const auto iter = ValueCache.find(path);
                if (iter != ValueCache.end()) {
                    const auto valueIter = iter->second.find(version);
                    if (valueIter != iter->second.end()) {
                        DEBUG_LOG << "Load value from cache " + key << Endl;
                        result = valueIter->second;
                        return true;
                    }
                }
            }
            if (!Storage->GetValue(path, result, version, lock))
                return false;
            SetCacheValue(key, result, version);
            return true;
        }

        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const override {
            i64 currentVersion = -1;
            if (!Storage->SetValue(key, value, storeHistory, lock, &currentVersion))
                return false;
            if (version) {
                *version = currentVersion;
            }
            if (!storeHistory) {
                RemoveAllCache(key);
            }
            if (currentVersion != -1) {
                SetCacheValue(key, value, currentVersion);
            }
            return true;
        }

        virtual bool ExistsNode(const TString& key) const override {
            return Storage->ExistsNode(key);
        }

        virtual bool GetVersion(const TString& key, i64& version) const override {
            return Storage->GetVersion(key, version);
        }

        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool withDirs = false) const override {
            return Storage->GetNodes(key, result, withDirs);
        }

    protected:
        virtual TAbstractLock::TPtr NativeWriteLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const override {
            return Storage->WriteLockNode(path, timeout);
        }

        virtual TAbstractLock::TPtr NativeReadLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const override {
            return Storage->ReadLockNode(path, timeout);
        }

    private:
        void SetCacheValue(const TString& key, const TString& value, i64 version) const {
            TString path = TFsPath("/" + key).Fix().GetPath();
            TWriteGuard wg(Mutex);
            if (ValueCache[path].size() > GetOptions().GetCacheLevel()) {
                ValueCache[path].erase(ValueCache[path].begin());
            }
            ValueCache[path][version] = value;
        }
        void RemoveAllCache(const TString& key) const {
            TString path = TFsPath("/" + key).Fix().GetPath();
            TWriteGuard wg(Mutex);
            ValueCache.erase(path);
        }
        mutable TRWMutex Mutex;
        mutable THashMap<TString, TMap<ui64, TString> > ValueCache;
        IVersionedStorage::TPtr Storage;
    };
}
