#include "index_info.h"

#include <util/stream/file.h>
#include <util/string/cast.h>

#include <library/cpp/protobuf/json/json2proto.h>
#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/svnversion/svnversion.h>

namespace NDoom {

TIndexInfo MakeDefaultIndexInfo() {
    TIndexInfo result;
    result.SetSvnVersion(ToString(GetProgramSvnRevision()));
    return result;
}

void SaveIndexInfo(IOutputStream* stream, const TIndexInfo& data) {
    NProtobufJson::TProto2JsonConfig config;
    config.SetFormatOutput(true);

    NProtobufJson::Proto2Json(data, *stream, config);
}

TIndexInfo LoadIndexInfo(IInputStream* stream) {
    return NProtobufJson::Json2Proto<TIndexInfo>(*stream);
}

TIndexInfo LoadIndexInfo(const TString& indexPath, const TIndexInfo& defaultInfo) {
    try {
        TString localPath = indexPath;
        if (localPath.EndsWith('.'))
            localPath.pop_back();

        TIFStream stream(localPath + ".info");

        return LoadIndexInfo(&stream);
    } catch (...) {
        return defaultInfo;
    }
}

} // namespace NDoom
