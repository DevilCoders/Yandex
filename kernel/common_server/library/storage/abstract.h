#pragma once

#include "config.h"

#include <library/cpp/object_factory/object_factory.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/datetime/base.h>
#include <library/cpp/string_utils/base64/base64.h>
#include "abstract/lock.h"

namespace google {
    namespace protobuf {
        class Message;
    }
}

namespace NRTProc {

    using TAbstractLock = NCS::NStorage::TAbstractLock;
    using TFakeLock = NCS::NStorage::TFakeLock;

    class IVersionedStorage {
    public:
        using TOptions = TStorageOptions;
        using TPtr = TAtomicSharedPtr<IVersionedStorage>;
    public:
        static IVersionedStorage::TPtr Create(const TOptions& options);

    public:
        IVersionedStorage(const TOptions& options);
        virtual ~IVersionedStorage() {}

        virtual bool RemoveNodes(const TSet<TString>& keys, bool withHistory = false, ui32* removedCount = nullptr) const {
            ui32 result = 0;
            for (auto&& i : keys) {
                if (RemoveNode(i, withHistory)) {
                    ++result;
                }
            }
            if (removedCount) {
                *removedCount = result;
            }
            return true;
        }
        virtual bool RemoveNode(const TString& key, bool withHistory = false) const = 0;
        virtual bool ExistsNode(const TString& key) const = 0;
        virtual bool GetVersion(const TString& key, i64& version) const = 0;
        virtual bool GetNodes(const TString& key, TVector<TString>& result, bool withDirs = false) const = 0;
        virtual bool GetValue(const TString& key, TString& result, i64 version = -1, bool lock = true) const = 0;
        virtual bool SetValue(const TString& key, const TString& value, bool storeHistory = true, bool lock = true, i64* version = nullptr) const = 0;

        class TKValue {
            RTLINE_READONLY_ACCEPTOR_DEF(Key, TString);
            RTLINE_READONLY_ACCEPTOR_DEF(Value, TString);
        public:
            TKValue(const TString& key, const TString& value)
                : Key(key)
                , Value(value)
            {

            }
        };

        virtual bool GetValues(const TSet<TString>& keys, TVector<TKValue>& result, TVector<TString>* incorrectIds = nullptr) const {
            result.reserve(keys.size());
            for (auto&& i : keys) {
                TString kv;
                if (GetValue(i, kv)) {
                    result.emplace_back(i, kv);
                } else if (incorrectIds) {
                    incorrectIds->emplace_back(i);
                }
            }
            return true;
        }

        void CopyPath(const TFsPath& oldRoot, const TString& newName, const TString& path) const;
        void GetValue(const TString& key, ::google::protobuf::Message& result, i64 version = -1, bool lock = true) const;
        bool GetValueSafe(const TString& key, ::google::protobuf::Message& result, i64 version = -1, bool lock = true) const;

        template <class TProtoMessage>
        bool SetValueProto(const TString& key, TProtoMessage& result, bool storeHistory = true, bool lock = true) const {
            TString value;
            return SetValue(key, Base64Encode(result.SerializeAsString()), storeHistory, lock);
        }

        template <class TProtoMessage>
        bool GetValueProto(const TString& key, TProtoMessage& result, i64 version = -1, bool lock = true) const {
            TString value;
            return GetValue(key, value, version, lock) && result.ParseFromString(Base64Decode(value));
        }

        void SetValue(const TString& key, const ::google::protobuf::Message& value, bool storeHistory = true, bool lock = true) const;
        virtual TAbstractLock::TPtr WriteLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const;
        virtual TAbstractLock::TPtr ReadLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const;

        const TOptions& GetOptions() const {
            return Options;
        }

    protected:
        virtual TAbstractLock::TPtr NativeWriteLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const = 0;
        virtual TAbstractLock::TPtr NativeReadLockNode(const TString& path, TDuration timeout = TDuration::Seconds(100000)) const = 0;

    protected:
        const TOptions Options;
    };

};
