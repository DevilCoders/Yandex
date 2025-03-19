#include "delete_file.h"

#include <library/cpp/json/writer/json_value.h>
#include <util/stream/str.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/global/global.h>

namespace NDaemonController {
    NJson::TJsonValue TDeleteFileAction::DoSerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("file_name", FileName);
        return result;
    }

    void TDeleteFileAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(json.GetMap(&map));
        FileName = map["file_name"].GetStringRobust();
    }

    void TDeleteFileAction::DoInterpretResult(const TString& result) {
        if (!result)
            Fail("Incorrect empty reply for delete file request");
        else
            Success(result);
    }

    TDeleteFileAction::TFactory::TRegistrator<TDeleteFileAction> TDeleteFileAction::Registrator(DELETE_FILE_ACTION_NAME);
}
