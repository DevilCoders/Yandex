#include "get_status.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <util/stream/str.h>

namespace NDaemonController {
    NJson::TJsonValue TStatusAction::DoSerializeToJson() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        for (auto&& status : StatusAim) {
            result["status_aim"].AppendValue(status);
        }
        for (auto&& status : ServerStatusAim) {
            result["server_status_aim"].AppendValue(status);
        }
        return result;
    }

    void TStatusAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(json.GetMap(&map));
        NJson::TJsonValue::TArray arr = map["status_aim"].GetArray();
        for (auto&& i : arr) {
            StatusAim.insert(i.GetString());
        }
        arr = map["server_status_aim"].GetArray();
        for (auto&& i : arr) {
            ServerStatusAim.insert(i.GetString());
        }
    }

    void TStatusAction::DoInterpretResult(const TString& result) {
        NJson::TJsonValue json;
        TStringStream ssJson;
        ssJson << result;
        if (!NJson::ReadJsonTree(&ssJson, &json)) {
            ERROR_LOG << "Incorrect json about controller status" << Endl;
            Fail("Incorrect reply : " + result);
            return;
        }

        NJson::TJsonValue::TMapType map;

        if (!json.GetMap(&map)) {
            ERROR_LOG << "Incorrect json about controller status (not map)" << Endl;
            Fail("Incorrect reply - json content isn't map : " + result);
            return;
        }

        bool statusAim = !StatusAim.size() || StatusAim.contains(map["status"].GetStringRobust());
        bool serverStatusAim = !ServerStatusAim.size() || ServerStatusAim.contains(map["result"]["server_status_global"]["state"].GetStringRobust());

        if (statusAim && serverStatusAim) {
            Success(map["status"].GetStringRobust());
        } else {
            Fail(map["status"].GetStringRobust());
        }
    }

    TStatusAction::TFactory::TRegistrator<TStatusAction> TStatusAction::Registrator(STATUS_ACTION_NAME);
}
