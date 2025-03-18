#pragma once

#include <library/cpp/logger/priority.h>
#include <util/generic/yexception.h>

namespace NStrm::NPackager {
    class THttpError: public yexception {
    public:
        THttpError(int status, ELogPriority logPriority, const yexception& ye)
            : yexception(ye)
            , Status(status)
            , LogPriority(logPriority)
            , NgxSendError(false)
        {
        }

        THttpError(int status, ELogPriority logPriority, bool ngxSendError = false)
            : Status(status)
            , LogPriority(logPriority)
            , NgxSendError(ngxSendError)
        {
        }

        const int Status;
        const ELogPriority LogPriority;
        const bool NgxSendError;
    };

    inline int ProxyBadUpstreamHttpCode(int code) {
        return code == 404 ? 404 : 502;
    }
}
