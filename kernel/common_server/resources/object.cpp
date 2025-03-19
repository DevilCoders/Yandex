#include "object.h"

namespace NCS {

    namespace NResources {

        NJson::TJsonValue TDBResource::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            TJsonProcessor::Write(result, "resource_id", Id);
            TJsonProcessor::Write(result, "resource_key", Key);
            TJsonProcessor::Write(result, "revision", Revision);
            TJsonProcessor::WriteInstant(result, "deadline", Deadline);
            TJsonProcessor::Write(result, "access_id", AccessId);
            TJsonProcessor::Write(result, "container", Container.SerializeToJson());
            return result;
        }

        bool TDBResource::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TJsonProcessor::Read(jsonInfo, "resource_id", Id)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "resource_key", Key)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "revision", Revision)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "deadline", Deadline)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "access_id", AccessId)) {
                return false;
            }
            if (!Container.DeserializeFromJson(jsonInfo)) {
                return false;
            }
            return true;
        }

        NStorage::TTableRecord TDBResource::SerializeToTableRecord() const {
            NStorage::TTableRecord result;
            result.SetNotEmpty("resource_id", Id);
            result.SetNotEmpty("resource_key", Key);
            result.SetNotEmpty("access_id", AccessId);
            result.SetNotEmpty("deadline", Deadline);
            result.SetNotEmpty("class_name", Container.GetClassName());
            result.SetBytes("container", Container.SerializeToString());
            return result;
        }

        bool TDBResource::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            READ_DECODER_VALUE(decoder, values, Id);
            READ_DECODER_VALUE(decoder, values, Key);
            READ_DECODER_VALUE(decoder, values, AccessId);
            READ_DECODER_VALUE(decoder, values, Revision);
            READ_DECODER_VALUE_INSTANT_OPT(decoder, values, Deadline);
            TString className;
            READ_DECODER_VALUE_TEMP(decoder, values, className, ClassName);
            auto gLogging = TFLRecords::StartContext()("class_name", className)("key", Key);
            THolder<IResource> r = IResource::TFactory::MakeHolder(className);
            if (!r) {
                TFLEventLog::Log("incorrect class name for resource");
                return false;
            }
            TString containerData;
            if (!decoder.GetValueBytes(decoder.GetContainer(), values, containerData)) {
                TFLEventLog::Log("cannot read bytes from decoder");
                return false;
            }
            if (!r->DeserializeFromString(containerData)) {
                TFLEventLog::Log("cannot parse resource from string");
                return false;
            }
            Container = TResourceContainer(r.Release());
            return true;
        }

        NFrontend::TScheme TDBResource::GetScheme(const IBaseServer& server) {
            NFrontend::TScheme result;
            result.Add<TFSNumeric>("resource_id").SetReadOnly(true);
            result.Add<TFSString>("resource_key");
            result.Add<TFSString>("access_id");
            result.Add<TFSNumeric>("revision").SetReadOnly(true);
            result.Add<TFSNumeric>("deadline").SetVisual(TFSNumeric::EVisualTypes::DateTime);
            result.Add<TFSStructure>("container").SetStructure(TResourceContainer::GetScheme(server));
            return result;
        }
    }
}
