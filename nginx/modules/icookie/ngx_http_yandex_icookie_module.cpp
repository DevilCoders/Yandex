extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
}

#include <nginx/modules/strm_packager/src/base/pool_util.h>
#include <yweb/webdaemons/icookiedaemon/icookie_lib/keys.h>
#include <yweb/webdaemons/icookiedaemon/icookie_lib/process.h>

using NStrm::NPackager::TNgxPoolUtil;

typedef struct {
    NIcookie::TIcookieEncrypter* encrypter;
} ngx_http_yandex_icookie_conf_t;

static ngx_int_t ngx_http_yandex_icookie_add_vars(ngx_conf_t* cf);
static void* ngx_http_yandex_icookie_create_conf(ngx_conf_t* cf);
static char* ngx_http_yandex_icookie_init_conf(ngx_conf_t* cf, void* conf);
static ngx_int_t ngx_http_yandex_icookie_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data);

static ngx_http_module_t ngx_http_yandex_icookie_module_ctx{
    ngx_http_yandex_icookie_add_vars, /* preconfiguration */
    NULL,                             /* postconfiguration */

    ngx_http_yandex_icookie_create_conf, /* create main configuration */
    ngx_http_yandex_icookie_init_conf,   /* init main configuration */

    NULL, /* create server configuration */
    NULL, /* merge server configuration */

    NULL, /* create location configuration */
    NULL, /* merge location configuration */
};

extern "C" ngx_module_t ngx_http_yandex_icookie_module = {
    NGX_MODULE_V1,
    &ngx_http_yandex_icookie_module_ctx, // module context
    NULL,                                // module directives
    NGX_HTTP_MODULE,                     // module type
    NULL,                                // init master
    NULL,                                // init module
    NULL,                                // init process
    NULL,                                // init thread
    NULL,                                // exit thread
    NULL,                                // exit process
    NULL,                                // exit master
    NGX_MODULE_V1_PADDING};

static ngx_http_variable_t ngx_http_yandex_icookie_vars[] = {
    {ngx_string("yandex_icookie"), NULL, ngx_http_yandex_icookie_variable, 0, 0, 0},
    ngx_http_null_variable};

static ngx_int_t ngx_http_yandex_icookie_add_vars(ngx_conf_t* cf) {
    ngx_http_variable_t *var, *v;

    for (v = ngx_http_yandex_icookie_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

static ngx_str_t icookie_name = ngx_string("i");
static ngx_str_t ngx_str_zero = ngx_string("0");

static ngx_int_t ngx_http_yandex_icookie_variable(ngx_http_request_t* r, ngx_http_variable_value_t* v, uintptr_t data) {
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    ngx_str_t cookie;
    if (ngx_http_parse_multi_header_lines(&r->headers_in.cookies, &icookie_name, &cookie) == NGX_DECLINED)
    {
        v->len = ngx_str_zero.len;
        v->data = ngx_str_zero.data;
        return NGX_OK;
    }

    ngx_str_t value;
    ngx_http_yandex_icookie_conf_t* icf = (ngx_http_yandex_icookie_conf_t*)ngx_http_get_module_main_conf(r, ngx_http_yandex_icookie_module);

    try {
        auto decrypted = NIcookie::DecryptIcookie(TStringBuf((char const*)cookie.data, cookie.len), icf->encrypter);

        if (decrypted) {
            TString* inPool = TNgxPoolUtil<TString>(r->pool).New(std::move(*decrypted));

            value.len = inPool->length();
            value.data = (unsigned char*)inPool->Data();
        } else {
            value = ngx_str_zero;
        }

        v->len = value.len;
        v->data = value.data;

        return NGX_OK;
    } catch (const std::exception& e) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "c++ exception [[ %s ]] ", e.what());

        v->len = ngx_str_zero.len;
        v->data = ngx_str_zero.data;
        return NGX_OK;
    } catch (...) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "c++ unknown exception");

        v->len = ngx_str_zero.len;
        v->data = ngx_str_zero.data;
        return NGX_OK;
    }
}

static void* ngx_http_yandex_icookie_create_conf(ngx_conf_t* cf) {
    ngx_http_yandex_icookie_conf_t* conf;

    conf = (ngx_http_yandex_icookie_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_yandex_icookie_conf_t));

    return conf;
}

static char* ngx_http_yandex_icookie_init_conf(ngx_conf_t* cf, void* conf) {
    ngx_http_yandex_icookie_conf_t* icf = (ngx_http_yandex_icookie_conf_t*)conf;

    try {
        icf->encrypter = new NIcookie::TIcookieEncrypter(NIcookie::GetDefaultYandexKeys());
    } catch (std::exception& e) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unable to initialize icookie encryptor, c++ exception [[ %s ]]", e.what());

        return (char*)NGX_CONF_ERROR;
    } catch (...) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "unable to initialize icookie encryptor, c++ unknown exception");

        return (char*)NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}
