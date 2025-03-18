#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_http_complex_value_t      *fwmark;
} ngx_http_fwmark_loc_conf_t;


static void *ngx_http_fwmark_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_fwmark_merge_loc_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_fwmark_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_fwmark_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_fwmark_commands[] = {

    { ngx_string("fwmark"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_fwmark_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_fwmark_loc_conf_t, fwmark),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_fwmark_module_ctx = {
    NULL,                          /* preconfiguration */
    ngx_http_fwmark_init,          /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    ngx_http_fwmark_create_loc_conf,   /* create location configuration */
    ngx_http_fwmark_merge_loc_conf     /* merge location configuration */
};


ngx_module_t  ngx_http_fwmark_module = {
    NGX_MODULE_V1,
    &ngx_http_fwmark_module_ctx,   /* module context */
    ngx_http_fwmark_commands,      /* module directives */
    NGX_HTTP_MODULE,               /* module type */
    NULL,                          /* init master */
    NULL,                          /* init module */
    NULL,                          /* init process */
    NULL,                          /* init thread */
    NULL,                          /* exit thread */
    NULL,                          /* exit process */
    NULL,                          /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;


static char *
ngx_http_fwmark_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    if (ngx_need_capability(cf, CAP_NET_ADMIN) != NGX_CONF_OK) {
        return NGX_CONF_ERROR;
    }

    return ngx_http_set_complex_value_slot(cf, cmd, conf);
}


static ngx_int_t
ngx_http_fwmark_header_filter(ngx_http_request_t *r)
{
    ngx_str_t                    fwmark_str;
    int64_t                      fwmark_int;
    uint32_t                     fwmark_u32;
    ngx_http_fwmark_loc_conf_t  *conf;

    if (r != r->main) {
        return ngx_http_next_header_filter(r);
    }

    conf = ngx_http_get_module_loc_conf(r, ngx_http_fwmark_module);

    if (!conf->fwmark) {
        return ngx_http_next_header_filter(r);
    }

    if (ngx_http_complex_value(r, conf->fwmark, &fwmark_str) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "fwmark: evaluation failed");
        return ngx_http_next_header_filter(r);
    }

    if (fwmark_str.len == 0) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "fwmark: not specified");
        return ngx_http_next_header_filter(r);
    }

    if (fwmark_str.len == 3 && ngx_strncmp(fwmark_str.data, "off", 3) == 0) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "fwmark: off");
        return ngx_http_next_header_filter(r);
    }

    if (fwmark_str.len > 10) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                  "fwmark: invalid value length '%V'", &fwmark_str);
        return ngx_http_next_header_filter(r);
    }

    if (fwmark_str.data[0] == '0' && fwmark_str.len >= 2) {
        if ((fwmark_str.data[1] != 'x' && fwmark_str.data[1] != 'X')
                || fwmark_str.len < 3)
        {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                      "fwmark: invalid hex base prefix '%V'", &fwmark_str);
            return ngx_http_next_header_filter(r);
        }
        fwmark_int = ngx_hextoi64(fwmark_str.data + 2, fwmark_str.len - 2);
    } else {
        fwmark_int = ngx_atoi64(fwmark_str.data, fwmark_str.len);
    }

    if (fwmark_int == NGX_ERROR || fwmark_int < 0 || fwmark_int > UINT32_MAX) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                  "fwmark: not a legal u32 value '%V'", &fwmark_str);
        return ngx_http_next_header_filter(r);
    }

    fwmark_u32 = fwmark_int;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "fwmark: using %uD (0x%08XD)", fwmark_u32, fwmark_u32);

    if (setsockopt(r->connection->fd, SOL_SOCKET, SO_MARK,
            (const void *) &fwmark_u32, sizeof(fwmark_u32)) == -1)
    {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, ngx_socket_errno,
                      "fwmark: setsockopt(SO_MARK) failed");
    }

    return ngx_http_next_header_filter(r);
}


static void *
ngx_http_fwmark_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_fwmark_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_fwmark_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->fwmark = NULL;
     */

    return conf;
}


static char *
ngx_http_fwmark_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    (void)cf;

    ngx_http_fwmark_loc_conf_t *prev = parent;
    ngx_http_fwmark_loc_conf_t *conf = child;

    if (conf->fwmark == NULL) {
        conf->fwmark = prev->fwmark;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_fwmark_init(ngx_conf_t *cf)
{
    (void)cf;

    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_fwmark_header_filter;

    return NGX_OK;
}
