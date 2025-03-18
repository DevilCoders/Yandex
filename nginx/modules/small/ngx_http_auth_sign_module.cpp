
/*
 * Copyright (C) Anton Kortunov
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


extern "C" {
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_crypt.h>
#include "utils.h"
}

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/join.h>
#include <util/string/split.h>


static time_t ngx_http_auth_sign_get_timestamp(ngx_http_request_t *r);
static ngx_int_t ngx_http_auth_sign_init(ngx_conf_t *cf);
static void * ngx_http_auth_sign_create_loc_conf(ngx_conf_t *cf);
static char * ngx_http_auth_sign_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static char * ngx_http_auth_sign_opt_set_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static char * ngx_http_auth_sign_decrypt_add_var(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_auth_sign_get_variable(ngx_http_request_t *r,
        ngx_http_variable_value_t *var, uintptr_t data);
static ngx_int_t ngx_http_auth_sign_signature(
    ngx_http_request_t *r, ngx_str_t message,
    ngx_str_t sign);
static ngx_int_t ngx_http_auth_sign_signature_by_token(
    ngx_http_request_t *r, ngx_str_t message, ngx_str_t token,
    ngx_str_t sign, EVP_MD *evp_md, u_char *digest_hex,
    u_char *digest, unsigned int *digest_len);
static ngx_int_t ngx_http_auth_sign_run_slash_mode(ngx_http_request_t *r) noexcept;
static EVP_MD * ngx_http_auth_sign_get_evp_md(ngx_log_t *log, ngx_uint_t hash);
static time_t ngx_http_auth_sign_get_timestamp_from_str(
    ngx_log_t *log, ngx_str_t ts_str);
static ngx_int_t ngx_http_auth_sign_slash_mode_get_var(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static char * ngx_http_auth_sign_slash_mode_add_var(ngx_conf_t *cf,
    ngx_command_t *cmd, void *conf);
static ngx_int_t ngx_http_auth_sign_allocate_string(ngx_http_request_t *r,
    TStringBuf s, ngx_str_t *result_s);
static ngx_int_t ngx_http_auth_sign_validate_expire_ts(ngx_http_request_t *r, TString ts_key);

// SLASH_MODE_PARAM_PREFIX is parameter prefix in slash mode.
const TString SLASH_MODE_PARAM_PREFIX = "ysign";

const TString SLASH_MODE_PREFIX_PARAM_KEY = "pfx";
const TString SLASH_MODE_SUFFIX_PARAM_KEY = "sfx";

const TString SLASH_MODE_TS_PARAM_KEY = "ts";

const auto SLASH_MODE_PARAM_DELIMITER = ',';
const auto SLASH_MODE_KEY_VALUE_DELIMITER = '=';

const ngx_int_t SLASH_MODE_MAX_PARAM_COUNT = 128;

typedef struct {
    ngx_str_t                     name;
    ngx_http_complex_value_t      value;
} ngx_http_auth_sign_optional_t;

typedef struct {
    ngx_array_t                   *tokens;
    ngx_array_t                   *optional;
    ngx_http_complex_value_t      *message;
    ngx_http_complex_value_t      *sign;
    ngx_http_complex_value_t      *expire;
    ngx_uint_t                     unauthorized_status;
    ngx_uint_t                     expired_status;
    ngx_uint_t                     hash;
    ngx_flag_t                     pass_if_empty;
    ngx_flag_t                     slash_mode;
} ngx_http_auth_sign_loc_conf_t;

typedef struct {
    ngx_str_t key;
    ngx_str_t value;
} ngx_http_auth_sign_slash_mode_param_t;

typedef struct {
    ngx_str_t                      token_used;
    ngx_array_t                   *slash_mode_params;
} ngx_http_auth_sign_ctx_t;

typedef enum {
    NGX_HTTP_AUTH_SIGN_MD5,
    NGX_HTTP_AUTH_SIGN_SHA256,
    NGX_HTTP_AUTH_SIGN_SHA512
} hash_type_e;

static ngx_conf_enum_t ngx_http_auth_sign_hash[] = {
    { ngx_string("md5"), NGX_HTTP_AUTH_SIGN_MD5 },
    { ngx_string("sha256"), NGX_HTTP_AUTH_SIGN_SHA256 },
    { ngx_string("sha512"), NGX_HTTP_AUTH_SIGN_SHA512 },
};

static ngx_conf_num_bounds_t  ngx_http_error_status_bounds = {
    ngx_conf_check_num_bounds, 400, 499
};

static ngx_command_t  ngx_http_auth_sign_commands[] = {

    { ngx_string("auth_sign"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, message),
      NULL },

    { ngx_string("auth_sign_signature"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, sign),
      NULL },

    { ngx_string("auth_sign_token"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, tokens),
      NULL },

    { ngx_string("auth_sign_expire"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, expire),
      NULL },

    { ngx_string("auth_sign_hash"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_enum_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, hash),
      &ngx_http_auth_sign_hash },

    { ngx_string("auth_sign_expired_status"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, expired_status),
      &ngx_http_error_status_bounds },

    { ngx_string("auth_sign_unauthorized_status"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, unauthorized_status),
      &ngx_http_error_status_bounds },

    { ngx_string("auth_sign_optional"),
      NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_auth_sign_opt_set_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("auth_sign_decrypt"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_http_auth_sign_decrypt_add_var,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("auth_sign_pass_if_empty"),
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, pass_if_empty),
      NULL },

    { ngx_string("auth_sign_slash_mode"),
      NGX_HTTP_LOC_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_auth_sign_loc_conf_t, slash_mode),
      NULL },

    { ngx_string("auth_sign_slash_mode_add_var"),
      NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_http_auth_sign_slash_mode_add_var,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_auth_sign_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_auth_sign_init,              /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_auth_sign_create_loc_conf,   /* create location configuration */
    ngx_http_auth_sign_merge_loc_conf     /* merge location configuration */
};


