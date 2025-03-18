#include <nginx/modules/strm_packager/src/base/headers.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/string/split.h>

namespace NStrm::NPackager {
    THeadersOut::THeadersOut(
        const ::ngx_http_headers_out_t& headers)
        : Headers(headers)
    {
    }

    int THeadersOut::Status() const {
        return Headers.status;
    }

    TMaybe<ui64> THeadersOut::ContentLength() const {
        if (Headers.content_length_n < 0) {
            return {};
        } else {
            return Headers.content_length_n;
        }
    }

    TMaybe<TContentRange> THeadersOut::ContentRange() const {
        if (!Headers.content_range) {
            return {};
        }
        Y_ENSURE(Headers.content_range->value.data);

        TStringBuf str((char const*)Headers.content_range->value.data, Headers.content_range->value.len);
        TVector<TStringBuf> parts(4);
        Split(str, " -/", parts);
        Y_ENSURE(parts.size() == 4);
        Y_ENSURE(parts[0] == "bytes");

        return TContentRange{
            .Begin = FromString<ui64>(parts[1]),
            .End = FromString<ui64>(parts[2]) + 1,
            .Size = FromString<ui64>(parts[3]),
        };
    }

}
