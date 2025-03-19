#include "put_file.h"

#include <library/cpp/logger/global/global.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/json/json_reader.h>
#include <util/stream/str.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NDaemonController {
    NJson::TJsonValue TPutFileAction::DoSerializeToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("file_name", FileName);
        result.InsertValue("file_data", Base64Encode(FileData));
        return result;
    }

    void TPutFileAction::DoDeserializeFromJson(const NJson::TJsonValue& json) {
        NJson::TJsonValue::TMapType map;
        CHECK_WITH_LOG(json.GetMap(&map));
        FileName = map["file_name"].GetStringRobust();
        FileData = Base64Decode(map["file_data"].GetStringRobust());
    }

    void TPutFileAction::DoInterpretResult(const TString& result) {
        if (!result)
            Fail("Incorrect empty reply for put file request");
        else
            Success(result);
    }

    TPutFileAction::TFactory::TRegistrator<TPutFileAction> TPutFileAction::Registrator(PUT_FILE_ACTION_NAME);
}
