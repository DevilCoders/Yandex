#include "take_file.h"

#include <library/cpp/logger/global/global.h>

namespace NDaemonController {
    NJson::TJsonValue TTakeFileAction::DoSerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("file_name", FileName);
        result.InsertValue("from_host", FromHost);
        result.InsertValue("from_port", FromPort);
        result.InsertValue("url", Url);
        result.InsertValue("post", Base64Encode(Post));
        result.InsertValue("sleep_duration", SleepDurationMs);

        return result;
    }

    void TTakeFileAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(json.GetMap(&map));
        FileName = map["file_name"].GetStringRobust();
        FromHost = map["from_host"].GetStringRobust();
        FromPort = map["from_port"].GetUIntegerRobust();
        Url = map["url"].GetStringRobust();
        Post = Base64Decode(map["post"].GetStringRobust());
        if (map.contains("sleep_duration"))
            SleepDurationMs = map["sleep_duration"].GetUInteger();
    }

    void TTakeFileAction::DoInterpretResult(const TString& result) {
        if (!result)
            Fail("Incorrect empty reply for put file request");
        else
            Success(result);
    }

    TTakeFileAction::TFactory::TRegistrator<TTakeFileAction> TTakeFileAction::Registrator(TAKE_FILE_ACTION_NAME);
}
