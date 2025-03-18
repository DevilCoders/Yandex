#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_variables.h>
#include <ngx_string.h>
#include <ngx_md5.h>

#define     MD5_BHASH_LEN   16

#define     NGX_HTTP_REQUEST_ID_HEADER_NAME "x-request-id"


typedef struct {
    ngx_flag_t                  enabled;
    ngx_flag_t                  enabled_header;
    ngx_str_t                   header_name;
    ngx_uint_t                  length;
    ngx_http_complex_value_t   *prefix;
} ngx_http_request_id_module_loc_conf_t;

typedef struct {
    ngx_str_t                   request_id;
} ngx_http_request_id_ctx_t;


static ngx_str_t ngx_http_request_id_variable_name = ngx_string("request_id");
static ngx_str_t ngx_http_request_id_raw_variable_name = ngx_string("request_id_raw");
static ngx_int_t ngx_http_request_id_add_variable(ngx_conf_t *);
static ngx_int_t ngx_http_request_id_get_variable(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);


static char *ngx_http_request_id_merge_loc_conf(ngx_conf_t *,
        void *, void *);
static void *ngx_http_request_id_create_loc_conf(ngx_conf_t *);


static ngx_conf_num_bounds_t  ngx_http_request_id_length_bounds = {
    ngx_conf_check_num_bounds, 1, 32
};

static ngx_http_module_t ngx_http_request_id_module_ctx = {
    ngx_http_request_id_add_variable,       /* preconfiguration */
    NULL,                                   /* postconfiguration */

    NULL,                                   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    ngx_http_request_id_create_loc_conf,    /* create location configuration */
    ngx_http_request_id_merge_loc_conf      /* merge location configuration */
};

static ngx_command_t ngx_http_request_id_module_commands[] = {
    {
        ngx_string("request_id"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_request_id_module_loc_conf_t, enabled),
        NULL
    },
    {
        ngx_string("request_id_from_header"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF|NGX_CONF_FLAG,
        ngx_conf_set_flag_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_request_id_module_loc_conf_t, enabled_header),
        NULL
    },
    {
        ngx_string("request_id_header_name"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_str_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_request_id_module_loc_conf_t, header_name),
        NULL
    },
    {
        ngx_string("request_id_length"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_conf_set_num_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_request_id_module_loc_conf_t, length),
        &ngx_http_request_id_length_bounds
    },
    {
        ngx_string("request_id_prefix"),
        NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
        ngx_http_set_complex_value_slot,
        NGX_HTTP_LOC_CONF_OFFSET,
        offsetof(ngx_http_request_id_module_loc_conf_t, prefix),
        NULL
    },
    ngx_null_command
};

ngx_module_t ngx_http_request_id_module = {
    NGX_MODULE_V1,
    &ngx_http_request_id_module_ctx,        /* module context */
    ngx_http_request_id_module_commands,    /* module directives */
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
ngx_http_request_id_add_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t *var = ngx_http_add_variable(cf,
            &ngx_http_request_id_variable_name, NGX_HTTP_VAR_NOHASH);

    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_request_id_get_variable;
    var->data = 0;

    ngx_http_variable_t *raw_var = ngx_http_add_variable(cf,
            &ngx_http_request_id_raw_variable_name, NGX_HTTP_VAR_NOHASH);

    if (raw_var == NULL) {
        return NGX_ERROR;
    }

    raw_var->get_handler = ngx_http_request_id_get_variable;
    raw_var->data = 1;

    return NGX_OK;
}

