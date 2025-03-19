#include "shutdown.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <util/stream/str.h>

namespace NDaemonController {
    NJson::TJsonValue TShutdownAction::DoSerializeToJson() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        return result;
    }

    void TShutdownAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(json.GetMap(&map));
    }

    void TShutdownAction::DoInterpretResult(const TString& result) {
        if (!result) {
            ERROR_LOG << "Empty reply" << Endl;
            Fail("Empty reply");
            return;
        }
        Success("shutdown");
    }

    TShutdownAction::TFactory::TRegistrator<TShutdownAction> TShutdownAction::Registrator(SHUTDOWN_ACTION_NAME);
}
