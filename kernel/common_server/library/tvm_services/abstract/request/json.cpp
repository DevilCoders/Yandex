#include "json.h"
#include <library/cpp/json/json_reader.h>

namespace NExternalAPI {

    bool IHttpRequestWithJsonReport::TJsonResponse::DoParseJson(const NJson::TJsonValue& jsonInfo) {
        if (IsReplyCodeSuccess()) {
            return DoParseJsonReply(jsonInfo);
        } else {
            return DoParseJsonError(jsonInfo);
        }
    }

}
