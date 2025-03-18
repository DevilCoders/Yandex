#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_http_variables.h>
#include <ngx_string.h>


typedef struct {
    ngx_int_t               cookie_index;
} ngx_http_cookie_obf_module_main_conf_t;

typedef struct {
    ngx_array_t            *obf_headers;
} ngx_http_cookie_obf_module_loc_conf_t;


typedef struct {
    ngx_variable_value_t    orig_cookie;
} ngx_http_cookie_obf_ctx_t;


static ngx_str_t ngx_http_cookie_variable_name = ngx_string("http_cookie");
static ngx_str_t ngx_http_full_cookie_variable_name = ngx_string("full_http_cookie");
static ngx_int_t ngx_http_cookie_obf_add_variable(ngx_conf_t *);
static ngx_int_t ngx_http_cookie_obf_get_variable(ngx_http_request_t *,
        ngx_http_variable_value_t *, uintptr_t);


static ngx_int_t ngx_http_cookie_obf_init(ngx_conf_t *cf);
static void *ngx_http_cookie_obf_create_main_conf(ngx_conf_t *);
static char *ngx_http_cookie_obf_merge_loc_conf(ngx_conf_t *,
        void *, void *);
static void *ngx_http_cookie_obf_create_loc_conf(ngx_conf_t *);



static ngx_command_t ngx_http_cookie_obf_module_commands[] = {
    { ngx_string("obf_header"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_cookie_obf_module_loc_conf_t, obf_headers),
      NULL },
    ngx_null_command
};

static ngx_http_module_t ngx_http_cookie_obf_module_ctx = {
    ngx_http_cookie_obf_add_variable,       /* preconfiguration */
    ngx_http_cookie_obf_init,               /* postconfiguration */

    ngx_http_cookie_obf_create_main_conf,   /* create main configuration */
    NULL,                                   /* init main configuration */

    NULL,                                   /* create server configuration */
    NULL,                                   /* merge server configuration */

    ngx_http_cookie_obf_create_loc_conf,    /* create location configuration */
    ngx_http_cookie_obf_merge_loc_conf      /* merge location configuration */
};

ngx_module_t ngx_http_cookie_obf_module = {
    NGX_MODULE_V1,
    &ngx_http_cookie_obf_module_ctx,        /* module context */
    ngx_http_cookie_obf_module_commands,    /* module directives */
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

ngx_str_t   cookies_to_obf[] = {
    ngx_string("session_id"),
    ngx_string("sessionid2"),
    ngx_string("secure_session_id"),
    ngx_string("golem_session"),
    ngx_string("eda1"),
    ngx_string("eda2"),
    ngx_string("ya_sess_id"),
    ngx_string("sessguard"),
    ngx_string("yandextoken"),
    ngx_null_string
};

ngx_str_t   cookies_to_mask[] = {
    ngx_string("webviewtoken"),
    ngx_string("webviewdriversession"),
    ngx_string("webviewbearertoken"),
    ngx_string("PHPSESSID"),
    ngx_string("token"),
    ngx_string("oauth_token"),
    ngx_string("webviewtoken_mlutp"),
    ngx_null_string
};

const u_char obf_char = 'X';


ngx_int_t
ngx_http_cookie_obf_save_variable(ngx_http_request_t *r)
{
    ngx_http_cookie_obf_ctx_t               *ctx;
    ngx_http_cookie_obf_module_main_conf_t  *main_conf =
            ngx_http_get_module_main_conf(r, ngx_http_cookie_obf_module);
    ngx_http_variable_value_t               *value;

    ctx = ngx_http_get_module_ctx(r, ngx_http_cookie_obf_module);
    if (ctx == NULL) {
        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_cookie_obf_ctx_t));
        if (ctx == NULL) {
            return NGX_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_cookie_obf_module);
    }

    value = ngx_http_get_indexed_variable(r, main_conf->cookie_index);
    if (value == NULL) {
        ctx->orig_cookie.valid = 1;
        ctx->orig_cookie.not_found = 1;
        return NGX_OK;
    }

    if (value->len) {
        ctx->orig_cookie.data = ngx_pcalloc(r->pool, value->len);
        if (ctx->orig_cookie.data == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(ctx->orig_cookie.data, value->data, value->len);
    }

    ctx->orig_cookie.len = value->len;
    ctx->orig_cookie.valid = value->valid;
    ctx->orig_cookie.no_cacheable = value->no_cacheable;
    ctx->orig_cookie.not_found = value->not_found;
    ctx->orig_cookie.escape = value->escape;

    return NGX_OK;
}


