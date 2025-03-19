#pragma once

#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>

namespace NCS {

    class TDBDek {
    private:
        CS_ACCESS(TDBDek, TString, Id, TGUID::Create().AsUuidString());
        CSA_DEFAULT(TDBDek, TString, Encrypted);
        CSA_MUTABLE(TDBDek, ui32, TTL, 1000);

    public:
        using TId = TString;
        using TPtr = TAtomicSharedPtr<TDBDek>;

        TDBDek() = default;

        const TId& GetInternalId() const {
            return Id;
        }

        static TString GetTableName() {
            return "deks";
        }

        static TString GetIdFieldName() {
            return "id";
        }

        static TString GetHistoryTableName() {
            return "deks_history";
        }

        class TDecoder: public TBaseDecoder {
            DECODER_FIELD(Id);
            DECODER_FIELD(Encrypted);
            DECODER_FIELD(TTL);

        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase) {
                Id = GetFieldDecodeIndex("id", decoderBase);
                Encrypted = GetFieldDecodeIndex("encrypted", decoderBase);
                TTL = GetFieldDecodeIndex("ttl", decoderBase);
            }
        };

        bool operator!() const {
            return !Id;
        }

        NJson::TJsonValue SerializeToJson() const;
        bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
        NStorage::TTableRecord SerializeToTableRecord() const;
        bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

        static NFrontend::TScheme GetScheme(const IBaseServer& server);
    };
}
