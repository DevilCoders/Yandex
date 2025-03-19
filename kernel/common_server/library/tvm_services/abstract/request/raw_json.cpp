#include "raw_json.h"

namespace NExternalAPI {

    bool TRawJsonRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        request.SetUri(Uri);
        if (!PostData.IsNull()) {
            request.SetPostData(PostData);
        }
        for (auto header : Headers) {
            request.AddHeader(header.first, header.second);
        }
        return true;
    }

    TRawJsonRequest::TRawJsonRequest(const NJson::TJsonValue& postData, const TString& path, const TMap<TString, TString>& headers)
        : PostData(postData)
        , Headers(headers)
        , Uri(path)
    {

    }

    bool TRawJsonRequest::TResponse::DoParseJsonReply(const NJson::TJsonValue& jsonReply) {
        JsonReply = jsonReply;
        return true;
    }

}
