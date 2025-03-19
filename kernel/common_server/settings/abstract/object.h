#pragma once
#include <kernel/common_server/util/accessor.h>
#include <util/string/join.h>
#include <util/generic/fwd.h>
#include <kernel/common_server/library/storage/records/record.h>
#include <kernel/common_server/library/storage/records/t_record.h>
#include <kernel/common_server/library/storage/reply/decoder.h>
#include <kernel/common_server/library/scheme/scheme.h>

class IHistoryContext;
class IBaseServer;

namespace NCS {
    class TSetting {
    public:
        CSA_DEFAULT(TSetting, TString, Key);
        CSA_DEFAULT(TSetting, TString, Value);

    public:
        class TDecoder: public NCS::NStorage::TBaseDecoder {
        public:
            DECODER_FIELD(Key);
            DECODER_FIELD(Value);

        public:
            TDecoder() = default;
            TDecoder(const TMap<TString, ui32>& decoderBase);
        };

    public:
        TSetting() = default;
        TSetting(const TString& key, const TString& value)
            : Key(key)
            , Value(value) {
        }

        bool operator<(const TSetting& item) const {
            return Key < item.Key;
        }

        Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);
        NJson::TJsonValue SerializeToJson() const;

        Y_WARN_UNUSED_RESULT bool DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values);
        bool Parse(const NCS::NStorage::TTableRecordWT& record);
        static NCS::NScheme::TScheme GetScheme(const IBaseServer& server);

        Y_WARN_UNUSED_RESULT bool DeserializeFromTableRecord(const NCS::NStorage::TTableRecordWT& record, const IHistoryContext* /*context*/);
        NCS::NStorage::TTableRecord SerializeToTableRecord() const;
        NCS::NStorage::TTableRecord SerializeUniqueToTableRecord() const;
    };

}

namespace NFrontend {
    using TSetting = NCS::TSetting;
}
