#include "ngx_http_geobase_module.h"


typedef struct {
    ngx_str_t    *name;
    uintptr_t     data;
} ngx_http_geobase_var_t;


typedef ngx_int_t (*ngx_get_geobase_data_func)(ngx_http_request_t *r,
    ngx_http_geobase_conf_t *conf, ngx_str_t ip,
    uintptr_t data, ngx_http_variable_value_t *val);


static ngx_int_t ngx_http_geobase_info_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t ngx_http_geobase_trait_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static ngx_int_t
ngx_http_geobase_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data,
    ngx_get_geobase_data_func data_get_func);

static ngx_int_t ngx_http_geobase_add_variables(ngx_conf_t *cf);
static void *ngx_http_geobase_create_conf(ngx_conf_t *cf);
static char *ngx_http_geobase_init_conf(ngx_conf_t *cf, void *conf);
static char *ngx_http_geobase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char *ngx_http_geobase_proxy(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static ngx_int_t ngx_http_geobase_cidr_value(ngx_conf_t *cf, ngx_str_t *net,
    ngx_cidr_t *cidr);


static ngx_command_t  ngx_http_geobase_commands[] = {

    { ngx_string("geobase"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_geobase,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("geobase_proxy"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_TAKE1,
      ngx_http_geobase_proxy,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("geobase_proxy_recursive"),
      NGX_HTTP_MAIN_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_MAIN_CONF_OFFSET,
      offsetof(ngx_http_geobase_conf_t, proxy_recursive),
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_geobase_module_ctx = {
    ngx_http_geobase_add_variables,    /* preconfiguration */
    NULL,                              /* postconfiguration */

    ngx_http_geobase_create_conf,      /* create main configuration */
    ngx_http_geobase_init_conf,        /* init main configuration */

    NULL,                              /* create server configuration */
    NULL,                              /* merge server configuration */

    NULL,                              /* create location configuration */
    NULL                               /* merge location configuration */
};


ngx_module_t  ngx_http_geobase_module = {
    NGX_MODULE_V1,
    &ngx_http_geobase_module_ctx,      /* module context */
    ngx_http_geobase_commands,         /* module directives */
    NGX_HTTP_MODULE,                   /* module type */
    NULL,                              /* init master */
    NULL,                              /* init module */
    NULL,                              /* init process */
    NULL,                              /* init thread */
    NULL,                              /* exit thread */
    NULL,                              /* exit process */
    NULL,                              /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_variable_t  ngx_http_geobase_vars[] = {

    { ngx_string("geobase_country"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_COUNTRY, 0, 0 },

    { ngx_string("geobase_country_name"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_COUNTRY_NAME, 0, 0 },

    { ngx_string("geobase_country_part"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_COUNTRY_PART, 0, 0 },

    { ngx_string("geobase_country_part_name"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_COUNTRY_PART_NAME, 0, 0 },

    { ngx_string("geobase_city"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_CITY, 0, 0 },

    { ngx_string("geobase_city_name"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_CITY_NAME, 0, 0 },

    { ngx_string("geobase_latitude"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_LATITUDE, 0, 0 },

    { ngx_string("geobase_longitude"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_LONGITUDE, 0, 0 },

    { ngx_string("geobase_path"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_PATH, 0, 0 },

    { ngx_string("geobase_path_name"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_PATH_NAME, 0, 0 },

    { ngx_string("geobase_region_id"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_REGION_ID, 0, 0 },

    { ngx_string("geobase_parents_ids"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_PARENTS_IDS, 0, 0 },

    { ngx_string("geobase_children_ids"), NULL,
          ngx_http_geobase_info_variable,
          GEOBASE_CHILDREN_IDS, 0, 0 },

    { ngx_string("geobase_is_stub"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_STUB, 0, 0 },

    { ngx_string("geobase_is_reserved"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_RESERVED, 0, 0 },

    { ngx_string("geobase_is_yandex_net"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_YANDEX_NET, 0, 0 },

    { ngx_string("geobase_is_yandex_staff"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_YANDEX_STAFF, 0, 0 },

    { ngx_string("geobase_is_yandex_turbo"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_YANDEX_TURBO, 0, 0 },

    { ngx_string("geobase_is_tor"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_TOR, 0, 0 },

    { ngx_string("geobase_is_proxy"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_PROXY, 0, 0 },

    { ngx_string("geobase_is_vpn"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_VPN, 0, 0 },

    { ngx_string("geobase_is_hosting"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_HOSTING, 0, 0 },

    { ngx_string("geobase_is_mobile"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_IS_MOBILE, 0, 0 },

    { ngx_string("geobase_isp_name"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_ISP_NAME, 0, 0 },

    { ngx_string("geobase_org_name"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_ORG_NAME, 0, 0 },

    { ngx_string("geobase_asn_list"), NULL,
          ngx_http_geobase_trait_variable,
          GEOBASE_TRAIT_ASN_LIST, 0, 0 },

    { ngx_null_string, NULL, NULL, 0, 0, 0 }
};


static ngx_int_t
ngx_http_geobase_info_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_geobase_variable(r, v, data, &ngx_get_geobase_info);
}


static ngx_int_t
ngx_http_geobase_trait_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    return ngx_http_geobase_variable(r, v, data, &ngx_get_geobase_trait);
}

static ngx_int_t
ngx_http_geobase_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data,
    ngx_get_geobase_data_func data_get_func)
{
    ngx_addr_t                addr;
    ngx_array_t              *xfwd;
    ngx_http_geobase_conf_t  *gcf;
    u_char                    addr_buff[NGX_INET6_ADDRSTRLEN];
    ngx_str_t                 addr_ngx = {sizeof(addr_buff), addr_buff};

    gcf = ngx_http_get_module_main_conf(r, ngx_http_geobase_module);

    addr.sockaddr = r->connection->sockaddr;
    addr.socklen = r->connection->socklen;

    xfwd = &r->headers_in.x_forwarded_for;

    if (xfwd->nelts > 0 && gcf->proxies != NULL) {
        (void) ngx_http_get_forwarded_addr(r, &addr, xfwd, NULL,
                                           gcf->proxies, gcf->proxy_recursive);
    }

    addr_ngx.len = ngx_sock_ntop(addr.sockaddr, addr.socklen, addr_buff, addr_ngx.len , 0);

    return (*data_get_func)(r, gcf, addr_ngx, data, v);
}


static ngx_int_t
ngx_http_geobase_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var, *v;

    for (v = ngx_http_geobase_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}


static void *
ngx_http_geobase_create_conf(ngx_conf_t *cf)
{
    ngx_pool_cleanup_t       *cln;
    ngx_http_geobase_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_geobase_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->proxy_recursive = NGX_CONF_UNSET;

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NULL;
    }

    cln->handler = ngx_http_geobase_cleanup;
    cln->data = conf;

    return conf;
}


static char *
ngx_http_geobase_init_conf(ngx_conf_t *cf, void *conf)
{
    ngx_http_geobase_conf_t  *gcf = conf;

    ngx_conf_init_value(gcf->proxy_recursive, 0);
    ngx_conf_init_ptr_value(gcf->lookUp, 0);

    return NGX_CONF_OK;
}


static char *
ngx_http_geobase(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_geobase_conf_t  *gcf = conf;
    ngx_str_t  *value;

    value = cf->args->elts;

    return ngx_http_geobase_init(cf, gcf, &value[1]);
}


static char *
ngx_http_geobase_proxy(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_geobase_conf_t  *gcf = conf;

    ngx_str_t   *value;
    ngx_cidr_t  cidr, *c;

    value = cf->args->elts;

    if (ngx_http_geobase_cidr_value(cf, &value[1], &cidr) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    if (gcf->proxies == NULL) {
        gcf->proxies = ngx_array_create(cf->pool, 4, sizeof(ngx_cidr_t));
        if (gcf->proxies == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    c = ngx_array_push(gcf->proxies);
    if (c == NULL) {
        return NGX_CONF_ERROR;
    }

    *c = cidr;

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_geobase_cidr_value(ngx_conf_t *cf, ngx_str_t *net, ngx_cidr_t *cidr)
{
    ngx_int_t  rc;

    if (ngx_strcmp(net->data, "255.255.255.255") == 0) {
        cidr->family = AF_INET;
        cidr->u.in.addr = 0xffffffff;
        cidr->u.in.mask = 0xffffffff;

        return NGX_OK;
    }

    rc = ngx_ptocidr(net, cidr);

    if (rc == NGX_ERROR) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid network \"%V\"", net);
        return NGX_ERROR;
    }

    if (rc == NGX_DONE) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "low address bits of %V are meaningless", net);
    }

    return NGX_OK;
}