static ngx_int_t
ngx_http_cookie_obf_add_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t *var = ngx_http_add_variable(cf,
            &ngx_http_full_cookie_variable_name, NGX_HTTP_VAR_NOHASH);

    if (var == NULL) {
        return NGX_ERROR;
    }

    var->get_handler = ngx_http_cookie_obf_get_variable;

    return NGX_OK;
}


ngx_int_t
ngx_http_cookie_obf_get_variable(ngx_http_request_t *r,
        ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_int_t                                rc;
    ngx_http_cookie_obf_ctx_t              *ctx;

    ctx = ngx_http_get_module_ctx(r, ngx_http_cookie_obf_module);
    if (ctx == NULL || !ctx->orig_cookie.valid) {
        rc = ngx_http_cookie_obf_save_variable(r);
        if (rc != NGX_OK) {
            return rc;
        }
    }

    *v = ctx->orig_cookie;

    return NGX_OK;
}


static u_char *
ngx_http_obf_string(u_char *start, u_char *end)
{
    u_char  *dot_pos = NULL;
    u_char  *last;

    for (last = start; last < end && *last != ';'; last++) {
        if(*last == '.') {
            dot_pos = last;
        }
    }

    if (dot_pos != NULL) {
        for (dot_pos++; dot_pos < last; dot_pos++) {
            *dot_pos = obf_char;
        }
    }

    return last;
}


static u_char *
ngx_http_mask_string(u_char *start, u_char *end)
{
    u_char  *last;

    for (last = start; last < end && *last != ';'; last++) {
        *last = obf_char;
    }

    return last;
}


static void
ngx_http_obf_cookie(ngx_http_request_t *r,
                    ngx_http_variable_value_t *value)
{
    ngx_str_t  *name;
    int         flag_to_mask_cookies;
    u_char      ch;
    u_char     *start = value->data;
    u_char     *end = start + value->len;
    u_char     *equal_sign;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "check obfuscator cookie string %v", value);
    while (start < end) {

        flag_to_mask_cookies = 0;

        equal_sign = ngx_strlchr(start, end, '=');
        if (!equal_sign) {
            goto skip;
        }

        for (name = cookies_to_obf; name->data != NULL; name++) {
            if (ngx_strlcasestrn(start, equal_sign, name->data, name->len - 1)) {
                break;
            }
        }

        if (name->data == NULL) {
            for (name = cookies_to_mask; name->data != NULL; name++) {
                if (ngx_strncasecmp(start, name->data, name->len) == 0) {
                    flag_to_mask_cookies = 1;
                    start += name->len;
                    break;
                }
            }

            if (flag_to_mask_cookies == 0) {
                goto skip;
            }
        } else {
            start = equal_sign;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "cookie obfuscator found cookie %V in string %v", name, value);

        if (start == end || *start++ != '=') {
            /* Cookie name should be cookies_to_mask, but instead it just starts with this string. Or the header value is invalid */
            goto skip;
        }

        if (flag_to_mask_cookies == 1) {
            start = ngx_http_mask_string(start, end);
        } else {
            start = ngx_http_obf_string(start, end);
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "obfuscator clear %V in cookie string %v", name, value);

    skip:

        while (start < end) {
            ch = *start++;
            if (ch == ';' || ch == ',') {
                break;
            }
        }

        while (start < end && *start == ' ') { start++; }
    }
}


