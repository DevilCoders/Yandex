#include "zip.h"

namespace NExternalAPI {

    bool IHttpRequestWithZipReport::TZipResponse::DoParseZip(const TStringBuf data) {
        if (IsReplyCodeSuccess()) {
            return DoParseZipReply(data);
        } else {
            return DoParseZipError(data);
        }
    }
}
