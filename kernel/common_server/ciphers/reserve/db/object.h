#pragma once

#include <kernel/common_server/library/storage/structured.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/abstract/frontend.h>

#include <library/cpp/digest/md5/md5.h>

namespace NCS {

    class TDBReserveEncrypted {
    private:
        CSA_DEFAULT(TDBReserveEncrypted, TString, CipherName);
        CSA_DEFAULT(TDBReserveEncrypted, ui64, ReserveId);
        CSA_DEFAULT(TDBReserveEncrypted, TString, Encrypted);
        CSA_DEFAULT(TDBReserveEncrypted, TString, Reserve);
        CSA_DEFAULT(TDBReserveEncrypted, TString, Hash);
        CSA_DEFAULT(TDBReserveEncrypted, TString, ReserveHash);

    public:
        using TId = ui64;

        TDBReserveEncrypted() = default;

        TDBReserveEncrypted(const TString& encrypted, const TString& reserve)
            : Encrypted(encrypted)
            , Reserve(reserve)
            , Hash(MD5::Calc(Encrypted))
            , ReserveHash(MD5::Calc(Reserve))
        {
        }

        const ui64& GetInternalId() const {
            return ReserveId;
        }

        static TString GetTableName() {
            return "reserve_ciphers";
        }

        static TString GetIdFieldName() {
            return "reserve_id";
        }

        static TString GetHistoryTableName() {
            return GetTableName() + "_history";
        }

        class TDecoder: public TBaseDecoder {
            DECODER_FIELD(ReserveId);
            DECODER_FIELD(Encrypted);
            DECODER_FIELD(Reserve);
            DECODER_FIELD(Hash);
            DECODER_FIELD(ReserveHash);

        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase) {
                ReserveId = GetFieldDecodeIndex("reserve_id", decoderBase);
                Encrypted = GetFieldDecodeIndex("encrypted", decoderBase);
                Reserve = GetFieldDecodeIndex("reserve", decoderBase);
                Hash = GetFieldDecodeIndex("hash", decoderBase);
                ReserveHash = GetFieldDecodeIndex("reserve_hash", decoderBase);
            }
        };

        bool operator!() const {
            return !ReserveId;
        }

        NJson::TJsonValue SerializeToJson() const;
        bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
        NStorage::TTableRecord SerializeToTableRecord() const;
        bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);

        static NFrontend::TScheme GetScheme(const IBaseServer& server);
    };
}
