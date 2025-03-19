#include "manager.h"
#include <kernel/common_server/util/json_processing.h>

namespace NCS {
    namespace NSecret {
        NFrontend::TScheme TDataToken::GetScheme(const IBaseServer& /*server*/) {
            NFrontend::TScheme result;
            result.Add<TFSString>("secret_id").SetRequired(false);
            result.Add<TFSString>("data").SetRequired(false);
            result.Add<TFSString>("data_hash").SetRequired(false);
            result.Add<TFSString>("data_type").SetRequired(false);
            result.Add<TFSString>("scope").SetRequired(false);
            return result;
        }

        NJson::TJsonValue TDataToken::SerializeToJson() const {
            NJson::TJsonValue result;
            result.InsertValue("secret_id", SecretId);
            result.InsertValue("data", Data);
            result.InsertValue("data_hash", DataHash);
            result.InsertValue("data_type", DataType);
            result.InsertValue("scope", Scope);
            return result;
        }

        bool TDataToken::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
            auto& idElement = TJsonProcessor::GetAvailableElement(jsonInfo, {"secret_id", "key"});
            if (idElement.IsString()) {
                SecretId = idElement.GetString();
            }
            auto& dataElement = TJsonProcessor::GetAvailableElement(jsonInfo, {"data", "value"});
            if (dataElement.IsString()) {
                Data = dataElement.GetString();
            }
            if (!TJsonProcessor::Read(jsonInfo, "data_hash", DataHash)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "data_type", DataType)) {
                return false;
            }
            if (!TJsonProcessor::Read(jsonInfo, "scope", Scope)) {
                return false;
            }
            return true;
        }

    }
}