ngx_module_t  ngx_http_auth_sign_module = {
    NGX_MODULE_V1,
    &ngx_http_auth_sign_module_ctx,       /* module context */
    ngx_http_auth_sign_commands,          /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_auth_sign_handler(ngx_http_request_t *r)
{
    ngx_uint_t                       i;
    ngx_str_t                        sign;
    ngx_str_t                        message;
    ngx_str_t                        full_message;
    u_char                          *p;
    ngx_str_t                        param;
    time_t                           ts;
    ngx_log_t                       *log;
    ngx_http_auth_sign_optional_t   *opt;
    ngx_http_auth_sign_loc_conf_t   *aslcf;

    log = r->connection->log;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,
            "auth_sign module");

    aslcf = (ngx_http_auth_sign_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_sign_module);

    ngx_http_auth_sign_ctx_t *ctx = (ngx_http_auth_sign_ctx_t *)
        ngx_http_get_module_ctx(r, ngx_http_auth_sign_module);
    if (ctx == NULL) {
        ctx = (ngx_http_auth_sign_ctx_t *)ngx_pcalloc(
            r->pool, sizeof(ngx_http_auth_sign_ctx_t));
        if (ctx == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0,
                        "auth_sign: failed allocate ctx");
            return NGX_ERROR;
        }

        ngx_array_t* slash_mode_params = ngx_array_create(
            r->pool, SLASH_MODE_MAX_PARAM_COUNT, sizeof(ngx_http_auth_sign_slash_mode_param_t));
        ctx->slash_mode_params = slash_mode_params;

        ngx_http_set_ctx(r, ctx, ngx_http_auth_sign_module);
    }

    if (aslcf->tokens == NULL) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,
                "auth_sign tokens list is empty");

        return NGX_DECLINED;
    }

    if (aslcf->slash_mode) {
        return ngx_http_auth_sign_run_slash_mode(r);
    }

    if (ngx_http_complex_value(r, aslcf->sign, &sign) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: unable to get signature");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
            "auth_sign: signature: %V", &sign);

    if (sign.len == 0 && aslcf->pass_if_empty) {
        return NGX_DECLINED;
    }

    if (ngx_http_complex_value(r, aslcf->message, &message) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: unable to get message");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
            "auth_sign: message: %V", &message);

    full_message.len = message.len;

    if (aslcf->optional) {
        opt = (ngx_http_auth_sign_optional_t *)aslcf->optional->elts;

        for (i = 0; i < aslcf->optional->nelts; i++) {
            if (ngx_http_complex_value(r, &opt[i].value, &param) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, log, 0,
                              "auth_sign: unable to get param");
                return NGX_ERROR;
            }

            ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0,
                          "auth_sign: optional param name: %V value: %V",
                          &opt[i].name, &param);

            if (param.len > 0) {
                    full_message.len += 1 + opt[i].name.len + 1 + param.len;
            }
        }

        full_message.data = (u_char *)ngx_palloc(r->pool, full_message.len);
        if (full_message.data == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0,
                          "auth_sign: unable to allocate memory");
            return NGX_ERROR;
        }

        ngx_memcpy(full_message.data, message.data, message.len);
        p = full_message.data + message.len;

        for (i = 0; i < aslcf->optional->nelts; i++) {
            if (ngx_http_complex_value(r, &opt[i].value, &param) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, log, 0,
                              "auth_sign: unable to get param");
                return NGX_ERROR;
            }

            if (param.len > 0) {
                *(p++) = '&';
                ngx_memcpy(p, opt[i].name.data, opt[i].name.len);
                p += opt[i].name.len;

                *(p++) = '=';
                ngx_memcpy(p, param.data, param.len);
                p += param.len;
            }
        }
    } else {
        full_message.data = message.data;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
            "auth_sign: full message: %V", &full_message);

    ts = ngx_http_auth_sign_get_timestamp(r);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                   "auth_sign: timestamp: %d", (int)ts);

    if (ts < ngx_time()) {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                      "auth_sign: expired signature");
        return aslcf->expired_status;
    }

    return ngx_http_auth_sign_signature(r, full_message, sign);
}


