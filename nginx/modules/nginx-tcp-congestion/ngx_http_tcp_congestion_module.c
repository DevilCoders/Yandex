#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    ngx_flag_t               enable; // 1 - fixed value, 2 - variable
    ngx_str_t                tcp_congestion;
    ngx_http_complex_value_t tcp_congestion_var;
} ngx_http_tcp_congestion_conf_t;


static void *ngx_http_tcp_congestion_create_conf(ngx_conf_t *cf);
static char *ngx_http_tcp_congestion_merge_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_tcp_congestion_add_variable(ngx_conf_t *cf, ngx_command_t *dummy, void *conf);
static char *ngx_http_tcp_congestion(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_tcp_congestion_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_tcp_congestion_commands[] = {

    { ngx_string("tcp_congestion"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_http_tcp_congestion,
      0,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_tcp_congestion_module_ctx = {
    NULL,                          /* preconfiguration */
    ngx_http_tcp_congestion_init,          /* postconfiguration */

    NULL,                          /* create main configuration */
    NULL,                          /* init main configuration */

    ngx_http_tcp_congestion_create_conf,   /* create server configuration */
    ngx_http_tcp_congestion_merge_conf,    /* merge server configuration */

    NULL,                          /* create location configuration */
    NULL,                          /* merge location configuration */
};


ngx_module_t  ngx_http_tcp_congestion_module = {
    NGX_MODULE_V1,
    &ngx_http_tcp_congestion_module_ctx,   /* module context */
    ngx_http_tcp_congestion_commands,      /* module directives */
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


static ngx_int_t
ngx_http_tcp_congestion_handler(ngx_http_request_t *r)
{
    ngx_http_tcp_congestion_conf_t  *conf;
    ngx_str_t                        val;
    if (r != r->main || !r->connection) {
        return NGX_DECLINED;
    }


    conf = ngx_http_get_module_srv_conf(r, ngx_http_tcp_congestion_module);

    if (!conf->enable) {
        return NGX_DECLINED;
    }

    if (conf->enable == 1) {
        val = conf->tcp_congestion;
    } else {
        ngx_str_null(&val);
        if (conf->tcp_congestion_var.value.len > 0) {
            if (ngx_http_complex_value(r, &conf->tcp_congestion_var, &val) != NGX_OK) {
                return NGX_DECLINED;
            }
        }

    }

    if (val.len == 0) {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                        "tcp_congestion variable is not parsed");
        return NGX_DECLINED;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "tcp_congestion: %s", val.data);

    if (setsockopt(r->connection->fd, IPPROTO_TCP, TCP_CONGESTION,
                   (const void *) val.data, val.len) == -1)
    {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, ngx_socket_errno,
                      "setsockopt(TCP_CONGESTION) failed");
    }

    return NGX_DECLINED;
}


static void *
ngx_http_tcp_congestion_create_conf(ngx_conf_t *cf)
{
    ngx_http_tcp_congestion_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tcp_congestion_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->enable = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_tcp_congestion_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    (void)cf;

    ngx_http_tcp_congestion_conf_t *prev = parent;
    ngx_http_tcp_congestion_conf_t *conf = child;

    ngx_conf_merge_value(conf->enable, prev->enable, 0);

    if (conf->tcp_congestion.len == 0) {
        conf->tcp_congestion = prev->tcp_congestion;
    }

    if (conf->tcp_congestion_var.value.data == NULL) {
        conf->tcp_congestion_var = prev->tcp_congestion_var;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_tcp_congestion(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_tcp_congestion_conf_t  *tcf = conf;

    ngx_str_t  *value;

    if (tcf->enable != NGX_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (value[1].data[0] == '$') {
        return ngx_http_tcp_congestion_add_variable(cf, cmd, conf);
    }

    tcf->enable = 1;
    tcf->tcp_congestion = value[1];

    return NGX_CONF_OK;
}

static char *
ngx_http_tcp_congestion_add_variable(ngx_conf_t *cf, ngx_command_t *dummy, void *conf)
{
    ngx_http_tcp_congestion_conf_t  *tcf = conf;
    ngx_str_t               *value;
    ngx_http_compile_complex_value_t   ccv;

    (void)dummy;

    value = cf->args->elts;

    if (value[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &tcf->tcp_congestion_var;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }
    tcf->enable = 2;
    return NGX_CONF_OK;

}

static ngx_int_t
ngx_http_tcp_congestion_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_POST_READ_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_tcp_congestion_handler;

    return NGX_OK;
}
