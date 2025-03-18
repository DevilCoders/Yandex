
/*
 * Copyright (C) Anton Kortunov
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef enum {
    file_type_flv = 0,
    file_type_mp4
// TODO: file_type_moov
} file_type_t;

typedef struct {
    off_t        start;
    off_t        end;
    off_t        offset;
    file_type_t  file_type;
} ngx_http_flv_filter_ctx_t;

typedef struct {
    ngx_uint_t                  type;
    ngx_http_complex_value_t   *file_type;
} ngx_http_flv_filter_conf_t;

static u_char  ngx_flv_header[] = "FLV\x1\x5\0\0\0\x9\0\0\0\0";


static ngx_int_t ngx_http_flv_parse(ngx_http_request_t *r,
    ngx_http_flv_filter_ctx_t *ctx);

static ngx_int_t ngx_http_flv_filter_init(ngx_conf_t *cf);

static char * ngx_http_flv_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void * ngx_http_flv_filter_create_conf(ngx_conf_t *cf);
static char * ngx_http_flv_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child);

#define NGX_FLV_FILTER_OFF      0
#define NGX_FLV_FILTER_CACHED   1
#define NGX_FLV_FILTER_ON       2


static ngx_command_t  ngx_http_flv_filter_commands[] = {

    { ngx_string("flv_filter"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE12,
      ngx_http_flv_filter,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL},

      ngx_null_command
};


static ngx_http_module_t  ngx_http_flv_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_flv_filter_init,              /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_flv_filter_create_conf,       /* create location configuration */
    ngx_http_flv_filter_merge_conf         /* merge location configuration */
};


ngx_module_t  ngx_http_flv_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_flv_filter_module_ctx,       /* module context */
    ngx_http_flv_filter_commands,          /* module directives */
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


static ngx_http_output_header_filter_pt  ngx_http_next_header_filter;
static ngx_http_output_body_filter_pt    ngx_http_next_body_filter;


static ngx_int_t
ngx_http_flv_header_filter(ngx_http_request_t *r)
{
    ngx_http_flv_filter_ctx_t  *ctx;
    ngx_http_flv_filter_conf_t *ffcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http flv filter header filter");

    ffcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_filter_module);

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http flv filter header filter: type: %d", ffcf->type);

    switch(ffcf->type) {
        case NGX_FLV_FILTER_ON:
            break;

#if (NGX_HTTP_CACHE)
        case NGX_FLV_FILTER_CACHED:
            if (r->upstream && (r->upstream->cache_status == NGX_HTTP_CACHE_HIT)) {
                break;
            }

#endif
        default: /* NGX_FLV_FILTER_OFF */
            return ngx_http_next_header_filter(r);
    }

    if (r->http_version < NGX_HTTP_VERSION_10
        || r->headers_out.status != NGX_HTTP_OK
        || r != r->main
        || r->headers_out.content_length_n == -1)
    {
        return ngx_http_next_header_filter(r);
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_flv_filter_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    switch (ngx_http_flv_parse(r, ctx)) {

    case NGX_OK:
        ngx_http_set_ctx(r, ctx, ngx_http_flv_filter_module);

        r->headers_out.status = NGX_HTTP_OK;
        if (ctx->start > 0 || ctx->end < r->headers_out.content_length_n) {
            r->headers_out.content_length_n = ctx->end - ctx->start;

            if (ctx->start > 0 && ctx->file_type == file_type_flv) {
                r->headers_out.content_length_n += sizeof(ngx_flv_header) - 1;
            }
        }

        if (r->headers_out.content_length) {
            r->headers_out.content_length->hash = 0;
            r->headers_out.content_length = NULL;
        }
        break;

    case NGX_ERROR:
        return NGX_ERROR;

    default: /* NGX_DECLINED */
        break;
    }

    return ngx_http_next_header_filter(r);
}


static ngx_int_t
ngx_http_flv_parse(ngx_http_request_t *r, ngx_http_flv_filter_ctx_t *ctx)
{
    ngx_http_flv_filter_conf_t *ffcf;
    off_t              start, end, content_length;
    ngx_str_t          value;
    u_char             *p;

    content_length = r->headers_out.content_length_n;
    start = 0;
    end = content_length;

    ffcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_filter_module);

    if (ffcf->file_type == NULL) {
        ctx->file_type = file_type_flv;

    } else {
        if (ngx_http_complex_value(r, ffcf->file_type, &value) != NGX_OK) {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http flv filter: unable to get file type");

            return NGX_DECLINED;
        }

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http flv filter: file type len = %d, value = |%V|", value.len, &value);

        if (value.len == 0) {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http flv filter: empty file type");
            return NGX_DECLINED;

        } else if (ngx_strncmp(value.data, "flv", value.len) == 0) {
            ctx->file_type = file_type_flv;

        } else if (ngx_strncmp(value.data, "mp4", value.len) == 0) {
            ctx->file_type = file_type_mp4;

        } else {
            ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                          "http flv filter: unknown file type %V", &value);
            return NGX_DECLINED;
        }
    }

    if (r->args.len) {
        if (ngx_http_arg(r, (u_char *) "start", 5, &value) == NGX_OK) {

            start = ngx_atoof(value.data, value.len);

            if (start == NGX_ERROR || start >= content_length || start <= 0) {
                return NGX_DECLINED;
            }
        } else if (ngx_http_arg(r, (u_char *) "range", 5, &value) == NGX_OK) {

           p = value.data;

           while (p < value.data + value.len) {
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                               "http flv filter p = %c", *p);

                if (*p == '-') {
                    start = ngx_atoof(value.data, p - value.data);

                    if (start == NGX_ERROR || start >= content_length || start < 0) {
                        return NGX_DECLINED;
                    }

                    end = ngx_atoof(p + 1, value.len - (p - value.data) - 1) + 1;

                    if (end == NGX_ERROR || end <= 0 || end <= start) {
                        return NGX_DECLINED;
                    }

                    if (end > content_length) {
                        end = content_length;
                    }

                    break;
                }

                p++;
            }

        } else {
            return NGX_DECLINED;
        }
    } else {
        return NGX_DECLINED;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http flv filter start = %d, end = %d", (int)start, (int)end);

    ctx->start = start;
    ctx->end = end;
    ctx->offset = 0;

    return NGX_OK;
}

