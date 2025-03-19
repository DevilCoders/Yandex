#include "direct.h"

namespace NExternalAPI {
    bool TServiceApiHttpDirectRequest::TResponse::DoParseReply(const NUtil::THttpReply& reply) {
        Reply = reply;
        return true;
    }

    bool TServiceApiHttpDirectRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
        if (RawRequest) {
            request = *RawRequest;
            return true;
        }
        return false;
    }

}