static ngx_int_t ngx_http_auth_sign_run_slash_mode(ngx_http_request_t *r) noexcept
try {
    ngx_log_t *log = r->connection->log;
    ngx_http_auth_sign_loc_conf_t *aslcf =
        (ngx_http_auth_sign_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_sign_module);

    TStringBuf uri((const char *)r->uri.data, r->uri.len);
    // validate uri size
    if (uri.size() > 4096) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: uri is too large");
        return aslcf->unauthorized_status;
    }

    TVector<TStringBuf> slash_parts;
    StringSplitter(uri).Split('/').ParseInto(&slash_parts);

    // Prepare sign parts, prefix, suffix and other signature parts
    bool signature_started = false;
    bool signature_stopped = false;
    TVector<TStringBuf> sign_parts;
    THashMap<TString, TString> sign_params;
    TVector<TStringBuf> prefix_parts;
    TVector<TStringBuf> suffix_parts;
    for (size_t i = 0; i < slash_parts.size(); ++i) {
        const auto slash_part = slash_parts[i];

        TVector<TStringBuf> colon_parts;
        StringSplitter(slash_part).Split(SLASH_MODE_PARAM_DELIMITER).ParseInto(&colon_parts);

        if (signature_stopped) {
            suffix_parts.emplace_back(slash_part);
            continue;
        }

        bool is_part_signed = slash_part.StartsWith(SLASH_MODE_PARAM_PREFIX);
        if (!is_part_signed) {
            if (signature_started) {
                signature_stopped = true;
                suffix_parts.emplace_back(slash_part);
                continue;
            }

            prefix_parts.emplace_back(slash_part);
            continue;
        }

        if (!signature_started) {
            signature_started = true;
        }

        sign_parts.emplace_back(slash_part);

        TVector<TStringBuf> equal_parts;
        StringSplitter(colon_parts[0]).Split(SLASH_MODE_KEY_VALUE_DELIMITER).ParseInto(&equal_parts);
        if (equal_parts.size() != 2) {
            ngx_log_error(NGX_LOG_ERR, log, 0,
                          "auth_sign: failed to parse sign name: %s",
                          TString(slash_part).c_str());
            return aslcf->unauthorized_status;
        }
        TString param_name = TString(equal_parts[0]);
        TVector<TStringBuf> keys_names;
        for (size_t i = 1; i < colon_parts.size(); ++i) {
            TVector<TStringBuf> equal_parts;
            StringSplitter(colon_parts[i]).Split(SLASH_MODE_KEY_VALUE_DELIMITER).ParseInto(&equal_parts);
            if (equal_parts.size() == 0) {
                ngx_log_error(NGX_LOG_ERR, log, 0,
                              "auth_sign: wrong equal parts count: %s",
                              TString(colon_parts[i]).c_str());
                return aslcf->unauthorized_status;
            }

            keys_names.emplace_back(equal_parts[0]);
        }
        TString keys = JoinSeq(TString(SLASH_MODE_PARAM_DELIMITER), keys_names);
        sign_params[param_name] = keys;
        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, log, 0,
                       "auth_sign: found param_name=%s keys=%s",
                       param_name.c_str(),
                       keys.c_str());
    }

    if (sign_parts.size() == 0) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: uri doesn't contain signed parts");
        return aslcf->unauthorized_status;
    }

    TString prefix = JoinSeq("/", prefix_parts);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                    "auth_sign: prefix=%s", prefix.c_str());
    TString suffix = JoinSeq("/", suffix_parts);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                    "auth_sign: suffix=%s", suffix.c_str());

    // Make signature and validate it
    THashMap<TString, TString> params;
    params[SLASH_MODE_PREFIX_PARAM_KEY] = prefix;
    params[SLASH_MODE_SUFFIX_PARAM_KEY] = suffix;
    for (const auto& sign_part : sign_parts) {
        TVector<TStringBuf> colon_parts;
        StringSplitter(sign_part).Split(SLASH_MODE_PARAM_DELIMITER).ParseInto(&colon_parts);

        TVector<TStringBuf> sign_message_parts;
        for (size_t i = 1; i < colon_parts.size(); ++i) {
            TVector<TStringBuf> equal_parts;
            StringSplitter(colon_parts[i]).Split(SLASH_MODE_KEY_VALUE_DELIMITER).ParseInto(&equal_parts);

            if (equal_parts.size() == 0 || equal_parts.size() > 2) {
                ngx_log_error(NGX_LOG_ERR, log, 0,
                              "auth_sign: wrong equal parts count: %s",
                              TString(colon_parts[i]).c_str());
                return aslcf->unauthorized_status;
            }

            TString param_name = TString(equal_parts[0]);

            if (equal_parts.size() == 2) {
                if (param_name == SLASH_MODE_TS_PARAM_KEY) {
                    TStringBuf ts = equal_parts[1];
                    auto ts_check_result = ngx_http_auth_sign_validate_expire_ts(r, TString(ts));
                    if (ts_check_result != NGX_OK) {
                        return ts_check_result;
                    }
                    sign_message_parts.emplace_back(ts);
                    continue;
                }

                TStringBuf param = equal_parts[1];
                if (params.FindPtr(param_name) != nullptr) {
                    ngx_log_error(NGX_LOG_ERR, log, 0,
                                  "auth_sign: param with name: %s doubled",
                                  TString(param_name).c_str());
                    return aslcf->unauthorized_status;
                }

                params[param_name] = param;
                sign_message_parts.emplace_back(param);
                continue;
            }

            if (param_name == SLASH_MODE_PREFIX_PARAM_KEY) {
                sign_message_parts.emplace_back(prefix);
                continue;
            }

            if (param_name == SLASH_MODE_SUFFIX_PARAM_KEY) {
                sign_message_parts.emplace_back(suffix);
                continue;
            }

            if (param_name.StartsWith(SLASH_MODE_PARAM_PREFIX)) {
                const auto sign_param_ptr = sign_params.FindPtr(param_name);
                if (sign_param_ptr == nullptr) {
                    ngx_log_error(NGX_LOG_ERR, log, 0,
                                  "auth_sign: there is no param with name: %s",
                                  TString(param_name).c_str());
                    return aslcf->unauthorized_status;
                }

                sign_message_parts.emplace_back(*sign_param_ptr);
                continue;
            }

            ngx_log_error(NGX_LOG_ERR, log, 0,
                          "auth_sign: unknown param_name: %s",
                          param_name.c_str());
            return aslcf->unauthorized_status;
        }

        TString full_message = JoinSeq(TString(SLASH_MODE_PARAM_DELIMITER), sign_message_parts);
        u_char full_message_data[full_message.size()];
        memcpy(full_message_data, full_message.data(), full_message.size());
        ngx_str_t full_message_str = {full_message.size(), full_message_data};
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                        "full_message: %V", &full_message_str);


        TVector<TStringBuf> equal_parts;
        StringSplitter(colon_parts[0]).Split(SLASH_MODE_KEY_VALUE_DELIMITER).ParseInto(&equal_parts);
        if (equal_parts.size() != 2) {
            ngx_log_error(NGX_LOG_ERR, log, 0,
                          "auth_sign: failed to parse sign name: %s",
                          TString(sign_part).c_str());
            return aslcf->unauthorized_status;
        }
        TStringBuf sign_value = equal_parts[1];
        u_char sign_data[sign_value.size()];
        memcpy(sign_data, sign_value.data(), sign_value.size());
        ngx_str_t sign_value_str = {sign_value.size(), sign_data};
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0, "sign: %V", &sign_value_str);

        auto status = ngx_http_auth_sign_signature(r, full_message_str, sign_value_str);
        if (status != NGX_OK) {
            ngx_log_error(NGX_LOG_INFO, log, 0,
                        "auth_sign: signature for param '%s' failed",
                        TString(equal_parts[0]).c_str());
            return status;
        }
    }

    // Save params
    ngx_http_auth_sign_ctx_t *ctx = (ngx_http_auth_sign_ctx_t *)
        ngx_http_get_module_ctx(r, ngx_http_auth_sign_module);
    if (ctx == NULL) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: failed to load context");
        return NGX_ERROR;
    }
    for (auto const& [key, value] : params) {
        ngx_http_auth_sign_slash_mode_param_t *param =
            (ngx_http_auth_sign_slash_mode_param_t *)ngx_array_push(ctx->slash_mode_params);
        if (param == NULL) {
            ngx_log_error(NGX_LOG_ERR, log, 0,
                          "auth_sign: failed to allocate slash mode param");
            return NGX_ERROR;
        }

        if (ngx_http_auth_sign_allocate_string(r, key, &(param->key)) != NGX_OK) {
            return NGX_ERROR;
        }

        if (ngx_http_auth_sign_allocate_string(r, value, &(param->value)) != NGX_OK) {
            return NGX_ERROR;
        }
    }

    return NGX_OK;
} catch (...) {
    ngx_log_t *log = r->connection->log;

    ngx_log_error(NGX_LOG_ERR, log, 0,
                  "auth_sign: slash mode failed with error: %s",
                  CurrentExceptionMessage().c_str());

    return NGX_ERROR;
}


