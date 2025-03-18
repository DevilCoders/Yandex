#pragma once

#include <library/cpp/logger/priority.h>

namespace NStrm::NPackager {
    inline ngx_uint_t ToNgxLogLevel(const ELogPriority level) {
        ngx_uint_t ngxLevel = 0;
        switch (level) {
            case TLOG_EMERG:
                ngxLevel = NGX_LOG_EMERG;
                break;
            case TLOG_ALERT:
                ngxLevel = NGX_LOG_ALERT;
                break;
            case TLOG_CRIT:
                ngxLevel = NGX_LOG_CRIT;
                break;
            case TLOG_ERR:
                ngxLevel = NGX_LOG_ERR;
                break;
            case TLOG_WARNING:
                ngxLevel = NGX_LOG_WARN;
                break;
            case TLOG_NOTICE:
                ngxLevel = NGX_LOG_NOTICE;
                break;
            case TLOG_INFO:
                ngxLevel = NGX_LOG_INFO;
                break;
            case TLOG_DEBUG:
            case TLOG_RESOURCES:
                ngxLevel = NGX_LOG_DEBUG;
        };
        return ngxLevel;
    }
}
