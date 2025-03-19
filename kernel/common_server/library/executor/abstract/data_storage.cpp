#include "data_storage.h"
#include <kernel/common_server/util/blob_with_header.h>

bool IDDataStorage::RestoreDataUnsafe(const TVector<TString>& identifiers, TVector<TAtomicSharedPtr<IDistributedData>>& data, TVector<TString>* incorrectIds) const {
    TVector<TString> idsLocal;
    for (auto&& i : identifiers) {
        idsLocal.emplace_back("data--" + i);
    }
    TVector<NRTProc::IVersionedStorage::TKValue> strData;
    if (!ReadDataImpl(idsLocal, strData)) {
        return false;
    }

    for (auto&& d : strData) {
        const TBlob bLocal = TBlob::FromString(d.GetValue());
        TBlobWithHeader<NFrontendProto::TDataMeta> bwh;
        if (!bwh.Load(bLocal)) {
            ERROR_LOG << "cannot parse blob with header" << Endl;
            if (incorrectIds) {
                incorrectIds->emplace_back(d.GetKey().substr(6));
            }
            continue;
        }

        const NFrontendProto::TDataMeta& dataMeta = bwh.GetHeader();

        TDataConstructionContext context(dataMeta.GetType(), d.GetKey().substr(6));
        TAtomicSharedPtr<IDistributedData> dLocal = IDistributedData::TFactory::Construct(dataMeta.GetType(), context);
        if (!dLocal) {
            ERROR_LOG << "Incorrect data type: '" << dataMeta.GetType() << "'" << Endl;
            if (incorrectIds) {
                incorrectIds->emplace_back(d.GetKey().substr(6));
            }
            continue;
        }
        if (!dLocal->ParseMetaFromProto(dataMeta)) {
            ERROR_LOG << "Incorrect data meta: '" << dataMeta.DebugString() << "'" << Endl;
            if (incorrectIds) {
                incorrectIds->emplace_back(d.GetKey().substr(6));
            }
            continue;
        }
        if (!dLocal->Deserialize(bwh.GetData())) {
            WARNING_LOG << "Incorrect data in storage" << Endl;
            if (incorrectIds) {
                incorrectIds->emplace_back(d.GetKey().substr(6));
            }
            continue;
        }
        data.emplace_back(dLocal);
    }

    return true;
}

IDistributedData* IDDataStorage::RestoreDataUnsafe(const TString& identifier, TInstant deadline, bool* isTimeouted) const {
    THolder<IDistributedData> result;
    {
        TBlob data;
        IDDataStorage::EReadStatus status = ReadDataImpl("data--" + identifier, data, deadline);
        if (isTimeouted) {
            *isTimeouted = false;
        }
        switch (status) {
        case EReadStatus::NotFound:
            WARNING_LOG << "No task data in storage: " << identifier << Endl;
            return nullptr;
        case EReadStatus::Timeout:
            ERROR_LOG << "Timeout in reading data from storage: " << identifier << Endl;
            if (isTimeouted) {
                *isTimeouted = true;
            }
            return nullptr;
        case EReadStatus::OK:
            break;
        }

        TBlobWithHeader<NFrontendProto::TDataMeta> bwh;
        if (!bwh.Load(data)) {
            return nullptr;
        }

        const NFrontendProto::TDataMeta& dataMeta = bwh.GetHeader();

        TDataConstructionContext context(dataMeta.GetType(), identifier);
        result.Reset(IDistributedData::TFactory::Construct(dataMeta.GetType(), context));
        if (!result) {
            ERROR_LOG << "Incorrect data type: '" << dataMeta.GetType() << "'" << Endl;
            return nullptr;
        }
        if (!result->ParseMetaFromProto(dataMeta)) {
            ERROR_LOG << "Incorrect data meta: '" << dataMeta.DebugString() << "'" << Endl;
            return nullptr;
        }
        if (!result->Deserialize(bwh.GetData())) {
            WARNING_LOG << "Incorrect data in storage" << Endl;
            return nullptr;
        }
    }
    return result.Release();
}

IDDataStorage::TGuard::TPtr IDDataStorage::RestoreDataLinkUnsafe(const TString& identifier) const {
    THolder<IDistributedData> result(RestoreDataUnsafe(identifier));
    return MakeAtomicShared<TGuard>(this, result.Release());
}

bool IDDataStorage::StoreData2(const IDistributedData* data) const {
    if (data) {
        NFrontendProto::TDataMeta dataMeta;
        data->SerializeMetaToProto(dataMeta);

        TBlobWithHeader<NFrontendProto::TDataMeta> bwh(dataMeta, data->Serialize());

        return StoreDataImpl2("data--" + data->GetIdentifier(), bwh.Save());
    }
    return true;
}