static ngx_int_t ngx_http_auth_sign_validate_expire_ts(
    ngx_http_request_t *r, TString ts_value
) {
    ngx_log_t *log = r->connection->log;
    ngx_http_auth_sign_loc_conf_t *aslcf =
            (ngx_http_auth_sign_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_sign_module);

    u_char ts_data[ts_value.size()];
    memcpy(ts_data, ts_value.data(), ts_value.size());
    ngx_str_t ts_str = {ts_value.size(), ts_data};

    time_t ts = ngx_http_auth_sign_get_timestamp_from_str(log, ts_str);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
            "auth_sign: timestamp: %d", ts);

    if (ts < ngx_time()) {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                "auth_sign: expired signature: ts = %d", ts);
        return aslcf->expired_status;
    }

    return NGX_OK;
}

static ngx_int_t ngx_http_auth_sign_allocate_string(
    ngx_http_request_t *r, TStringBuf s, ngx_str_t *result_s
) {
    result_s->len = s.size();
    result_s->data = (u_char *)ngx_palloc(r->pool, s.size());
    if (result_s->data == NULL) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "auth_sign: failed to allocate memory");
        return NGX_ERROR;
    }

    ngx_memcpy(result_s->data, TString(s).c_str(), result_s->len);
    return NGX_OK;
}


