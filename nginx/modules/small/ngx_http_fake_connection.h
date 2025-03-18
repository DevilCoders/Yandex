#pragma once

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_http_request_t     *r;
    ngx_chain_t            *data;
    off_t                   header_remain;
} ngx_httt_fake_conn_ctx_t;


ngx_connection_t * ngx_http_create_fake_connection(ngx_http_core_srv_conf_t *conf, ngx_log_t *log);
ngx_http_request_t * ngx_http_create_fake_request(ngx_connection_t *fc, ngx_str_t *uri, ngx_http_post_subrequest_t *ps);
