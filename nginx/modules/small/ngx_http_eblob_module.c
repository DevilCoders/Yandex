
/*
 * Copyright (C) Anton Kortunov
 * Copyright (C) Igor Sysoev
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <magic.h>

/* Hack for old libmagick from Hardy */
#ifndef MAGIC_MIME_TYPE
#define MAGIC_MIME_TYPE MAGIC_MIME
#endif

typedef struct {
    magic_t     magic;
} ngx_http_eblob_conf_t;

const char eblob_separator = ':';

static char *ngx_http_eblob(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_eblob_create_conf(ngx_conf_t *cf);
static void ngx_http_eblob_cleanup(void *data);

static ngx_command_t  ngx_http_eblob_commands[] = {

    { ngx_string("eblob"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_http_eblob,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_eblob_module_ctx = {
    NULL,                          /* preconfiguration */
    NULL,                          /* postconfiguration */

    ngx_http_eblob_create_conf,    /* create main configuration */
    NULL,                          /* init main configuration */

    NULL,                          /* create server configuration */
    NULL,                          /* merge server configuration */

    NULL,                          /* create location configuration */
    NULL                           /* merge location configuration */
};


ngx_module_t  ngx_http_eblob_module = {
    NGX_MODULE_V1,
    &ngx_http_eblob_module_ctx,      /* module context */
    ngx_http_eblob_commands,         /* module directives */
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
ngx_http_eblob_handler(ngx_http_request_t *r)
{
    u_char                    *last, *content;
    const char                *ct;
    off_t                      content_size = 10*1024; /* Pass the first 10kb to libmagic */
    off_t                      len, offset, length;
    size_t                     root;
    ngx_int_t                  rc;
    ngx_uint_t                 level, i;
    ngx_str_t                  path;
    ngx_log_t                 *log;
    ngx_buf_t                 *b;
    ngx_chain_t                out;
    ngx_open_file_info_t       of;
    ngx_http_eblob_conf_t     *ecf;
    ngx_http_core_loc_conf_t  *clcf;

    if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD))) {
        return NGX_HTTP_NOT_ALLOWED;
    }

    if (r->uri.data[r->uri.len - 1] == '/') {
        return NGX_DECLINED;
    }

    rc = ngx_http_discard_request_body(r);

    if (rc != NGX_OK) {
        return rc;
    }

    last = ngx_http_map_uri_to_path(r, &path, &root, 0);
    if (last == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    log = r->connection->log;

    path.len = last - path.data;
    offset = 0;
    length = 0;

    for (i = path.len-1; i > 0; i--) {
        if (path.data[i] == eblob_separator) {
            length = ngx_atoof(path.data + i + 1, path.len - i - 1);
            path.len = i;
            path.data[i] = '\0';
            break;
        }
    }

    for (i = path.len-1; i > 0; i--) {
        if (path.data[i] == eblob_separator) {
            offset = ngx_atoof(path.data + i + 1, path.len - i - 1);
            path.len = i;
            path.data[i] = '\0';
            break;
        }
    }

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, log, 0,
                   "http eblob filename: \"%V\", offset: %d, length: %d",
                   &path, offset, length);

    clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

    ngx_memzero(&of, sizeof(ngx_open_file_info_t));

    of.read_ahead = clcf->read_ahead;
    of.directio = clcf->directio;
    of.valid = clcf->open_file_cache_valid;
    of.min_uses = clcf->open_file_cache_min_uses;
    of.errors = clcf->open_file_cache_errors;
    of.events = clcf->open_file_cache_events;

    if (ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool)
        != NGX_OK)
    {
        switch (of.err) {

        case 0:
            return NGX_HTTP_INTERNAL_SERVER_ERROR;

        case NGX_ENOENT:
        case NGX_ENOTDIR:
        case NGX_ENAMETOOLONG:

            level = NGX_LOG_ERR;
            rc = NGX_HTTP_NOT_FOUND;
            break;

        case NGX_EACCES:

            level = NGX_LOG_ERR;
            rc = NGX_HTTP_FORBIDDEN;
            break;

        default:

            level = NGX_LOG_CRIT;
            rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
            break;
        }

        if (rc != NGX_HTTP_NOT_FOUND || clcf->log_not_found) {
            ngx_log_error(level, log, of.err,
                          "%s \"%s\" failed", of.failed, path.data);
        }

        return rc;
    }

    if (!of.is_file) {

        if (ngx_close_file(of.fd) == NGX_FILE_ERROR) {
            ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                          ngx_close_file_n " \"%s\" failed", path.data);
        }

        return NGX_DECLINED;
    }

    r->root_tested = !r->error_page;

    len = of.size;

#if 0
    if (r->args.len) {
        if (ngx_http_arg(r, (u_char *) "length", 5, &value) == NGX_OK) {

            length = ngx_atoof(value.data, value.len);

            if (length == NGX_ERROR) {
                length = 0;
            }
        }

        if (ngx_http_arg(r, (u_char *) "offset", 5, &value) == NGX_OK) {

            offset = ngx_atoof(value.data, value.len);

            if (offset == NGX_ERROR) {
                offset = 0;
            }
        }


    }
#endif

    if (offset > len) {
        offset = 0;
        length = 0;
    }

    if (offset + length > len) {
        length = len - offset;
    }

    ngx_log_debug3(NGX_LOG_DEBUG_HTTP, log, 0,
                   "http eblob filename: \"%V\", offset: %d, length: %d",
                   &path, offset, length);
    log->action = "sending eblob content to client";

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = length;
    r->headers_out.last_modified_time = of.mtime;
    r->allow_ranges = 1;

    b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
    if (b == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
    if (b->file == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    b->file_pos = offset;
    b->file_last = offset + length;

    b->in_file = b->file_last ? 1: 0;
    b->last_buf = 1;
    b->last_in_chain = 1;

    b->file->fd = of.fd;
    b->file->name = path;
    b->file->log = log;
    b->file->directio = of.is_directio;

    /* Determine content type using libmagic */
    ecf = ngx_http_get_module_main_conf(r, ngx_http_eblob_module);
    if (ecf->magic) {
        if (content_size > length) {
            content_size = length;
        }

        content = ngx_palloc(r->pool, content_size);
        if (content == NULL) {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        rc = ngx_read_file(b->file, content, content_size, offset);
        if (rc == NGX_ERROR) {
            ngx_log_error(NGX_LOG_WARN, log, 0,
                         "http eblob failed to read file: filename: \"%V\" errno: %d",
                         &path, rc);
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ct = magic_buffer(ecf->magic, content, content_size);

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                       "http eblob got content type: %s", ct?ct:"NULL");

        if (ct) {
            r->headers_out.content_type.data = (u_char *)ct;
            r->headers_out.content_type.len = ngx_strlen(ct);
            r->headers_out.content_type_len = r->headers_out.content_type.len;
        } else {
            r->headers_out.content_type_len = clcf->default_type.len;
            r->headers_out.content_type = clcf->default_type;
        }
    }

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    out.buf = b;
    out.next = NULL;

    return ngx_http_output_filter(r, &out);
}


static char *
ngx_http_eblob(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_eblob_conf_t     *ecf = conf;
    ngx_http_core_loc_conf_t  *clcf;

    ecf->magic = magic_open(MAGIC_MIME_TYPE);
    if (!ecf->magic) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "unable to initialize libmagic: %d", errno);
    } else {
        if (magic_load(ecf->magic, NULL)) {
            ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                               "unable to load libmagic file: %d", errno);
        }
    }

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
    clcf->handler = ngx_http_eblob_handler;

    return NGX_CONF_OK;
}


static void *
ngx_http_eblob_create_conf(ngx_conf_t *cf)
{
    ngx_pool_cleanup_t     *cln;
    ngx_http_eblob_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_eblob_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->magic = NULL;

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NULL;
    }

    cln->handler = ngx_http_eblob_cleanup;
    cln->data = conf;

    return conf;
}


static void
ngx_http_eblob_cleanup(void *data)
{
    ngx_http_eblob_conf_t  *ecf = data;

    if (ecf->magic) {
        magic_close(ecf->magic);
    }
}