static void
ngx_http_obf_header(ngx_http_request_t *r, ngx_str_t *name) {
    ngx_list_part_t   *part;
    ngx_table_elt_t   *h;
    ngx_uint_t         i;

    part = &r->headers_in.headers.part;
    h = part->elts;

    for (i = 0; /* void */ ; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL) {
                /* The last part, search is done. */
                break;
            }

            part = part->next;
            h = part->elts;
            i = 0;
        }

        if (name->len == h[i].key.len && ngx_strncasecmp(h[i].key.data, name->data, name->len) == 0) {
            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "find  header obfuscator %V %V", &h[i].key, &h[i].value);
            ngx_http_obf_string(h[i].value.data, h[i].value.data + h[i].value.len);
            ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "clear header obfuscator %V %V", &h[i].key, &h[i].value);
        }
    }
}


static ngx_int_t
ngx_http_cookie_obf_log_handler(ngx_http_request_t *r)
{
    ngx_int_t                                rc;
    ngx_http_cookie_obf_ctx_t               *ctx;
    ngx_http_cookie_obf_module_main_conf_t  *main_conf =
            ngx_http_get_module_main_conf(r, ngx_http_cookie_obf_module);
    ngx_http_cookie_obf_module_loc_conf_t   *loc_conf =
            ngx_http_get_module_loc_conf(r, ngx_http_cookie_obf_module);
    ngx_http_variable_value_t               *value;
    ngx_str_t                               *name;
    ngx_uint_t                               i;

    // obf headers
    if (loc_conf->obf_headers != NULL) {
        name = loc_conf->obf_headers->elts;

        for (i = 0; i < loc_conf->obf_headers->nelts; i++) {
            ngx_http_obf_header(r, &name[i]);
        }
    }

    /* Save old value if it wasn't saved yet */
    ctx = ngx_http_get_module_ctx(r, ngx_http_cookie_obf_module);
    if (ctx == NULL || !ctx->orig_cookie.valid) {
        rc = ngx_http_cookie_obf_save_variable(r);
        if (rc != NGX_OK) {
            return rc;
        }
    }

    value = ngx_http_get_indexed_variable(r, main_conf->cookie_index);
    if (value == NULL || value->not_found) {
        return NGX_OK;
    }

    ngx_http_obf_cookie(r, value);

    return NGX_OK;
}


static ngx_int_t
ngx_http_cookie_obf_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt                    *h;
    ngx_http_core_main_conf_t              *mcf;
    ngx_http_cookie_obf_module_main_conf_t  *cmcf =
            ngx_http_conf_get_module_main_conf(cf, ngx_http_cookie_obf_module);

    cmcf->cookie_index = ngx_http_get_variable_index(cf, &ngx_http_cookie_variable_name);
    if (cmcf->cookie_index == NGX_ERROR) {
        return NGX_ERROR;
    }

    mcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&mcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_cookie_obf_log_handler;

    return NGX_OK;
}


static void *
ngx_http_cookie_obf_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_cookie_obf_module_main_conf_t   *conf =
            ngx_pcalloc(cf->pool, sizeof(ngx_http_cookie_obf_module_main_conf_t));

    if (conf == NULL) {
        return NULL;
    }

    conf->cookie_index = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_cookie_obf_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_cookie_obf_module_loc_conf_t   *prev = parent;
    ngx_http_cookie_obf_module_loc_conf_t   *conf = child;

    ngx_conf_merge_ptr_value(conf->obf_headers, prev->obf_headers, NULL);

    return NGX_CONF_OK;
}


static void *
ngx_http_cookie_obf_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_cookie_obf_module_loc_conf_t   *conf =
            ngx_pcalloc(cf->pool, sizeof(ngx_http_cookie_obf_module_loc_conf_t));

    if (conf == NULL) {
        return NULL;
    }

    conf->obf_headers = NGX_CONF_UNSET_PTR;

    return conf;
}