static ngx_int_t
ngx_http_flv_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_flv_filter_ctx_t  *ctx;
    off_t                       start, last;
    ngx_buf_t                  *buf;
    ngx_chain_t                *out, *cl, **ll, header;
    ngx_http_flv_filter_conf_t *ffcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http flv filter body filter");

    if (in == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    ffcf = ngx_http_get_module_loc_conf(r, ngx_http_flv_filter_module);

    if (ffcf->type == NGX_FLV_FILTER_OFF) {
        return ngx_http_next_body_filter(r, in);
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_flv_filter_module);

    if (ctx == NULL) {
        return ngx_http_next_body_filter(r, in);
    }

    out = NULL;
    ll = &out;

    /* Add streaming header if needed */
    if (ctx->offset == 0 && ctx->start > 0 && ctx->file_type == file_type_flv) {
        buf = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
        if (buf == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        buf->pos = ngx_flv_header;
        buf->last = ngx_flv_header + sizeof(ngx_flv_header) - 1;
        buf->memory = 1;

        header.buf = buf;
        header.next = NULL;

        *ll = &header;
        ll = &header.next;
    }

    for (cl = in; cl; cl = cl->next) {

        buf = cl->buf;

        start = ctx->offset;
        last = ctx->offset + ngx_buf_size(buf);

        ctx->offset += ngx_buf_size(buf);

        ngx_log_debug2(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http flv filter body buf: %O-%O", start, last);

        if (ngx_buf_special(buf)) {
            *ll = cl;
            ll = &cl->next;
            continue;
        }

        if (ctx->start > last || ctx->end < start) {

            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http flv filter body skip");

            if (buf->in_file) {
                buf->file_pos = buf->file_last;
            }

            buf->pos = buf->last;
            buf->sync = 1;

            continue;
        }

        if (ctx->start > start) {

            if (buf->in_file) {
                buf->file_pos += ctx->start - start;
            }

            if (ngx_buf_in_memory(buf)) {
                buf->pos += (size_t) (ctx->start - start);
            }
        }

        if (ctx->end < last) {

            if (buf->in_file) {
                buf->file_last -= last - ctx->end;
            }

            if (ngx_buf_in_memory(buf)) {
                buf->last -= (size_t) (last - ctx->end);
            }

            buf->last_buf = 1;
            *ll = cl;
            cl->next = NULL;

            break;
        }

        *ll = cl;
        ll = &cl->next;
    }

    if (out == NULL) {
        return NGX_OK;
    }

    return ngx_http_next_body_filter(r, out);
}

static ngx_int_t
ngx_http_flv_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_header_filter = ngx_http_top_header_filter;
    ngx_http_top_header_filter = ngx_http_flv_header_filter;

    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_flv_body_filter;

    return NGX_OK;
}

static char *
ngx_http_flv_filter(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_flv_filter_conf_t *ffcf = conf;

    ngx_str_t                         *value;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (ngx_strcmp(value[1].data, "off") == 0) {
        ffcf->type = NGX_FLV_FILTER_OFF;

#if (NGX_HTTP_CACHE)
    } else if (ngx_strcmp(value[1].data, "cached") == 0) {
        ffcf->type = NGX_FLV_FILTER_CACHED;

#endif
    } else if (ngx_strcmp(value[1].data, "on") == 0) {
        ffcf->type = NGX_FLV_FILTER_ON;

    } else {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%V\"",
                           &value[1]);
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts == 3) {
        ffcf->file_type = ngx_palloc(cf->pool, sizeof(ngx_http_complex_value_t));
        if (ffcf->file_type == NULL) {
            return NGX_CONF_ERROR;
        }

        ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

        ccv.cf = cf;
        ccv.value = &value[2];
        ccv.complex_value = ffcf->file_type;

        if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
            return NGX_CONF_ERROR;
        }
    }

    return NGX_CONF_OK;
}

static void *
ngx_http_flv_filter_create_conf(ngx_conf_t *cf)
{
    ngx_http_flv_filter_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_flv_filter_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->type = NGX_CONF_UNSET_UINT;
    conf->file_type = NULL;

    return conf;
}

static char *
ngx_http_flv_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_flv_filter_conf_t *prev = parent;
    ngx_http_flv_filter_conf_t *conf = child;

    ngx_conf_merge_uint_value(conf->type, prev->type, NGX_FLV_FILTER_OFF);

    if (conf->file_type == NULL) {
        conf->file_type = prev->file_type;
    }

    return NGX_CONF_OK;
}

