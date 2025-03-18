#include "job_context.h"

#include <library/cpp/json/json_reader.h>

#include <util/stream/file.h>
#include <util/folder/path.h>
#include <util/system/env.h>
#include <util/system/fs.h>

using namespace NNirvana;

TString NNirvana::JobContextFilePath() {
    return JoinFsPaths(GetEnv("JWD", "."), "job_context.json");
}

TMaybe<NJson::TJsonValue> NNirvana::NPrivate::ReadJson(const TString& filePath) {
    if (!NFs::Exists(filePath)) {
        return {};
    }
    NJson::TJsonValue result;
    TFileInput file(filePath);
    NJson::ReadJsonTree(&file, &result, true);
    return result;
}
