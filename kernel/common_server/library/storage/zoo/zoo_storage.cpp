#include "zoo_storage.h"

#include <library/cpp/zookeeper/zookeeper.h>
#include <library/cpp/logger/global/global.h>

#include <kernel/common_server/util/logging/trace.h>

#include <util/folder/path.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/string/subst.h>
#include <util/system/guard.h>
#include <util/string/printf.h>
#include "lock.h"

namespace NRTProc {

    TAbstractLock::TPtr TZooStorage::NativeLockNode(const TString& path, const bool isWrite, const TDuration timeout) const {
        try {
            TFsPath fs(path);
            auto result = MakeHolder<TZooLock>(this, fs.Fix().GetPath(), isWrite);
            result->WaitLock(timeout);
            if (result->IsLockTaken()) {
                return result.Release();
            }
        } catch (...) {
            BuildZK("NativeLockNode " + CurrentExceptionMessage());
            return NativeLockNode(path, isWrite, timeout);
        }
        return nullptr;
    }

    TAbstractLock::TPtr TZooStorage::NativeReadLockNode(const TString& path, TDuration timeout) const {
        return NativeLockNode(path, false, timeout);
    }

    TAbstractLock::TPtr TZooStorage::NativeWriteLockNode(const TString& path, TDuration timeout) const {
        return NativeLockNode(path, true, timeout);
    }

    bool TZooStorage::GetVersion(const TString& key, i64& version) const {
        try {
            TReadGuard wg(RWMutex);
            TString path = TFsPath("/" + key).Fix().GetPath();
            if (!!ZK->Exists(path)) {
                TString ver = ZK->GetData(path);
                if (!ver) {
                    return false;
                } else {
                    try {
                        version = FromString(ver);
                    } catch (...) {
                        return false;
                    }
                }
                return true;
            }
        } catch (...) {
            BuildZK("GetVersion failed: " + CurrentExceptionMessage());
            return GetVersion(key, version);
        }
        return false;

    }

    TZooStorage::TZooStorage(const TOptions& options, const TZooStorageOptions& selfOptions)
        : IVersionedStorage(options)
        , TZooOwner(selfOptions)
        , ValueCache(selfOptions.CacheSize)
    {
    }

    bool TZooStorage::FastRemoveNode(const TString& key) const {
        try {
            TString path = TFsPath("/" + key).Fix().GetPath();
            TReadGuard wg(RWMutex);
            NZooKeeper::TStatPtr ex = ZK->Exists(path);
            if (!!ex)
                ZK->Delete(path, ex->version);
            return !!ex && !ZK->Exists(path);
        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("RemoveNode failed: " + CurrentExceptionMessage());
            return FastRemoveNode(key);
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("RemoveNode failed: " + CurrentExceptionMessage());
            return FastRemoveNode(key);
        } catch (NZooKeeper::TNodeNotExistsError&) {
            WARNING_LOG << "Race for node " << key << " removing" << Endl;
            return true;
        }
        return false;
    }

    bool TZooStorage::RemoveNode(const TString& key, bool withHistory) const {
        try {
            TString path = TFsPath("/" + key).Fix().GetPath();
            NZooKeeper::TStatPtr ex;
            {
                TReadGuard wg(RWMutex);
                ex = ZK->Exists(path);
            }
            if (!!ex) {
                bool removeChildren = true;
                TVector<TString> nodes;
                if (GetNodes(key, nodes, true)) {
                    for (ui32 i = 0; i < nodes.size(); ++i) {
                        removeChildren &= RemoveNode(key + "/" + nodes[i], withHistory);
                    }
                }
                bool fastRemove = true;
                if (withHistory) {
                    TVector<TString> nodes;
                    if (GetNodes("history", nodes, false)) {
                        for (const auto& node : nodes) {
                            i64 ver;
                            if (node.StartsWith(key)
                                && (node.size() == (key.size() + 10) || (node.size() >(key.size() + 12) && node.find(".$", key.size() + 10) == key.size() + 10))
                                && TryFromString<i64>(node.substr(("/history/" + key).size(), 10), ver)) {

                                fastRemove &= FastRemoveNode("/history/" + node);
                                auto cache = ValueCache.Find("/history/" + node);
                                if (cache != ValueCache.End()) {
                                    ValueCache.Erase(cache);
                                }
                            }
                        }
                    } else {
                        return false;
                    }
                }

                return removeChildren && fastRemove && FastRemoveNode(path);
            } else {
                return false;
            }

        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("RemoveNode failed: " + CurrentExceptionMessage());
            return RemoveNode(key, withHistory);
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("RemoveNode failed: " + CurrentExceptionMessage());
            return RemoveNode(key, withHistory);
        } catch (NZooKeeper::TNodeNotExistsError&) {
            WARNING_LOG << "Race for node " << key << " removing" << Endl;
            return true;
        }
        return false;
    }

