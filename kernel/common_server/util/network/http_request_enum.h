#pragma once

namespace NUtil {
    enum class EHttpReplyFlags {
        Timeout = 1 << 0 /* "timeout" */,
        Unknown = 1 << 1 /* "unknown" */,
        ErrorInResponse = 1 << 2 /* "error_in_response" */
    };
}