static ngx_int_t ngx_http_auth_sign_signature(
    ngx_http_request_t *r, ngx_str_t message,
    ngx_str_t sign
) {
    ngx_log_t *log = r->connection->log;
    ngx_http_auth_sign_loc_conf_t *aslcf =
        (ngx_http_auth_sign_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_sign_module);

    ngx_http_auth_sign_ctx_t *ctx = (ngx_http_auth_sign_ctx_t *)
        ngx_http_get_module_ctx(r, ngx_http_auth_sign_module);
    if (ctx == NULL) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: failed to load context");
        return NGX_ERROR;
    }

    u_char       digest[EVP_MAX_MD_SIZE];
    u_char       digest_hex[EVP_MAX_MD_SIZE*2 + 1];
    unsigned int digest_len;

    EVP_MD *evp_md = ngx_http_auth_sign_get_evp_md(log, aslcf->hash);
    if (evp_md == NULL) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: failed to get evp_md");
        return NGX_ERROR;
    }

    if (ctx->token_used.len > 0) {
        return ngx_http_auth_sign_signature_by_token(
            r, message, ctx->token_used, sign, evp_md,
            digest_hex, digest, &digest_len);
    }

    ngx_str_t *t = (ngx_str_t *)aslcf->tokens->elts;
    ngx_int_t status = NGX_ERROR;
    for (size_t i = 0; i < aslcf->tokens->nelts; i++) {
        status = ngx_http_auth_sign_signature_by_token(
            r, message, t[i], sign, evp_md,
            digest_hex, digest, &digest_len);

        if (status == NGX_OK) {
            /* Store token in request context */
            ctx->token_used = t[i];

            return status;
        }
    }

    return status;
}


static ngx_int_t ngx_http_auth_sign_signature_by_token(
    ngx_http_request_t *r, ngx_str_t message, ngx_str_t token,
    ngx_str_t sign, EVP_MD *evp_md, u_char *digest_hex,
    u_char *digest, unsigned int *digest_len
) {
    ngx_log_t *log = r->connection->log;
    ngx_http_auth_sign_loc_conf_t *aslcf =
        (ngx_http_auth_sign_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_sign_module);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                   "auth_sign: checking against token: %V", &(token));

    HMAC(evp_md, token.data, token.len, message.data, message.len, digest, digest_len);
    ngx_hex_dump(digest_hex, digest, *digest_len);

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, log, 0,
                   "auth_sign: calculated signature: %*s", (*digest_len)*2, digest_hex);
    if ((*digest_len*2 == sign.len) && (ngx_memcmp(digest_hex, sign.data, sign.len) == 0)) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,
                "auth_sign: matched!");
        return NGX_OK;
    }

    return aslcf->unauthorized_status;
}


static EVP_MD *ngx_http_auth_sign_get_evp_md(ngx_log_t *log, ngx_uint_t hash) {
    switch (hash) {
        case NGX_HTTP_AUTH_SIGN_MD5:
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,
                          "auth_sign: using MD5 hash function");
            return (EVP_MD *)EVP_md5();

        case NGX_HTTP_AUTH_SIGN_SHA256:
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,
                          "auth_sign: using SHA256 hash function");
            return (EVP_MD *)EVP_sha256();

        case NGX_HTTP_AUTH_SIGN_SHA512:
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, log, 0,
                          "auth_sign: using SHA512 hash function");
            return (EVP_MD *)EVP_sha512();

        default:
            ngx_log_error(NGX_LOG_ERR, log, 0,
                          "auth_sign: unknown hash function %d", hash);
            break;
    }

    return NULL;
}


