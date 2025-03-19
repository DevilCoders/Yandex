#include "abstract.h"

#include <google/protobuf/text_format.h>

namespace NRTProc {
    IVersionedStorage::TPtr IVersionedStorage::Create(const TOptions& options) {
        IVersionedStorage::TPtr result = options.ConstructStorage();
        return result;
    }


    IVersionedStorage::IVersionedStorage(const TOptions& options)
        : Options(options)
    {}

    void IVersionedStorage::GetValue(const TString& key, ::google::protobuf::Message& result, i64 version, bool lock) const {
        TString value;
        if (!GetValue(key, value, version, lock))
            ythrow yexception() << "cannot load " << key << ", version " << version;
        if (!::google::protobuf::TextFormat::ParseFromString(value, &result))
            ythrow yexception() << "corrupted protobuf " << key << ", version " << version;
    }

    bool IVersionedStorage::GetValueSafe(const TString& key, ::google::protobuf::Message& result, i64 version /*= -1*/, bool lock /*= true*/) const {
        TString value;
        if (!GetValue(key, value, version, lock))
            return false;
        if (!::google::protobuf::TextFormat::ParseFromString(value, &result))
            return false;
        return true;
    }

    void IVersionedStorage::SetValue(const TString& key, const ::google::protobuf::Message& value, bool storeHistory, bool lock) const {
        TString valueStr;
        ::google::protobuf::TextFormat::PrintToString(value, &valueStr);
        if (!SetValue(key, valueStr, storeHistory, lock))
            ythrow yexception() << "can not write to storage " << key;
    }

    TAbstractLock::TPtr IVersionedStorage::WriteLockNode(const TString& path, TDuration timeout /*= TDuration::Seconds(100000)*/) const {
        return NativeWriteLockNode(path, timeout);
    }

    TAbstractLock::TPtr IVersionedStorage::ReadLockNode(const TString& path, TDuration timeout /*= TDuration::Seconds(100000)*/) const {
        return NativeReadLockNode(path, timeout);
    }

    void IVersionedStorage::CopyPath(const TFsPath& oldRoot, const TString& newName, const TString& path) const {
        TVector<TString> files;
        if (!GetNodes(oldRoot / path, files, true))
            return;
        TFsPath newRoot = oldRoot.Parent() / newName;
        for (const auto& file : files) {
            TString data;
            TString newPath = (!path ? TString() : (path + "/")) + file;
            if (GetValue(oldRoot / path / file, data)) {
                INFO_LOG << "replace data from : " << path << "/" << file << " data: " << data << Endl;
                if (!SetValue(newRoot / newPath, data))
                    ythrow yexception() << "cannot write data to " << (newRoot / newPath);
            }
            CopyPath(oldRoot, newName, newPath);
        }
    }
}
