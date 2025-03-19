#include "local_storage.h"

#include <library/cpp/threading/named_lock/named_lock.h>

#include <library/cpp/logger/global/global.h>

#include <util/stream/file.h>
#include <util/string/cast.h>
#include <util/string/vector.h>
#include <util/system/fs.h>
#include <util/system/file_lock.h>

namespace {
    class TFsLock {
    public:
        TFsLock(const TString& path)
            : File(MakeHolder<TFileLock>(path))
            , Guard(File.Get())
        {
        }

        bool WasAcquired() const {
            return Guard.WasAcquired();
        }

    private:
        THolder<TFileLock> File;
        TTryGuard<TFileLock> Guard;
    };

    class TCompositeFsLock : public NRTProc::TAbstractLock {
    public:
        TCompositeFsLock(NNamedLock::TNamedLockPtr&& local, TFsLock&& global)
            : Local(std::move(local))
            , Global(std::move(global))
        {
        }

        bool IsLockTaken() const override {
            return IsLocked();
        }

        bool IsLocked() const override {
            return Local && Global.WasAcquired();
        }

    private:
        NNamedLock::TNamedLockPtr Local;
        TFsLock Global;
    };
}

NRTProc::TFSStorage::TFSStorage(const TOptions& options, const TLocalStorageOptions& storageOptions)
    : NRTProc::IVersionedStorage(options)
    , StorageOptions(storageOptions)
    , Root(storageOptions.Root)
{
}

bool NRTProc::TFSStorage::RemoveNode(const TString& key, bool withHistory /*= false*/) const {
    Y_UNUSED(withHistory);
    TFsPath path = GetPath(key);

    TWriteGuard guard(Lock);
    path.ForceDelete();
    DEBUG_LOG << "RemoveNode " << key << Endl;
    return true;
}

bool NRTProc::TFSStorage::ExistsNode(const TString& key) const {
    TFsPath path = GetPath(key);

    TReadGuard guard(Lock);
    DEBUG_LOG << "ExistsNode " << key << Endl;
    return path.Exists();
}

bool NRTProc::TFSStorage::GetVersion(const TString& key, i64& version) const {
    TFsPath path = GetPath(key);

    TReadGuard guard(Lock);
    if (!path.Exists()) {
        DEBUG_LOG << "GetVersion " << key << " no path" << Endl;
        return false;
    }
    TFsPath versionFile = path / "version";
    if (!versionFile.Exists()) {
        DEBUG_LOG << "GetVersion " << key << " no version" << Endl;
        return false;
    }
    TString versionData = TIFStream(versionFile).ReadAll();
    version = FromString<i64>(versionData);
    DEBUG_LOG << "GetVersion " << key << Endl;
    return true;
}

bool NRTProc::TFSStorage::GetNodes(const TString& key, TVector<TString>& result, bool withDirs /*= false*/) const {
    TFsPath path = GetPath(key);

    TReadGuard guard(Lock);
    if (!path.Exists()) {
        return false;
    }

    TVector<TFsPath> children;
    path.List(children);
    for (auto&& child : children) {
        if (child.IsFile()) {
            continue;
        }
        if (!withDirs && !IsData(child)) {
            continue;
        }
        result.push_back(child.Basename());
    }
    DEBUG_LOG << "GetNodes " << key << ": " << JoinStrings(result, "\t") << Endl;
    return true;
}

bool NRTProc::TFSStorage::GetValue(const TString& key, TString& result, i64 version /*= -1*/, bool lock /*= true*/) const {
    TFsPath path = GetPath(key);
    auto l = lock ? ReadLockNode(key) : nullptr;
    if (lock && !l) {
        ERROR_LOG << "cannot acquire lock on " << key << Endl;
        return false;
    }
    if (version < 0) {
        if (!GetVersion(key, version)) {
            return false;
        }
    }
    TFsPath data = path / ToString(version);
    TReadGuard guard(Lock);
    if (!data.Exists()) {
        return false;
    }
    result = TIFStream(data).ReadAll();
    DEBUG_LOG << "GetValue " << key << Endl;
    return true;
}

bool NRTProc::TFSStorage::SetValue(const TString& key, const TString& value, bool storeHistory /*= true*/, bool lock /*= true*/, i64* version /*= nullptr*/) const {
    Y_UNUSED(storeHistory);
    TFsPath path = GetPath(key);
    auto l = lock ? ReadLockNode(key) : nullptr;
    if (lock && !l) {
        ERROR_LOG << "cannot acquire lock on " << key << Endl;
        return false;
    }

    TWriteGuard guard(Lock);
    i64 v = 0;
    ui64 now = Now().MicroSeconds();
    TFsPath versionFile = path / "version";
    TFsPath versionTmpFile = path / ("version." + ToString(now));
    if (versionFile.Exists()) {
        TString versionData = TIFStream(versionFile).ReadAll();
        v = FromString<i64>(versionData) + 1;
    }
    TFsPath dataFile = path / ToString(v);
    TFsPath dataTmpFile = path / (ToString(v) + '.' + ToString(now));

    dataTmpFile.Parent().MkDirs();
    dataTmpFile.DeleteIfExists();
    TOFStream(dataTmpFile).Write(value);
    dataTmpFile.RenameTo(dataFile);

    versionTmpFile.Parent().MkDirs();
    versionTmpFile.DeleteIfExists();
    TOFStream(versionTmpFile).Write(ToString(v));
    versionTmpFile.RenameTo(versionFile);
    if (version) {
        *version = v;
    }
    DEBUG_LOG << "SetValue " << key << Endl;
    return true;
}

NRTProc::TAbstractLock::TPtr NRTProc::TFSStorage::NativeReadLockNode(const TString& key, TDuration timeout /*= TDuration::Seconds(100000)*/) const {
    return NativeWriteLockNode(key, timeout);
}

NRTProc::TAbstractLock::TPtr NRTProc::TFSStorage::NativeWriteLockNode(const TString& key, TDuration timeout /*= TDuration::Seconds(100000)*/) const {
    TInstant deadline = Now() + timeout;
    TFsPath path = GetPath(key) / "lock";
    path.Parent().MkDirs();

    NNamedLock::TNamedLockPtr local = NNamedLock::AcquireLock(path, deadline);
    if (!local) {
        DEBUG_LOG << "NativeWriteLockNode " << key << " failure" << Endl;
        return nullptr;
    }

    do {
        TFsLock global(path);
        if (global.WasAcquired()) {
            return MakeAtomicShared<TCompositeFsLock>(std::move(local), std::move(global));
        } else {
            Sleep(TDuration::MilliSeconds(10));
        }
    } while (Now() < deadline);
    {
        DEBUG_LOG << "NativeWriteLockNode " << key << " failure" << Endl;
        return nullptr;
    }
}

TFsPath NRTProc::TFSStorage::GetPath(const TString& key) const {
    return Root / ("./" + key);
}

bool NRTProc::TFSStorage::IsData(const TString& key) const {
    TFsPath path = GetPath(key);
    return IsData(path);
}

bool NRTProc::TFSStorage::IsData(const TFsPath& path) const {
    TFsPath version = path / "version";
    return version.Exists();
}