static time_t
ngx_http_auth_sign_get_timestamp(ngx_http_request_t *r)
{
    ngx_http_auth_sign_loc_conf_t       *aslcf;
    ngx_log_t                           *log;
    ngx_str_t                            ts_str;

    log = r->connection->log;

    aslcf = (ngx_http_auth_sign_loc_conf_t *)ngx_http_get_module_loc_conf(r, ngx_http_auth_sign_module);

    if (ngx_http_complex_value(r, aslcf->expire, &ts_str) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: unable to get expire");
        return NGX_ERROR;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
            "auth_sign: converting expire from %V", &ts_str);

    return ngx_http_auth_sign_get_timestamp_from_str(log, ts_str);
}


static time_t ngx_http_auth_sign_get_timestamp_from_str(ngx_log_t *log, ngx_str_t ts_str) {
    time_t  ts;
    int64_t ts64;

    ts64 = ngx_hextoi64(ts_str.data, ts_str.len);
    if (ts64 == NGX_ERROR) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "auth_sign: unable to convert expire from %V", &ts_str);
        return NGX_ERROR;
    }

    /* If timestamp is in microseconds */
    if (ts64 > NGX_MAX_INT32_VALUE) {
        ts = (time_t)(ts64 / 1000000);
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                "auth_sign: converting timestamp from microseconds: %T", ts);
    } else {
        ts = (time_t)ts64;
    }

    return ts;
}


static ngx_int_t ngx_http_auth_sign_get_variable(ngx_http_request_t *r,
        ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_auth_sign_ctx_t        *ctx;
    ngx_str_t                       *src_var = (ngx_str_t*)data;
    ngx_str_t                        encoded;
    ngx_str_t                        encrypted;
    ngx_int_t                        hash;
    ngx_http_variable_value_t       *vv;
    unsigned char                   *salt;
    unsigned char                   *enc, *dec;
    unsigned char                    key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
    int                              enc_len, dec_len;
    int                              rc;
    EVP_CIPHER_CTX                  *dec_ctx;
    //u_char                           hex_dump[1024];

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: decrypting value from variable %V",
            src_var);

    v->not_found = 1;

    ctx = (ngx_http_auth_sign_ctx_t *)ngx_http_get_module_ctx(r, ngx_http_auth_sign_module);
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    if (ctx->token_used.len == 0 || !ctx->token_used.data) {
        return NGX_OK;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: decrypting value from variable %V with key %V",
            src_var, &ctx->token_used);

    /* Get variable value */
    hash = ngx_hash_strlow(src_var->data, src_var->data, src_var->len);
    vv = ngx_http_get_variable(r, src_var, hash);

    if (vv == NULL || vv->not_found) {
        return NGX_OK;
    }

    /* Decode variable from base64 */
    encoded.len = vv->len;
    encoded.data = vv->data;

    encrypted.len = ngx_base64_decoded_length(encoded.len);
    encrypted.data = (u_char *)ngx_pnalloc(r->pool, encoded.len);
    if (encrypted.data == NULL) {
        return NGX_ERROR;
    }

    v->data = (u_char *)ngx_pnalloc(r->pool, encoded.len + 32);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    if (ngx_decode_base64url(&encrypted, &encoded) != NGX_OK) {
        return NGX_OK;
    }

    /*
    memset(hex_dump, 0, 1024);
    ngx_hex_dump(hex_dump, encrypted.data, encrypted.len);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: encrypted string is %s", hex_dump);
    */

    /* Decrypt variabe content */

    //Minimal length is 16 bytes magick + salt and 1 data block of 32 bytes
    if (encrypted.len < (8 + PKCS5_SALT_LEN + 32) || encrypted.len > 512) {
        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: bad encrypted length %d", encrypted.len);
        return NGX_OK;
    }

    if (ngx_memcmp("Salted__", encrypted.data, 8)) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: bad encrypted magick");
        return NGX_OK;
    }

    salt = encrypted.data + 8;
    enc = encrypted.data + 8 + PKCS5_SALT_LEN;
    enc_len = encrypted.len - 8 - PKCS5_SALT_LEN;
    dec = v->data;
    dec_len = 0;

    EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha256(), salt,
                   ctx->token_used.data, ctx->token_used.len, 1, key, iv);

    /*
    memset(hex_dump, 0, 1024);
    ngx_hex_dump(hex_dump, salt, PKCS5_SALT_LEN);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: salt %s", hex_dump);

    memset(hex_dump, 0, 1024);
    ngx_hex_dump(hex_dump, key, EVP_MAX_KEY_LENGTH);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: key %s", hex_dump);

    memset(hex_dump, 0, 1024);
    ngx_hex_dump(hex_dump, iv, EVP_MAX_IV_LENGTH);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: iv %s", hex_dump);
    */


    dec_ctx = EVP_CIPHER_CTX_new();
    if (dec_ctx == NULL) {
        return NGX_ERROR;
    }

    rc = EVP_DecryptInit_ex(dec_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    if (rc != 1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: failed on DecryptInit_Ex");
        goto failed;
    }

    rc = EVP_CIPHER_CTX_set_padding(dec_ctx, 1);
    if (rc != 1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: failed on set_padding");
        goto failed;
    }

    rc = EVP_DecryptUpdate(dec_ctx, dec, &dec_len, enc, enc_len);
    if (rc != 1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: failed on DecryptUpdate");
        goto failed;
    }
    dec += dec_len;

    rc = EVP_DecryptFinal_ex(dec_ctx, dec, &dec_len);
    if (rc != 1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                "auth_sign: failed on DecryptFinal");
        goto failed;
    }
    dec += dec_len;

    EVP_CIPHER_CTX_free(dec_ctx);

    v->len = dec - v->data;

    /*
    memset(hex_dump, 0, 1024);
    ngx_hex_dump(hex_dump, v->data, v->len);
    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: decrypted string is %s", hex_dump);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
            "auth_sign: decrypted string len %d, %.*s", v->len, v->len, v->data);
    */

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    return NGX_OK;

