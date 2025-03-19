#include "manager.h"

namespace NCS {
    namespace NSecret {

        bool TKVManager::DoEncode(TVector<TDataToken>& data, const bool /*writeIfNotExists*/) const {
            for (auto&& i : data) {
                NKVStorage::TWriteOptions writeOptions;
                writeOptions.SetTimestamp(i.GetTimestamp()).SetUserId(i.GetUserId()).SetDataHash(i.GetDataHash()).SetVisibilityScope(i.GetScope());
                const TString newId = !!i.GetSecretId() ? i.GetSecretId() : TGUID::CreateTimebased().AsGuidString();
                const auto blob = TBlob::FromString(i.GetData());
                if (!AllowDuplicate) {
                    NKVStorage::TReadOptions readOptions;
                    readOptions.SetUserId(i.GetUserId()).SetDataHash(i.GetDataHash());
                    const auto key = Storage->GetPathByHash(blob, readOptions);
                    if (!!key) {
                        i.SetSecretId(key);
                        continue;
                    }
                    if (!Storage->PutData(newId, blob, writeOptions)) {
                        TFLEventLog::Error("cannot put data")("data_hash", i.GetDataHash());
                        return false;
                    }
                } else {
                    if (!Storage->PutDataAsync(newId, blob, writeOptions)) {
                        TFLEventLog::Error("cannot put data")("data_hash", i.GetDataHash());
                        return false;
                    }
                }
                i.SetSecretId(newId);
            }
            return true;
        }

        bool TKVManager::DoDecode(TVector<TDataToken>& data, const bool /*forceCheckExistance*/) const {
            if (!Storage) {
                return false;
            }
            for (auto& i : data) {
                TMaybe<TBlob> res;
                NKVStorage::TReadOptions readOptions;
                readOptions.SetUserId(i.GetUserId()).SetVisibilityScope(i.GetScope());
                if (!Storage->GetData(i.GetSecretId(), res, readOptions)) {
                    TFLEventLog::Error("cannot get data")("key", i.GetSecretId());
                    return false;
                }
                if (!res) {
                    TFLEventLog::Error("empty data");
                    return false;
                }
                i.SetData(TString(res->AsStringBuf()));
            }
            return true;
        }
    }
}
