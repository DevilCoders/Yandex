#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_variables.h>
#include <ngx_string.h>

#include <sys/socket.h>

#include <contrib/libs/linux-headers/linux/inet_diag.h>

#ifndef TCP_CC_INFO
#define TCP_CC_INFO     26  /* Get Congestion Control (optional) info */
#endif


typedef enum {
    TCP_CC_INFO_BBR_BANDWIDTH,
    TCP_CC_INFO_BBR_RTT,
} tcp_cc_info_variable_e;

typedef enum {
    TCP_CC_INFO_STATE_NOT_INITED,
    TCP_CC_INFO_STATE_BBR,
    TCP_CC_INFO_STATE_OTHER,
} tcp_cc_info_state_e;


typedef struct {
    ngx_int_t           state;
    ngx_int_t           bytes_sent;
    union tcp_cc_info   info;
} tcp_cc_info_cache;


typedef struct {
    tcp_cc_info_cache cc;
} ngx_http_yandex_vars_module_ctx_t;


static ngx_int_t ngx_http_yandex_vars_add_variable(ngx_conf_t *);
static ngx_int_t ngx_http_yandex_vars_get_variable(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);
static ngx_int_t ngx_http_yandex_request_time_ms(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);
static ngx_int_t ngx_http_yandex_timestamp_seconds(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_yandex_timestamp_milliseconds(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_yandex_request_first_recv_timestamp_usec(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);
static ngx_int_t ngx_http_yandex_tcp_cc_info_variable(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);
static ngx_int_t ngx_http_yandex_tcp_cc_info_make_variable(ngx_http_request_t *r,
        ngx_http_variable_value_t *v, uintptr_t data, tcp_cc_info_cache *cc_cache);
static ngx_int_t ngx_http_yandex_tcp_cc_info_variable_recalculate(ngx_http_request_t *r,
        ngx_http_yandex_vars_module_ctx_t *ctx, ngx_http_variable_value_t *v, uintptr_t data);


static ngx_http_module_t ngx_http_yandex_vars_module_ctx = {
    ngx_http_yandex_vars_add_variable,      /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};


ngx_module_t ngx_http_yandex_vars_module = {
    NGX_MODULE_V1,
    &ngx_http_yandex_vars_module_ctx,       /* module context */
    NULL,                                   /* module directives */
    NGX_HTTP_MODULE,                        /* module type */
    NULL,                                   /* init master */
    NULL,                                   /* init module */
    NULL,                                   /* init process */
    NULL,                                   /* init thread */
    NULL,                                   /* exit thread */
    NULL,                                   /* exit process */
    NULL,                                   /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_variable_t  ngx_http_yandex_vars[] = {

    { ngx_string("request_time_ms"), NULL,
      ngx_http_yandex_request_time_ms, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_connect_time_ms"), NULL,
      ngx_http_yandex_vars_get_variable, 2,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_header_time_ms"), NULL,
      ngx_http_yandex_vars_get_variable, 1,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_response_time_ms"), NULL,
      ngx_http_yandex_vars_get_variable, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("timestamp_seconds"), NULL,
      ngx_http_yandex_timestamp_seconds, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("timestamp_milliseconds"), NULL,
      ngx_http_yandex_timestamp_milliseconds, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("request_first_recv_timestamp_usec"), NULL,
      ngx_http_yandex_request_first_recv_timestamp_usec, 0,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("bbr_bandwidth"), NULL,
      ngx_http_yandex_tcp_cc_info_variable, TCP_CC_INFO_BBR_BANDWIDTH,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("bbr_rtt"), NULL,
      ngx_http_yandex_tcp_cc_info_variable, TCP_CC_INFO_BBR_RTT,
      NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_int_t
ngx_http_yandex_vars_add_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_yandex_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_yandex_request_time_ms(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char          *p;
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 6);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = ngx_max(ms, 0);

    p = ngx_sprintf(p, "%d", ms * 1000);

    v->len = p - v->data;
    return NGX_OK;
}


static ngx_int_t
ngx_http_yandex_vars_get_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                     *p;
    size_t                      len;
    ngx_uint_t                  i;
    ngx_msec_int_t              ms;
    ngx_http_upstream_state_t  *state;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (r->upstream_states == NULL || r->upstream_states->nelts == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = r->upstream_states->nelts * (NGX_TIME_T_LEN + 6 + 2);

    p = ngx_pnalloc(r->pool, len);
    if (p == NULL) {
        return NGX_ERROR;
    }

    v->data = p;

    i = 0;
    state = r->upstream_states->elts;

    for ( ;; ) {
        if (state[i].status) {
            if (data == 1 && state[i].header_time != (ngx_msec_t) -1) {
                  ms = (ngx_msec_int_t)state[i].header_time;

              } else if (data == 2 && state[i].connect_time != (ngx_msec_t) -1) {
                  ms = (ngx_msec_int_t)state[i].connect_time;

              } else {
                  ms = (ngx_msec_int_t)state[i].response_time;
              }
            ms = ngx_max(ms, 0);
            p = ngx_sprintf(p, "%d", ms * 1000);

        } else {
            *p++ = '-';
        }

        if (++i == r->upstream_states->nelts) {
            break;
        }

        if (state[i].peer) {
            *p++ = ',';
            *p++ = ' ';

        } else {
            *p++ = ' ';
            *p++ = ':';
            *p++ = ' ';

            if (++i == r->upstream_states->nelts) {
                break;
            }

            continue;
        }
    }

    v->len = p - v->data;
    return NGX_OK;
}

static ngx_int_t
ngx_http_yandex_timestamp_seconds(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    v->len = ngx_sprintf(p, "%T", tp->sec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}

static ngx_int_t
ngx_http_yandex_timestamp_milliseconds(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char      *p;
    ngx_time_t  *tp;

    p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 3);
    if (p == NULL) {
        return NGX_ERROR;
    }

    tp = ngx_timeofday();

    v->len = ngx_sprintf(p, "%T%03M", tp->sec, tp->msec) - p;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    v->data = p;

    return NGX_OK;
}

static ngx_int_t
ngx_http_yandex_request_first_recv_timestamp_usec(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
#if (NGX_HAVE_SO_TIMESTAMP)
    u_char          *p;

    if (r->first_recv_timestamp_usec == 0) {
        v->data = (u_char*)"";
        v->len = 0;
    } else {
        p = ngx_pnalloc(r->pool, NGX_TIME_T_LEN + 6);
        if (p == NULL) {
            return NGX_ERROR;
        }
        v->data = p;
        p = ngx_sprintf(p, "%uL", r->first_recv_timestamp_usec);
        v->len = p - v->data;
    }
#else
    v->data = (u_char*)"";
    v->len = 0;
#endif

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t ngx_http_yandex_tcp_cc_info_make_variable(ngx_http_request_t *r,
        ngx_http_variable_value_t *v, uintptr_t data, tcp_cc_info_cache *cc_cache)
{
    uint64_t value;

    v->data = ngx_pnalloc(r->pool, NGX_INT64_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    if (cc_cache->state != TCP_CC_INFO_STATE_BBR) {
        v->not_found = 1;
        return NGX_OK;
    }

    switch (data) {
        case TCP_CC_INFO_BBR_BANDWIDTH:         // bandwidth in bits per second
            value = cc_cache->info.bbr.bbr_bw_hi;
            value <<= 32;
            value = cc_cache->info.bbr.bbr_bw_lo;
            value *= 8;
            break;

        case TCP_CC_INFO_BBR_RTT:               // rtt in milliseconds
            value = cc_cache->info.bbr.bbr_min_rtt;
            break;

        default:
            v->not_found = 1;
            return NGX_OK;
    }

    v->len = ngx_sprintf(v->data, "%uD", value) - v->data;
    v->valid = 1;
    v->no_cacheable = 1;
    v->not_found = 0;

    return NGX_OK;
}


static ngx_int_t ngx_http_yandex_tcp_cc_info_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_yandex_vars_module_ctx_t *ctx;
    ngx_int_t                         rc;

    ctx = ngx_http_get_module_ctx(r, ngx_http_yandex_vars_module);
    if (ctx == NULL) {
        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_yandex_vars_module_ctx_t));
        if (ctx == NULL) {
            return NGX_ERROR;
        }
        ngx_http_set_ctx(r, ctx, ngx_http_yandex_vars_module);
    }
    if (ctx->cc.state == TCP_CC_INFO_STATE_NOT_INITED
            || ctx->cc.bytes_sent < r->connection->sent) {
        rc = ngx_http_yandex_tcp_cc_info_variable_recalculate(r, ctx, v, data);
        if (rc != NGX_OK) {
            return rc;
        }
    }

    return ngx_http_yandex_tcp_cc_info_make_variable(r, v, data, &ctx->cc);
}


static ngx_int_t ngx_http_yandex_tcp_cc_info_variable_recalculate(ngx_http_request_t *r,
        ngx_http_yandex_vars_module_ctx_t *ctx, ngx_http_variable_value_t *v, uintptr_t data)
{
    u_char                           congestion[32];
    socklen_t                        congestion_len = 32;
    socklen_t                        len;

    if (ctx->cc.state == TCP_CC_INFO_STATE_NOT_INITED) {
        if (getsockopt(r->connection->fd, IPPROTO_TCP, TCP_CONGESTION, congestion, &congestion_len) != 0) {
            return NGX_ERROR;
        }

        ctx->cc.state = TCP_CC_INFO_STATE_OTHER;
        if (ngx_strcmp(congestion, "bbr") == 0) {
            ctx->cc.state = TCP_CC_INFO_STATE_BBR;
        }
    }

    ctx->cc.bytes_sent = r->connection->sent;

    if (ctx->cc.state != TCP_CC_INFO_STATE_BBR) {
        return NGX_OK;
    }

    len = sizeof(ctx->cc.info);
    if (getsockopt(r->connection->fd, SOL_TCP, TCP_CC_INFO, &ctx->cc.info, &len) != 0) {
        return NGX_ERROR;
    }

    return NGX_OK;
}