ngx_int_t
ngx_http_request_id_get_variable(ngx_http_request_t *r,
        ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_request_id_module_loc_conf_t   *loc_conf =
            ngx_http_get_module_loc_conf(r, ngx_http_request_id_module);

    if (loc_conf->enabled == 0) {
        v->not_found = 1;

        return NGX_OK;
    }

    ngx_flag_t  is_in_header = 0;

    if (loc_conf->enabled_header) {
        ngx_str_t        header_name = loc_conf->header_name;
        ngx_list_part_t *headers_in_list_part = &r->headers_in.headers.part;
        ngx_table_elt_t *headers_in_list_part_data = headers_in_list_part->elts;
        ngx_uint_t       i = 0;

        for (;;i++) {
            if (i >= headers_in_list_part->nelts) {
                if (headers_in_list_part->next == NULL) {
                    break;
                }

                headers_in_list_part = headers_in_list_part->next;
                headers_in_list_part_data = headers_in_list_part->elts;
                i = 0;
            }

            if (
                    headers_in_list_part_data[i].key.len == header_name.len &&
                    !ngx_strncasecmp(
                            headers_in_list_part_data[i].lowcase_key,
                            header_name.data,
                            header_name.len)
            ) {
                v->len = headers_in_list_part_data[i].value.len;
                v->data = headers_in_list_part_data[i].value.data;

                is_in_header = 1;
                break;
            }
        }
    }

    if (!is_in_header) {
        ngx_str_t   prefix;
        u_char      val[NGX_TIME_T_LEN + 3 + NGX_INT32_LEN + NGX_ATOMIC_T_LEN +
                        NGX_INT32_LEN];
        u_char     *p;
        ngx_uint_t  len, i;
        ngx_md5_t   md5;
        ngx_http_request_id_ctx_t  *ctx;

        ctx = ngx_http_get_module_ctx(r, ngx_http_request_id_module);
        if (ctx == NULL) {
            ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_request_id_ctx_t));
            if (ctx == NULL) {
                return NGX_ERROR;
            }

            ngx_http_set_ctx(r, ctx, ngx_http_request_id_module);
        }

        /* Store generated value in request context */
        if (ctx->request_id.len == 0) {
            ctx->request_id.len = 2 * MD5_BHASH_LEN;

            ctx->request_id.data = ngx_pnalloc(r->pool, ctx->request_id.len);
            if (ctx->request_id.data == NULL) {
                return NGX_ERROR;
            }

            len = ngx_sprintf(val,
                    "%T%M%P%uA%i",
                    r->start_sec,
                    r->start_msec,
                    ngx_pid,
                    r->connection->number,
                    ngx_random()) - val;

            ngx_md5_init(&md5);
            ngx_md5_update(&md5, val, len);
            ngx_md5_final(val, &md5);

            static u_char   hex[] = "0123456789abcdef";

            for (i = 0; i < MD5_BHASH_LEN; ++i) {
                ctx->request_id.data[2 * i] = hex[val[i] >> 4];
                ctx->request_id.data[2 * i + 1] = hex[val[i] & 0xf];
            }
        }

        prefix.len = 0;
        if (data == 0 && loc_conf->prefix) {
            if (ngx_http_complex_value(r, loc_conf->prefix, &prefix) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                              "request_id: unable to get prefix");
            }
            ngx_log_debug1(NGX_LOG_DEBUG, r->connection->log, 0,
                            "request_id: calculated prefix: %V", &prefix);
        }

        v->len = prefix.len + loc_conf->length;

        v->data = ngx_pnalloc(r->pool, v->len);
        if (v->data == NULL) {
            return NGX_ERROR;
        }

        p = v->data;
        if (prefix.len) {
            ngx_memcpy(p, prefix.data, prefix.len);
            p += prefix.len;
        }

        ngx_memcpy(p, ctx->request_id.data, loc_conf->length);
    }

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
};

static char *
ngx_http_request_id_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_request_id_module_loc_conf_t   *prev = parent;
    ngx_http_request_id_module_loc_conf_t   *conf = child;

    ngx_conf_merge_value(conf->enabled, prev->enabled, 1);
    ngx_conf_merge_value(conf->enabled_header, prev->enabled_header, 0);

    ngx_conf_merge_str_value(conf->header_name, prev->header_name,
            NGX_HTTP_REQUEST_ID_HEADER_NAME);

    ngx_conf_merge_uint_value(conf->length, prev->length, 32);

    if (conf->prefix == NULL) {
        conf->prefix = prev->prefix;
    }

    return NGX_CONF_OK;
}

static void *
ngx_http_request_id_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_request_id_module_loc_conf_t   *conf =
            ngx_pcalloc(cf->pool, sizeof(ngx_http_request_id_module_loc_conf_t));

    if (conf == NULL) {
        return NULL;
    }

    conf->enabled = NGX_CONF_UNSET;
    conf->enabled_header = NGX_CONF_UNSET;
    conf->length = NGX_CONF_UNSET_UINT;

    return conf;
}