failed:
    EVP_CIPHER_CTX_free(dec_ctx);
    return NGX_ERROR;
}


static ngx_int_t ngx_http_auth_sign_slash_mode_get_var(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_log_t *log = r->connection->log;

    v->not_found = 1;

    ngx_str_t *var_name = (ngx_str_t *)data;

    ngx_http_auth_sign_ctx_t *ctx = (ngx_http_auth_sign_ctx_t *)
        ngx_http_get_module_ctx(r, ngx_http_auth_sign_module);
    if (ctx == NULL) {
        // TODO: think about this case
        ngx_log_error(NGX_LOG_WARN, log, 0,
                      "auth_sign: failed to load context");
        return NGX_ERROR;
    }

    ngx_http_auth_sign_slash_mode_param_t *ctx_params =
        (ngx_http_auth_sign_slash_mode_param_t *) ctx->slash_mode_params->elts;

    ngx_str_t *var = nullptr;
    for (ngx_uint_t i = 0; i < ctx->slash_mode_params->nelts; ++i) {
        ngx_http_auth_sign_slash_mode_param_t param = ctx_params[i];

        if (param.key.len == var_name->len &&
            ngx_memcmp(param.key.data, var_name->data, param.key.len) == 0
        ) {
            var = &param.value;
            break;
        }
    }

    if (var == nullptr) {
        return NGX_OK;
    }

    v->data = var->data;
    v->len = var->len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;
    return NGX_OK;
}


static char *
ngx_http_auth_sign_opt_set_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    u_char                             *p, *last;
    ngx_uint_t                          i;
    ngx_str_t                          *value;
    ngx_str_t                           var_name;
    ngx_http_auth_sign_optional_t      *opt;
    ngx_http_compile_complex_value_t    ccv;
    ngx_http_auth_sign_loc_conf_t      *aslcf = (ngx_http_auth_sign_loc_conf_t *)conf;

    if (aslcf->optional != NULL && aslcf->optional != NGX_CONF_UNSET_PTR) {
        return (char *)"is duplicate";
    }

    aslcf->optional = ngx_array_create(cf->pool, 8, sizeof(ngx_http_auth_sign_optional_t));
    if (aslcf->optional == NULL)
    {
        return NULL;
    }

    value = (ngx_str_t *)cf->args->elts;

    for (i = 1; i < cf->args->nelts; i++) {

        last = value[i].data + value[i].len;

        p = (u_char *) ngx_strchr(value[i].data, '=');

        if (!p) {
            return (char *)"incorrect parameter, no '=' delimiter";
        }

        if (p == last) {
            return (char *)"incorrect parameter, no variable name after '='";
        }

        opt = (ngx_http_auth_sign_optional_t *)ngx_array_push(aslcf->optional);
        if (opt == NULL) {
            return (char *)NGX_CONF_ERROR;
        }

        opt->name.data = value[i].data;
        opt->name.len = p - value[i].data;

        p++;

        var_name.data = p;
        var_name.len = last - p;

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &var_name;
        ccv.complex_value = &(opt->value);

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return (char *)NGX_CONF_ERROR;
        }

        ngx_log_debug(NGX_LOG_DEBUG_CORE, cf->log, 0,
                      "auth_sign: added param %V val %V", &opt->name, &var_name);
    }

    return (char *)NGX_CONF_OK;
}

