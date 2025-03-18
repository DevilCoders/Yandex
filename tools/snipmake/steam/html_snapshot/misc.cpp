#include "misc.h"
#include <library/cpp/http/fetch/exthttpcodes.h>
#include <yweb/webutil/url_fetcher_lib/fetcher.h>

namespace NSteam
{

bool CanRetry(int httpCode)
{
    switch (httpCode)
    {
        case HTTP_BAD_RESPONSE_HEADER:
        case HTTP_CONNECTION_LOST:
        case HTTP_BODY_TOO_LARGE:
        case HTTP_BAD_STATUS_CODE:
        case HTTP_BAD_HEADER_STRING:
        case HTTP_BAD_CHUNK:
        case HTTP_CONNECT_FAILED:
        case HTTP_LOCAL_EIO:
        case HTTP_BAD_CONTENT_LENGTH:
        case HTTP_BAD_ENCODING:
        case HTTP_LENGTH_UNKNOWN:
        case HTTP_HEADER_EOF:
        case HTTP_MESSAGE_EOF:
        case HTTP_CHUNK_EOF:
        case HTTP_PAST_EOF:
        case HTTP_HEADER_TOO_LARGE:
        case HTTP_INTERRUPTED:
        case HTTP_CUSTOM_NOT_MODIFIED:
        case HTTP_BAD_CONTENT_ENCODING:
        case HTTP_NO_RESOURCES:
        case HTTP_FETCHER_SHUTDOWN:
        case HTTP_CHUNK_TOO_LARGE:
        case HTTP_SERVER_BUSY:
        case HTTP_SERVICE_UNKNOWN:
        case HTTP_PROXY_UNKNOWN:
        case HTTP_PROXY_REQUEST_TIME_OUT:
        case HTTP_PROXY_INTERNAL_ERROR:
        case HTTP_PROXY_CONNECT_FAILED:
        case HTTP_PROXY_CONNECTION_LOST:
        case HTTP_PROXY_NO_PROXY:
        case HTTP_PROXY_ERROR:
            return true;
    };

    // These two sets of codes are overlapping for some reason
    switch (httpCode)
    {
        case RESULT_NO_LOCATION:
        case RESULT_INVALID_RESPONSE:
        case RESULT_STATUS_NOT_NUMERIC:
        case RESULT_CANNOT_CONNECT:
        case RESULT_WRONG_URL:
        case RESULT_CANNOT_CONNECT_INT:
        case RESULT_TIMEOUT:
        case RESULT_INTERNAL_PROBLEM:
        case RESULT_CANNOT_RESOLVE:
        case RESULT_MALFORMED_HTTP:
            return true;
    }

    return false;
}

}