    bool TZooStorage::ExistsNode(const TString& key) const {
        try {
            TReadGuard wg(RWMutex);
            TString path = TFsPath("/" + key).Fix().GetPath();
            return !!ZK->Exists(path);
        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("ExistsNode failed: " + CurrentExceptionMessage());
            return ExistsNode(key);
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("ExistsNode failed: " + CurrentExceptionMessage());
            return ExistsNode(key);
        }
    }

    bool TZooStorage::GetValue(const TString& key, TString& result, i64 version, bool lock) const {
        NUtil::TTracer timer("get_value " + key);
        try {
            TString path;
            if (version == -1) {
                TAbstractLock::TPtr lockPtr = nullptr;
                if (lock) {
                    NUtil::TTracer lockTimer("lock " + key);
                    lockPtr = ReadLockNode(key);
                }
                if (GetVersion(key, version))
                    return GetValue(key, result, version);
                else {
                    WARNING_LOG << "Incorrect version " << version << " for " << key << Endl;
                    return false;
                }
            } else {
                const TString ver = Sprintf("%.10ld", version);
                path = TFsPath("/history/" + key + ver).Fix().GetPath();
            }
            TReadGuard rg(RWMutex);
            auto cache = ValueCache.Find(path);
            if (cache != ValueCache.End()) {
                result = *cache;
                DEBUG_LOG << "Loaded from cache " << key << Endl;
                return true;
            }
            if (!!ZK->Exists(path)) {
                result = "";
                for (ui32 part = 0; ; ++part) {
                    TString pathPart = path + (part ? ".$" + ToString(part) : "");
                    NZooKeeper::TStatPtr stat = ZK->Exists(pathPart);
                    if (!stat) {
                        rg.Release();
                        TWriteGuard wg(RWMutex);
                        ValueCache.Update(path, result);
                        return true;
                    }
                    result += ZK->GetData(pathPart);
                }
            }
        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("GetValue failed: " + CurrentExceptionMessage());
            return GetValue(key, result, version);
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("GetValue failed: " + CurrentExceptionMessage());
            return GetValue(key, result, version);
        } catch (NZooKeeper::TNodeNotExistsError&) {
            WARNING_LOG << "Race for node " << key << " get" << Endl;
            return false;
        }
        return false;
    }

    void TZooStorage::MkDirs(const TString& path) const {
        Y_VERIFY(path.StartsWith('/'), "path should be absolute");
        try {
            TFsPath fs(path);
            TReadGuard wg(RWMutex);
            ZK->CreateHierarchy(fs, NZooKeeper::ACLS_ALL);
        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("MkDirs failed: " + CurrentExceptionMessage());
            return MkDirs(path);
        } catch (NZooKeeper::TNodeExistsError&) {
            return;
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("MkDirs failed: " + CurrentExceptionMessage());
            return MkDirs(path);
        }
    }