static char *
ngx_http_auth_sign_decrypt_add_var(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t                          *name;
    ngx_str_t                          *value;
    ngx_http_variable_t                *var;

    value = (ngx_str_t *)cf->args->elts;

    var = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return (char *)NGX_CONF_ERROR;
    }

    var->get_handler = ngx_http_auth_sign_get_variable;

    name = (ngx_str_t *)ngx_pcalloc(cf->pool, sizeof(ngx_str_t));
    if (name == NULL) {
        return (char *)NGX_CONF_ERROR;
    }

    name->len = value[2].len;
    name->data = (u_char *)ngx_pcalloc(cf->pool, name->len);
    if (name->data == NULL) {
        return (char *)NGX_CONF_ERROR;
    }

    ngx_memcpy(name->data, value[2].data, name->len);

    var->data = (uintptr_t)name;

    return (char *)NGX_CONF_OK;
}


static char *
ngx_http_auth_sign_slash_mode_add_var(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_str_t                          *name;
    ngx_str_t                          *value;
    ngx_http_variable_t                *var;

    value = (ngx_str_t *)cf->args->elts;
    if (value[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return (char *)NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;

    var = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (var == NULL) {
        return (char *)NGX_CONF_ERROR;
    }

    var->get_handler = ngx_http_auth_sign_slash_mode_get_var;

    name = (ngx_str_t *)ngx_pcalloc(cf->pool, sizeof(ngx_str_t));
    if (name == NULL) {
        return (char *)NGX_CONF_ERROR;
    }

    name->len = value[2].len;
    name->data = (u_char *)ngx_pcalloc(cf->pool, name->len);
    if (name->data == NULL) {
        return (char *)NGX_CONF_ERROR;
    }

    ngx_memcpy(name->data, value[2].data, name->len);

    var->data = (uintptr_t)name;

    return (char *)NGX_CONF_OK;
}

static void *
ngx_http_auth_sign_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_auth_sign_loc_conf_t  *conf;

    conf = (ngx_http_auth_sign_loc_conf_t *)ngx_pcalloc(cf->pool, sizeof(ngx_http_auth_sign_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->tokens = (ngx_array_t *)NGX_CONF_UNSET_PTR;
    conf->optional = (ngx_array_t *)NGX_CONF_UNSET_PTR;
    conf->hash = NGX_CONF_UNSET_UINT;
    conf->unauthorized_status = NGX_CONF_UNSET_UINT;
    conf->expired_status = NGX_CONF_UNSET_UINT;
    conf->sign = NULL;
    conf->message = NULL;
    conf->expire = NULL;
    conf->pass_if_empty = NGX_CONF_UNSET;
    conf->slash_mode = NGX_CONF_UNSET;

    return conf;
}


static char *
ngx_http_auth_sign_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_auth_sign_loc_conf_t  *prev = (ngx_http_auth_sign_loc_conf_t *)parent;
    ngx_http_auth_sign_loc_conf_t  *conf = (ngx_http_auth_sign_loc_conf_t *)child;

    ngx_conf_merge_ptr_value(conf->tokens,
                             prev->tokens, NULL);

    ngx_conf_merge_ptr_value(conf->optional,
                             prev->optional, NULL);

    ngx_conf_merge_uint_value(conf->hash,
                              prev->hash, NGX_HTTP_AUTH_SIGN_MD5);

    ngx_conf_merge_uint_value(conf->unauthorized_status,
                              prev->unauthorized_status, NGX_HTTP_UNAUTHORIZED);

    ngx_conf_merge_uint_value(conf->expired_status,
                              prev->expired_status, NGX_HTTP_UNAUTHORIZED);

    if (conf->sign == NULL) {
            conf->sign = prev->sign;
    }

    if (conf->message == NULL) {
            conf->message = prev->message;
    }

    if (conf->expire == NULL) {
        conf->expire = prev->expire;
    }

    ngx_conf_merge_value(conf->pass_if_empty,
                         prev->pass_if_empty, 0);

    ngx_conf_merge_value(conf->slash_mode,
                         prev->slash_mode, 0);

    if (conf->tokens == NULL) {
        return NGX_CONF_OK;
    }

    if (conf->slash_mode) {
        return NGX_CONF_OK;
    }

    if (!conf->message) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "auth_sign is required");
    }
    if (!conf->sign) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "auth_sign_signature is required");
    }
    if (!conf->expire) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                           "auth_sign_expire is required");
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_auth_sign_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = (ngx_http_core_main_conf_t *)ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = (ngx_http_handler_pt *)ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_auth_sign_handler;

    return NGX_OK;
}
