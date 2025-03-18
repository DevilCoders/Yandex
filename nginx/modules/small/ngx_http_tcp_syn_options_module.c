#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_variables.h>
#include <ngx_string.h>

#define MIN_MTU 1280

static ngx_str_t ngx_http_tcp_syn_options_variable_name = ngx_string("tcp_syn_options");
static ngx_int_t ngx_http_tcp_syn_options_add_variable(ngx_conf_t *);
static ngx_int_t ngx_http_tcp_syn_options_get_variable(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);

static ngx_http_module_t ngx_http_tcp_syn_options_module_ctx = {
    ngx_http_tcp_syn_options_add_variable,       /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    NULL,                                   /* create location configuration */
    NULL                                    /* merge location configuration */
};

ngx_module_t ngx_http_tcp_syn_options_module = {
    NGX_MODULE_V1,
    &ngx_http_tcp_syn_options_module_ctx,   /* module context */
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

static ngx_int_t
ngx_http_tcp_syn_options_add_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t *var = ngx_http_add_variable(cf,
            &ngx_http_tcp_syn_options_variable_name, NGX_HTTP_VAR_NOHASH);

    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_tcp_syn_options_get_variable;
    var->data = 0;

    return NGX_OK;
}

#ifdef TCP_SAVED_SYN
static ngx_uint_t
check_and_find_options(u_char** opts, ngx_uint_t size)
{
    if (size >= MIN_MTU - 60 || size <= 40) {
        return 0;
    }

    ngx_uint_t tcphdr_len = 20;
    u_char* buf = *opts;

#if NGX_HAVE_BIG_ENDIAN
    u_char ip_version = buf[0] & 0x0f;
    ngx_uint_t iphdr_len = (buf[0] >> 4) << 2;
#else
    u_char ip_version = buf[0] >> 4;
    ngx_uint_t iphdr_len = (buf[0] & 0x0f) << 2;
#endif
    if (ip_version == 6) {
        iphdr_len = 40;
    }

    if (iphdr_len + tcphdr_len >= size || (ip_version != 4 && ip_version != 6)) {
        return 0;
    }

#if NGX_HAVE_BIG_ENDIAN
    ngx_uint_t doff = (*(buf + iphdr_len + 12) & 0x0f) << 2;
#else
    ngx_uint_t doff = (*(buf + iphdr_len + 12) & 0xf0) >> 2;
#endif

    if (size < iphdr_len + doff) {
        return 0;
    }

    ngx_uint_t tcp_opts_len = doff - tcphdr_len;
    *opts = buf + iphdr_len + tcphdr_len;

    return tcp_opts_len;
}
#endif

static ngx_int_t
ngx_http_tcp_syn_options_get_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{

    v->len = 0;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 1;

#ifdef TCP_SAVED_SYN
    u_char buf[MIN_MTU];
    socklen_t size = sizeof(buf);

    while (r->connection->listening->tcp_save_syn) {
        if (getsockopt(r->connection->fd, IPPROTO_TCP, TCP_SAVED_SYN, buf, &size) != 0) {
            break;
        }

        u_char *opts = buf;
        ngx_uint_t tcp_opts_len = check_and_find_options(&opts, size);

        if (tcp_opts_len == 0) {
            break;
        }

        v->data = ngx_pnalloc(r->pool, 2 * tcp_opts_len);
        if (v->data == NULL) {
            return NGX_ERROR;
        }

        u_char *data = v->data;
        for (ngx_uint_t i=0; i<tcp_opts_len; ++i) {
            data = ngx_sprintf(data, "%02xi", *opts++);
        }
        v->len = 2 * tcp_opts_len;
        v->not_found = 0;

        return NGX_OK;
    }
#endif

    return NGX_OK;
}
