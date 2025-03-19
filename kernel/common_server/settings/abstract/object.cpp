#include "object.h"
#include <kernel/common_server/library/scheme/fields.h>

namespace NCS {
    TSetting::TDecoder::TDecoder(const TMap<TString, ui32>& decoderBase) {
        {
            auto it = decoderBase.find("setting_key");
            CHECK_WITH_LOG(it != decoderBase.end());
            Key = it->second;
        }
        {
            auto it = decoderBase.find("setting_value");
            CHECK_WITH_LOG(it != decoderBase.end());
            Value = it->second;
        }
    }

    bool TSetting::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
        READ_DECODER_VALUE(decoder, values, Key);
        READ_DECODER_VALUE(decoder, values, Value);
        if (!Key) {
            return false;
        }
        return true;
    }

    bool TSetting::Parse(const NCS::NStorage::TTableRecordWT& record) {
        return DeserializeFromTableRecord(record, nullptr);
    }

    bool TSetting::DeserializeFromTableRecord(const NCS::NStorage::TTableRecordWT& record, const IHistoryContext* /*context*/) {
        Key = record.CastTo<TString>("setting_key").GetOrElse("");
        if (!Key) {
            return false;
        }
        Value = record.CastTo<TString>("setting_value").GetOrElse("");
        return true;
    }

    NCS::NStorage::TTableRecord TSetting::SerializeToTableRecord() const {
        NCS::NStorage::TTableRecord result;
        result.Set("setting_key", Key).Set("setting_value", Value);
        return result;
    }

    NCS::NStorage::TTableRecord TSetting::SerializeUniqueToTableRecord() const {
        NCS::NStorage::TTableRecord result;
        result.Set("setting_key", Key);
        return result;
    }

    NCS::NScheme::TScheme TSetting::GetScheme(const IBaseServer& /*server*/) {
        NCS::NScheme::TScheme scheme;
        scheme.Add<NCS::NScheme::TFSString>("setting_key", "setting_key");
        scheme.Add<NCS::NScheme::TFSString>("setting_value", "setting_value").SetMultiLine(true);
        return scheme;
    }


    bool TSetting::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
        NCS::NStorage::TTableRecordWT record;
        if (!record.DeserializeFromJson(jsonInfo)) {
            return false;
        }
        return DeserializeFromTableRecord(record, nullptr);
    }

    NJson::TJsonValue TSetting::SerializeToJson() const {
        return SerializeToTableRecord().BuildWT().SerializeToJson();
    }
}
