#include "localization.h"
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/library/unistat/cache.h>
namespace NCS {
    namespace NLocalization {

        NJson::TJsonValue TResource::TResourceLocalization::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            result.InsertValue("l", Localization);
            result.InsertValue("v", Value);
            return result;
        }

        bool TResource::TResourceLocalization::DeserializeFromJson(const NJson::TJsonValue& info) {
            JREAD_STRING(info, "l", Localization);
            JREAD_STRING(info, "v", Value);
            return true;
        }

        TMaybe<TString> TResource::GetLocalization(const TString& localizationId) const {
            for (auto&& i : Localizations) {
                if (localizationId == i.GetLocalization()) {
                    return i.GetValue();
                }
            }
            return {};
        }

        bool TResource::DeserializeFromJson(const NJson::TJsonValue& info) {
            if (!DeserializeDataFromJson(info)) {
                return false;
            }
            if (!info["resource_id"].GetString(&Id) || !Id) {
                return false;
            }
            return true;
        }

        bool TResource::DeserializeDataFromJson(const NJson::TJsonValue& info) {
            const NJson::TJsonValue::TArray* arr;
            if (!info["localizations"].GetArrayPointer(&arr)) {
                return false;
            }
            for (auto&& i : *arr) {
                TResourceLocalization rl;
                if (rl.DeserializeFromJson(i)) {
                    Localizations.emplace_back(std::move(rl));
                } else {
                    ERROR_LOG << "Cannot parse from TResourceLocalization from " << i << Endl;
                }
            }
            return true;
        }

        NStorage::TTableRecord TResource::SerializeToTableRecord() const {
            NStorage::TTableRecord result;
            result.Set("resource_id", Id).Set("resource_data", SerializeDataToJson());
            result.SetNotEmpty("revision", Revision);
            return result;
        }

        NJson::TJsonValue TResource::SerializeDataToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            NJson::TJsonValue& localizations = result.InsertValue("localizations", NJson::JSON_ARRAY);
            for (auto&& i : Localizations) {
                localizations.AppendValue(i.SerializeToJson());
            }
            return result;
        }

        NJson::TJsonValue TResource::SerializeToJson() const {
            NJson::TJsonValue result = SerializeDataToJson();
            result.InsertValue("resource_id", Id);
            result.InsertValue("revision", Revision);
            return result;
        }

        bool TResource::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE(decoder, values, Id);
            if (!Id) {
                return false;
            }
            READ_DECODER_VALUE(decoder, values, Revision);
            NJson::TJsonValue jsonMeta;
            READ_DECODER_VALUE_JSON(decoder, values, jsonMeta, Meta);
            return DeserializeDataFromJson(jsonMeta);
        }

        TResource::TResource(const TString& id)
            : Id(id) {

        }

        TMaybe<TString> TLocalizationDB::GetLocalStringImpl(const TString& localizationId, const TString& resourceId) const {
            TVector<TResource> resouces;
            auto snapshot = GetSnapshots().GetMainSnapshotPtr();
            auto object = snapshot->GetObjectById(resourceId);
            if (!object) {
                TCSSignals::SignalAdd("frontend", "localization-problems", 1);
                WARNING_LOG << "Localization problem with key " << localizationId << "." << resourceId << Endl;
                return {};
            } else {
                TCSSignals::SignalAdd("frontend", "localization-success", 1);
            }
            return object->GetLocalization(localizationId);
        }

        TResource::TDecoder::TDecoder(const TMap<TString, ui32>& decoderBase) {
            Id = GetFieldDecodeIndex("resource_id", decoderBase);
            Meta = GetFieldDecodeIndex("resource_data", decoderBase);
            Revision = GetFieldDecodeIndex("revision", decoderBase);
        }
    }
}
