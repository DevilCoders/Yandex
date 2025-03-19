#include "object.h"

namespace NCS {
    namespace NObfuscator {

        NJson::TJsonValue TDBObfuscator::SerializeToJson() const {
            NJson::TJsonValue result = TBase::SerializeToJson();
            JWRITE(result, "obfuscator_id", ObfuscatorId);
            JWRITE(result, "revision", Revision);
            JWRITE(result, "name", Name);
            JWRITE(result, "priority", Priority);
            return result;
        }

        Y_WARN_UNUSED_RESULT bool TDBObfuscator::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            if (!TBase::DeserializeFromJson(jsonInfo)) {
                return false;
            }
            JREAD_UINT(jsonInfo, "obfuscator_id", ObfuscatorId);
            JREAD_UINT(jsonInfo, "revision", Revision);
            JREAD_STRING(jsonInfo, "name", Name);
            JREAD_UINT_OPT(jsonInfo, "priority", Priority);
            return true;
        }

        Y_WARN_UNUSED_RESULT bool TDBObfuscator::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
            if (!TBase::DeserializeWithDecoder(decoder, values)) {
                return false;
            }
            READ_DECODER_VALUE(decoder, values, ObfuscatorId);
            READ_DECODER_VALUE(decoder, values, Revision);
            READ_DECODER_VALUE(decoder, values, Name);
            READ_DECODER_VALUE(decoder, values, Priority);
            return true;
        }
        NStorage::TTableRecord TDBObfuscator::SerializeToTableRecord() const {
            NStorage::TTableRecord result = TBase::SerializeToTableRecord();
            if (ObfuscatorId) {
                result.Set("obfuscator_id", ObfuscatorId);
            }
            result.Set("name", Name);
            result.Set("revision", Revision);
            result.Set("priority", Priority);
            return result;
        }

        NFrontend::TScheme TDBObfuscator::GetScheme(const IBaseServer& server) {
            NFrontend::TScheme result = TBase::GetScheme(server);
            result.Add<TFSNumeric>("obfuscator_id").SetRequired(true);
            result.Add<TFSString>("name").SetRequired(true);
            result.Add<TFSNumeric>("priority").SetRequired(false);
            result.Add<TFSNumeric>("revision").SetRequired(true).SetReadOnly(true);
            return result;
        }

    }
}
