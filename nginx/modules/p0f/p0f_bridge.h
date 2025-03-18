#pragma once

#include <kernel/p0f/format/p0f_format.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "ngx_http.h"

    typedef struct {
        p0f_value_t p0f;
        ngx_str_t p0f_fingerprint;
    } ngx_http_p0f_req_ctx_t;

    ngx_str_t format_p0f_to_ngx_str(ngx_http_request_t* r,
                                    ngx_http_p0f_req_ctx_t* req_ctx, p0f_value_t* p0f_data);

#ifdef __cplusplus
}
#endif
