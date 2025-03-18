#include "p0f_bridge.h"

extern "C" ngx_str_t format_p0f_to_ngx_str(ngx_http_request_t* r,
                                           ngx_http_p0f_req_ctx_t* req_ctx, p0f_value_t* p0f_data) {
    try {
        auto result = NP0f::FormatP0f(*p0f_data);

        if (result.Value.Defined()) {
            auto str = result.Value.Get();
            auto c_str = static_cast<u_char*>(ngx_pnalloc(r->pool, str->size()));
            if (c_str == nullptr) {
                ngx_log_error(NGX_LOG_EMERG, r->connection->log, 0,
                              "allocation of p0f_fingerprint c_str failed");
                return ngx_null_string;
            }
            ngx_memcpy(c_str, str->c_str(), str->size());
            return {str->size(), c_str};
        } else {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, NGX_ERROR_INFO, "Invalid p0f format result: %s", result.Error.Get()->c_str());
        }
    } catch (...) {
    }

    return ngx_null_string;
}
