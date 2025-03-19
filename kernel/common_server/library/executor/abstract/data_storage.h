#pragma once
#include "data.h"
#include <kernel/common_server/library/storage/abstract.h>
#include <library/cpp/logger/global/global.h>
#include <util/generic/cast.h>

class IDistributedData;

class IDDataStorage {
public:
    enum class EReadStatus {
        OK,
        NotFound,
        Timeout
    };

protected:
    virtual EReadStatus ReadDataImpl(const TString& identifier, TBlob& result, TInstant timeout = TInstant::Max()) const = 0;
    virtual bool ReadDataImpl(const TVector<TString>& identifiers, TVector<NRTProc::IVersionedStorage::TKValue>& result) const = 0;
    virtual bool StoreDataImpl2(const TString& identifier, const TBlob& data) const = 0;
    virtual bool RemoveDataImpl(const TString& identifier) const = 0;
    virtual bool RemoveDataImpl(const TSet<TString>& identifier) const = 0;

public:
    class TGuard {
    private:
        const IDDataStorage* Storage = nullptr;
        IDistributedData::TPtr Data;
    public:

        using TPtr = TAtomicSharedPtr<TGuard>;

        TGuard(const IDDataStorage* storage, IDistributedData::TPtr data)
            : Storage(storage)
            , Data(data) {

        }

        bool Store2() const {
            if (!!Data.Get()) {
                return Storage->StoreData2(Data.Get());
            }
            return true;
        }

        const IDDataStorage* GetStorage() const {
            return Storage;
        }

        IDistributedData::TPtr GetData() const {
            return Data;
        }

        template <class T>
        T& GetDataAs() const {
            CHECK_WITH_LOG(!!Data);
            return *VerifyDynamicCast<T*>(Data.Get());
        }
    };

    virtual NRTProc::TAbstractLock::TPtr LockData(const TString& identifier, TDuration timeout = TDuration::Zero()) const = 0;

    bool RemoveData(const TString& identifier) const {
        return RemoveDataImpl("data--" + identifier);
    }

    bool RemoveData(const TSet<TString>& identifiers) const {
        TSet<TString> originalNodes;
        for (auto&& i : identifiers) {
            originalNodes.emplace("data--" + i);
        }
        return RemoveDataImpl(originalNodes);
    }

    IDistributedData* RestoreDataUnsafe(const TString& identifier, TInstant timeout = TInstant::Max(), bool* isTimeouted = nullptr) const;
    bool RestoreDataUnsafe(const TVector<TString>& identifiers, TVector<TAtomicSharedPtr<IDistributedData>>& data, TVector<TString>* incorrectIds = nullptr) const;
    TGuard::TPtr RestoreDataLinkUnsafe(const TString& identifier) const;
    bool StoreData2(const IDistributedData* data) const;
};
