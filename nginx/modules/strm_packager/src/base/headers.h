#pragma once

extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <util/generic/maybe.h>
#include <util/system/types.h>

namespace NStrm::NPackager {
    struct TContentRange {
        ui64 Begin;
        ui64 End;
        ui64 Size;
    };

    class THeadersOut: public TNonCopyable {
    public:
        explicit THeadersOut(const ::ngx_http_headers_out_t& headers);

        int Status() const;

        TMaybe<ui64> ContentLength() const;

        TMaybe<TContentRange> ContentRange() const;

    private:
        const ::ngx_http_headers_out_t& Headers;
    };

}