    bool TZooStorage::SetValue(const TString& key, const TString& value, bool storeHistory, bool lock, i64* versionOut) const {
        TDebugLogTimer timer("set_value " + key);
        try
        {
            TString currentValue;
            TString pathHistory = TFsPath("/history/" + key).Fix().GetPath();
            MkDirs(TFsPath("/history/" + key).Fix().Parent().GetPath());
            TAbstractLock::TPtr lockPtr = nullptr;
            if (lock) {
                TDebugLogTimer lockTimer("lock " + key);
                lockPtr = WriteLockNode(key);
            }
            TString startPath;
            for (ui32 part = 0;; ++part) {
                if (720 * 1024 * part >= value.size() && part)
                    break;
                TReadGuard wg(RWMutex);
                if (!part)
                    startPath = ZK->Create(pathHistory, value.substr(part * 720 * 1024, 720 * 1024), NZooKeeper::ACLS_ALL, NZooKeeper::CM_PERSISTENT_SEQUENTIAL);
                else
                    ZK->Create(startPath + ".$" + ToString(part), value.substr(part * 720 * 1024, 720 * 1024), NZooKeeper::ACLS_ALL, NZooKeeper::CM_PERSISTENT);
            }
            TString ver;
            ver = startPath.substr(startPath.size() - 10, TString::npos);
            i32 version = FromString<i32>(ver);
            if (versionOut)
                *versionOut = version;
            TString path = TFsPath("/" + key).Fix().GetPath();
            {
                i64 oldVersion = -1;
                if (!storeHistory)
                    GetVersion(path, oldVersion);
                if (version < oldVersion)
                    return false;
                TReadGuard wg(RWMutex);
                if (!!ZK->Exists(path)) {
                    ZK->SetData(path, ver);
                    if (!storeHistory && oldVersion > 0) {
                        const TString verOld = Sprintf("%.10ld", oldVersion);
                        TString pathDelHistory = TFsPath("/history/" + key + verOld).Fix().GetPath();
                        auto cache = ValueCache.Find(pathDelHistory);
                        if (cache != ValueCache.End()) {
                            ValueCache.Erase(cache);
                        }
                        for (ui32 part = 0;; ++part) {
                            TString pathPart = pathDelHistory + (part ? ".$" + ToString(part) : "");
                            NZooKeeper::TStatPtr stat = ZK->Exists(pathPart);
                            if (!stat)
                                break;
                            ZK->Delete(pathPart, stat->version);
                        }
                    }
                    return true;
                }
            }
            MkDirs(TFsPath("/" + key).Fix().Parent().GetPath());
            {
                TReadGuard wg(RWMutex);
                ZK->Create(path, ver, NZooKeeper::ACLS_ALL, NZooKeeper::CM_PERSISTENT);
            }
            return true;
        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("SetValue failed: " + CurrentExceptionMessage());
            return SetValue(key, value);
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("SetValue failed: " + CurrentExceptionMessage());
            return SetValue(key, value);
        }
        return false;
    }

    bool TZooStorage::GetNodes(const TString& key, TVector<TString>& result, bool withDirs) const {
        auto path = TFsPath("/" + key).Fix();
        try {
            TReadGuard wg(RWMutex);
            if (!ZK->Exists(path))
                return false;
            result = ZK->GetChildren(path);
            if (!withDirs) {
                TVector<TString> files;
                for (auto&& node : result) {
                    auto stat = ZK->Exists(path / node);
                    if (stat) {
                        if (stat->numChildren == 0) {
                            files.push_back(node);
                        }
                    }
                }
                result = std::move(files);
            }
            for (int i = result.size() - 1; i >= 0; i--) {
                if (result[i].find(".$") != TString::npos) {
                    result.erase(result.begin() + i);
                }
            }
            return true;
        } catch (NZooKeeper::TInvalidStateError&) {
            BuildZK("GetNodes failed: " + CurrentExceptionMessage());
            return GetNodes(key, result, withDirs);
        } catch (NZooKeeper::TConnectionLostError&) {
            BuildZK("GetNodes failed: " + CurrentExceptionMessage());
            return GetNodes(key, result, withDirs);
        } catch (NZooKeeper::TNodeNotExistsError&) {
            WARNING_LOG << "Race for node " << key << " get child" << Endl;
            return true;
        }
        return false;
    }

}
