
/*
 * Copyright (C) Anton Kortunov
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#if (NGX_ZLIB)
#include <contrib/libs/zlib/zlib.h>
#endif


#include <string.h>
#include <syslog.h>

/* ISO8601 timestamp = "1970-09-28T12:00:00+06:00" */
#define TIME_LEN  (sizeof("1970-09-28T12:00:00") - 1)
#define TIMEZONE_OFF  sizeof("28/Sep/1970:12:00:00")
#define NGX_TSKV_SYSLOG_MAX_STR                                                    \
NGX_MAX_ERROR_STR + sizeof("<255>Jan 01 00:00:00 ") - 1                   \
+ (NGX_MAXHOSTNAMELEN - 1) + 1 /* space */                                \
+ 32 /* tag */ + 2 /* colon, space */

typedef struct ngx_http_tskvlog_op_s  ngx_http_tskvlog_op_t;
typedef struct ngx_http_tskvlog_s     ngx_http_tskvlog_t;

typedef u_char *(*ngx_http_tskvlog_op_run_pt) (ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);

typedef size_t (*ngx_http_tskvlog_op_getlen_pt) (ngx_http_request_t *r,
    uintptr_t data);

typedef void (*ngx_http_tskvlog_writer_pt) (ngx_http_tskvlog_t *log,
    u_char *buf, size_t len);

struct ngx_http_tskvlog_op_s {
    size_t                          len;
    ngx_http_tskvlog_op_getlen_pt   getlen;
    ngx_http_tskvlog_op_run_pt      run;
    uintptr_t                       data;
    ngx_str_t                       name;
};


typedef struct {
    ngx_str_t                   name;
    ngx_array_t                *flushes;
    ngx_array_t                *ops;        /* array of ngx_http_tskvlog_op_t */
} ngx_http_tskvlog_fmt_t;


typedef struct {
    ngx_array_t                 formats;    /* array of ngx_http_tskvlog_fmt_t */
} ngx_http_tskvlog_main_conf_t;


typedef struct {
    u_char                     *start;
    u_char                     *pos;
    u_char                     *last;

    ngx_event_t                *event;
    ngx_msec_t                  flush;
    ngx_int_t                   gzip;
} ngx_http_tskvlog_buf_t;


typedef struct {
    ngx_array_t                *lengths;
    ngx_array_t                *values;
} ngx_http_tskvlog_script_t;


struct ngx_http_tskvlog_s {
    ngx_open_file_t            *file;
    time_t                      disk_full_time;
    time_t                      error_log_time;
    ngx_http_tskvlog_fmt_t     *format;
    ngx_syslog_peer_t          *syslog_peer;
    ngx_http_complex_value_t   *filter;
};


typedef struct {
    ngx_array_t                *logs;       /* array of ngx_http_tskvlog_t */
    ngx_uint_t                  off;        /* unsigned  off:1 */
} ngx_http_tskvlog_loc_conf_t;


typedef struct {
    ngx_str_t                   name;
    size_t                      len;
    ngx_http_tskvlog_op_run_pt  run;
} ngx_http_tskvlog_var_t;

typedef struct {
    ngx_str_t                   name;
    ngx_str_t                   var;
} ngx_http_tskvlog_default_var_t;


static void ngx_http_tskvlog_syslog_write(ngx_http_request_t *r, ngx_http_tskvlog_t *log,
    u_char *buf, size_t len);
static void ngx_http_tskvlog_write(ngx_http_request_t *r, ngx_http_tskvlog_t *log,
    u_char *buf, size_t len);

#if (NGX_ZLIB)
static ssize_t ngx_http_tskvlog_gzip(ngx_fd_t fd, u_char *buf, size_t len,
    ngx_int_t level, ngx_log_t *log);

static void *ngx_http_tskvlog_gzip_alloc(void *opaque, u_int items, u_int size);
static void ngx_http_tskvlog_gzip_free(void *opaque, void *address);
#endif

static void ngx_http_tskvlog_flush(ngx_open_file_t *file, ngx_log_t *log);
static void ngx_http_tskvlog_flush_handler(ngx_event_t *ev);

static u_char *ngx_http_tskvlog_time(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_timezone(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_unixtime(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_request_time(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_status(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_bytes_sent(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_body_bytes_sent(ngx_http_request_t *r,
    u_char *buf, ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_request_length(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static u_char *ngx_http_tskvlog_copy_long(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);

static ngx_int_t ngx_http_tskvlog_add_var(ngx_conf_t *cf, ngx_array_t *ops, ngx_str_t *name,
    ngx_str_t *var, ngx_array_t *flushes);
static ngx_int_t ngx_http_tskvlog_add_const(ngx_conf_t *cf, ngx_array_t *ops, ngx_str_t *name,
    ngx_str_t *var);
static ngx_int_t ngx_http_tskvlog_variable_compile(ngx_conf_t *cf,
    ngx_http_tskvlog_op_t *op, ngx_str_t *name, ngx_str_t *value);
static size_t ngx_http_tskvlog_variable_getlen(ngx_http_request_t *r,
    uintptr_t data);
static u_char *ngx_http_tskvlog_variable(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op);
static uintptr_t ngx_http_tskvlog_escape(u_char *dst, u_char *src, size_t size);


static void *ngx_http_tskvlog_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_tskvlog_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_tskvlog_merge_loc_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_tskvlog_set_log(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_tskvlog_set_format(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_tskvlog_compile_format(ngx_conf_t *cf,
    ngx_http_tskvlog_fmt_t *fmt, ngx_array_t *args, ngx_uint_t s);
static ngx_int_t ngx_http_tskvlog_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_tskvlog_commands[] = {

    { ngx_string("tskv_log_format"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_2MORE,
      ngx_http_tskvlog_set_format,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("tskv_log"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_HTTP_LMT_CONF|NGX_CONF_1MORE,
      ngx_http_tskvlog_set_log,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_tskvlog_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_tskvlog_init,                     /* postconfiguration */

    ngx_http_tskvlog_create_main_conf,         /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_tskvlog_create_loc_conf,          /* create location configuration */
    ngx_http_tskvlog_merge_loc_conf            /* merge location configuration */
};


ngx_module_t  ngx_http_tskvlog_module = {
    NGX_MODULE_V1,
    &ngx_http_tskvlog_module_ctx,              /* module context */
    ngx_http_tskvlog_commands,                 /* module directives */
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


static ngx_str_t ngx_http_tskv_format = ngx_string("tskv_format");
static char      tskv_prefix[]        = "tskv";


static ngx_http_tskvlog_var_t  ngx_http_tskvlog_vars[] = {
    { ngx_string("timestamp"), sizeof("28/Sep/1970:12:00:00") - 1,
                          ngx_http_tskvlog_time },
    { ngx_string("timezone"), sizeof("+0600") - 1,
                          ngx_http_tskvlog_timezone },
    { ngx_string("unixtime"), NGX_TIME_T_LEN + 4,
                          ngx_http_tskvlog_unixtime },
    { ngx_string("request_time"), NGX_TIME_T_LEN + 4,
                          ngx_http_tskvlog_request_time },
    { ngx_string("status"), NGX_INT_T_LEN, ngx_http_tskvlog_status },
    { ngx_string("bytes_sent"), NGX_OFF_T_LEN, ngx_http_tskvlog_bytes_sent },
    { ngx_string("body_bytes_sent"), NGX_OFF_T_LEN,
                          ngx_http_tskvlog_body_bytes_sent },
    { ngx_string("request_length"), NGX_SIZE_T_LEN,
                          ngx_http_tskvlog_request_length },

    { ngx_null_string, 0, NULL }
};

static ngx_http_tskvlog_default_var_t ngx_http_tskvlog_default_vars[] = {
    { ngx_string("timestamp"), ngx_string("timestamp") },
    { ngx_string("timezone"), ngx_string("timezone") },
    { ngx_string("status"), ngx_string("status") },
    { ngx_string("protocol"), ngx_string("server_protocol") },
    { ngx_string("method"), ngx_string("request_method") },
    { ngx_string("request"), ngx_string("request_uri") },
    { ngx_string("referer"), ngx_string("http_referer") },
    { ngx_string("cookies"), ngx_string("http_cookie") },
    { ngx_string("user_agent"), ngx_string("http_user_agent") },
    { ngx_string("vhost"), ngx_string("http_host") },
    { ngx_string("ip"), ngx_string("remote_addr") },
    { ngx_string("x_forwarded_for"), ngx_string("http_x_forwarded_for") },
    { ngx_string("x_real_ip"), ngx_string("http_x_real_ip") },
    { ngx_null_string, ngx_null_string }
};

static ngx_int_t
ngx_http_tskvlog_handler(ngx_http_request_t *r)
{
    u_char                       *line, *p;
    size_t                        len;
    ngx_str_t                     val;
    ngx_uint_t                    i, l;
    ngx_http_tskvlog_t           *log;
    ngx_http_tskvlog_op_t        *op;
    ngx_http_tskvlog_buf_t       *buffer;
    ngx_http_tskvlog_loc_conf_t  *lcf;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http tskv log handler");

    lcf = ngx_http_get_module_loc_conf(r, ngx_http_tskvlog_module);

    if (lcf->logs == NULL || lcf->off) {
        return NGX_OK;
    }

    log = lcf->logs->elts;
    for (l = 0; l < lcf->logs->nelts; l++) {

        if (log[l].file) {
            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http tskv got log: %s", log[l].file->name.data);
        } else {
            ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                           "http tskv got log: syslog");
        }


        if (log[l].filter) {
            if (ngx_http_complex_value(r, log[l].filter, &val) != NGX_OK) {
                return NGX_ERROR;
            }

            if (val.len == 0 || (val.len == 1 && val.data[0] == '0')) {
                continue;
            }
        }

        if (ngx_time() == log[l].disk_full_time) {

            /*
             * on FreeBSD writing to a full filesystem with enabled softupdates
             * may block process for much longer time than writing to non-full
             * filesystem, so we skip writing to a log for one second
             */

            continue;
        }

        ngx_http_script_flush_no_cacheable_variables(r, log[l].format->flushes);
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                       "http tskv flush no cacheable vars");

        len = 4; /* Line starts with tskv */
        op = log[l].format->ops->elts;
        for (i = 0; i < log[l].format->ops->nelts; i++) {
            /* op len = '\t' + name + '=' + 'value' */
            len += 1 + op[i].name.len + 1;
            if (op[i].len == 0) {
                len += op[i].getlen(r, op[i].data);

            } else {
                len += op[i].len;
            }
        }

        len += NGX_LINEFEED_SIZE + sizeof(tskv_prefix) - 1;

        buffer = log[l].file ? log[l].file->data : NULL;

        if (buffer) {

            if (len > (size_t) (buffer->last - buffer->pos)) {

                ngx_http_tskvlog_write(r, &log[l], buffer->start,
                                       buffer->pos - buffer->start);

                buffer->pos = buffer->start;
            }

            if (len <= (size_t) (buffer->last - buffer->pos)) {

                p = buffer->pos;

                if (buffer->event && p == buffer->start) {
                    ngx_add_timer(buffer->event, buffer->flush);
                }

                p = ngx_cpymem(p, tskv_prefix, sizeof(tskv_prefix) - 1);

                for (i = 0; i < log[l].format->ops->nelts; i++) {
                    *p++ = '\t';
                    p = ngx_cpymem(p, op[i].name.data, op[i].name.len);
                    *p++ = '=';
                    p = op[i].run(r, p, &op[i]);
                }

                ngx_linefeed(p);

                buffer->pos = p;

                return NGX_OK;
            }

            if (buffer->event && buffer->event->timer_set) {
                ngx_del_timer(buffer->event);
            }
        }

        if (log[l].syslog_peer) {
            /* length of syslog's PRI and HEADER message parts */
            len += sizeof("<255>Jan 01 00:00:00 ") - 1
                   + ngx_cycle->hostname.len + 1
                   + log[l].syslog_peer->tag.len + 2;
        }

        line = ngx_pnalloc(r->pool, len);
        if (line == NULL) {
            return NGX_ERROR;
        }

        p = line;

        if (log[l].syslog_peer) {
            p = ngx_syslog_add_header(log[l].syslog_peer, p);
        }

        p = ngx_cpymem(p, tskv_prefix, sizeof(tskv_prefix) - 1);

        for (i = 0; i < log[l].format->ops->nelts; i++) {
            *p++ = '\t';
            p = ngx_cpymem(p, op[i].name.data, op[i].name.len);
            *p++ = '=';
            p = op[i].run(r, p, &op[i]);
        }

        if (log[l].syslog_peer) {
            ngx_http_tskvlog_syslog_write(r, &log[l], line, p - line);
        } else {
            ngx_linefeed(p);
            ngx_http_tskvlog_write(r, &log[l], line, p - line);
        }
    }

    return NGX_OK;
}

static void
ngx_http_tskvlog_syslog_write(ngx_http_request_t *r, ngx_http_tskvlog_t *log, u_char *buf,
    size_t len)
{
    ssize_t ret;

    if (log->syslog_peer->busy) {
        return;
    }

    log->syslog_peer->busy = 1;
    //log->syslog_peer->severity = level - 1;

    ret = ngx_syslog_send(log->syslog_peer, buf, len);
    if (ret < 0) {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                "send() to syslog failed");

    } else if ((size_t) ret != len) {
        ngx_log_error(NGX_LOG_WARN, r->connection->log, 0,
                "send() to syslog has written only %z of %uz",
                ret, len);
    }

    log->syslog_peer->busy = 0;
}


static void
ngx_http_tskvlog_write(ngx_http_request_t *r, ngx_http_tskvlog_t *log, u_char *buf,
    size_t len)
{
    u_char              *name;
    time_t               now;
    ssize_t              n;
    ngx_err_t            err;
#if (NGX_ZLIB)
    ngx_http_tskvlog_buf_t  *buffer;
#endif

    name = log->file->name.data;

#if (NGX_ZLIB)
    buffer = log->file->data;

    if (buffer && buffer->gzip) {
        n = ngx_http_tskvlog_gzip(log->file->fd, buf, len, buffer->gzip,
                              r->connection->log);
    } else {
        n = ngx_write_fd(log->file->fd, buf, len);
    }
#else
    n = ngx_write_fd(log->file->fd, buf, len);
#endif

    if (n == (ssize_t) len) {
        return;
    }

    now = ngx_time();

    if (n == -1) {
        err = ngx_errno;

        if (err == NGX_ENOSPC) {
            log->disk_full_time = now;
        }

        if (now - log->error_log_time > 59) {
            ngx_log_error(NGX_LOG_ALERT, r->connection->log, err,
                          ngx_write_fd_n " to \"%s\" failed", name);

            log->error_log_time = now;
        }

        return;
    }

    if (now - log->error_log_time > 59) {
        ngx_log_error(NGX_LOG_ALERT, r->connection->log, 0,
                      ngx_write_fd_n " to \"%s\" was incomplete: %z of %uz",
                      name, n, len);

        log->error_log_time = now;
    }
}


#if (NGX_ZLIB)

static ssize_t
ngx_http_tskvlog_gzip(ngx_fd_t fd, u_char *buf, size_t len, ngx_int_t level,
    ngx_log_t *log)
{
    int          rc, wbits, memlevel;
    u_char      *out;
    size_t       size;
    ssize_t      n;
    z_stream     zstream;
    ngx_err_t    err;
    ngx_pool_t  *pool;

    wbits = MAX_WBITS;
    memlevel = MAX_MEM_LEVEL - 1;

    while ((ssize_t) len < ((1 << (wbits - 1)) - 262)) {
        wbits--;
        memlevel--;
    }

    /*
     * This is a formula from deflateBound() for conservative upper bound of
     * compressed data plus 18 bytes of gzip wrapper.
     */

    size = len + ((len + 7) >> 3) + ((len + 63) >> 6) + 5 + 18;

    ngx_memzero(&zstream, sizeof(z_stream));

    pool = ngx_create_pool(256, log);
    if (pool == NULL) {
        /* simulate successful logging */
        return len;
    }

    pool->log = log;

    zstream.zalloc = ngx_http_tskvlog_gzip_alloc;
    zstream.zfree = ngx_http_tskvlog_gzip_free;
    zstream.opaque = pool;

    out = ngx_pnalloc(pool, size);
    if (out == NULL) {
        goto done;
    }

    zstream.next_in = buf;
    zstream.avail_in = len;
    zstream.next_out = out;
    zstream.avail_out = size;

    rc = deflateInit2(&zstream, (int) level, Z_DEFLATED, wbits + 16, memlevel,
                      Z_DEFAULT_STRATEGY);

    if (rc != Z_OK) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "deflateInit2() failed: %d", rc);
        goto done;
    }

    ngx_log_debug4(NGX_LOG_DEBUG_HTTP, log, 0,
                   "deflate in: ni:%p no:%p ai:%ud ao:%ud",
                   zstream.next_in, zstream.next_out,
                   zstream.avail_in, zstream.avail_out);

    rc = deflate(&zstream, Z_FINISH);

    if (rc != Z_STREAM_END) {
        ngx_log_error(NGX_LOG_ALERT, log, 0,
                      "deflate(Z_FINISH) failed: %d", rc);
        goto done;
    }

    ngx_log_debug5(NGX_LOG_DEBUG_HTTP, log, 0,
                   "deflate out: ni:%p no:%p ai:%ud ao:%ud rc:%d",
                   zstream.next_in, zstream.next_out,
                   zstream.avail_in, zstream.avail_out,
                   rc);

    size -= zstream.avail_out;

    rc = deflateEnd(&zstream);

    if (rc != Z_OK) {
        ngx_log_error(NGX_LOG_ALERT, log, 0, "deflateEnd() failed: %d", rc);
        goto done;
    }

    n = ngx_write_fd(fd, out, size);

    if (n != (ssize_t) size) {
        err = (n == -1) ? ngx_errno : 0;

        ngx_destroy_pool(pool);

        ngx_set_errno(err);
        return -1;
    }

done:

    ngx_destroy_pool(pool);

    /* simulate successful logging */
    return len;
}


static void *
ngx_http_tskvlog_gzip_alloc(void *opaque, u_int items, u_int size)
{
    ngx_pool_t *pool = opaque;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pool->log, 0,
                   "gzip alloc: n:%ud s:%ud", items, size);

    return ngx_palloc(pool, items * size);
}


static void
ngx_http_tskvlog_gzip_free(void *opaque, void *address)
{
#if 0
    ngx_pool_t *pool = opaque;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pool->log, 0, "gzip free: %p", address);
#endif
}

#endif


static void
ngx_http_tskvlog_flush(ngx_open_file_t *file, ngx_log_t *log)
{
    size_t               len;
    ssize_t              n;
    ngx_http_tskvlog_buf_t  *buffer;

    buffer = file->data;

    len = buffer->pos - buffer->start;

    if (len == 0) {
        return;
    }

#if (NGX_ZLIB)
    if (buffer->gzip) {
        n = ngx_http_tskvlog_gzip(file->fd, buffer->start, len, buffer->gzip, log);
    } else {
        n = ngx_write_fd(file->fd, buffer->start, len);
    }
#else
    n = ngx_write_fd(file->fd, buffer->start, len);
#endif

    if (n == -1) {
        ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                      ngx_write_fd_n " to \"%s\" failed",
                      file->name.data);

    } else if ((size_t) n != len) {
        ngx_log_error(NGX_LOG_ALERT, log, 0,
                      ngx_write_fd_n " to \"%s\" was incomplete: %z of %uz",
                      file->name.data, n, len);
    }

    buffer->pos = buffer->start;

    if (buffer->event && buffer->event->timer_set) {
        ngx_del_timer(buffer->event);
    }
}


static void
ngx_http_tskvlog_flush_handler(ngx_event_t *ev)
{
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "http log buffer flush handler");

    ngx_http_tskvlog_flush(ev->data, ev->log);
}


static u_char *
ngx_http_tskvlog_time(ngx_http_request_t *r, u_char *buf, ngx_http_tskvlog_op_t *op)
{
    return ngx_cpymem(buf, ngx_cached_http_log_iso8601.data, TIME_LEN);
}

static u_char *
ngx_http_tskvlog_timezone(ngx_http_request_t *r, u_char *buf, ngx_http_tskvlog_op_t *op)
{
    return ngx_cpymem(buf, ngx_cached_http_log_time.data + TIMEZONE_OFF,
                      sizeof("+0600") - 1);
}

static u_char *
ngx_http_tskvlog_unixtime(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op)
{
    ngx_time_t      *tp;

    tp = ngx_timeofday();

    return ngx_sprintf(buf, "%T", tp->sec);
}

static u_char *
ngx_http_tskvlog_request_time(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op)
{
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = ngx_max(ms, 0);

    return ngx_sprintf(buf, "%T.%03M", (time_t) ms / 1000, ms % 1000);
}


static u_char *
ngx_http_tskvlog_status(ngx_http_request_t *r, u_char *buf, ngx_http_tskvlog_op_t *op)
{
    ngx_uint_t  status;

    if (r->err_status) {
        status = r->err_status;

    } else if (r->headers_out.status) {
        status = r->headers_out.status;

    } else if (r->http_version == NGX_HTTP_VERSION_9) {
        status = 9;

    } else {
        status = 0;
    }

    return ngx_sprintf(buf, "%03ui", status);
}


static u_char *
ngx_http_tskvlog_bytes_sent(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op)
{
    return ngx_sprintf(buf, "%O", r->connection->sent);
}


/*
 * although there is a real $body_bytes_sent variable,
 * this log operation code function is more optimized for logging
 */

static u_char *
ngx_http_tskvlog_body_bytes_sent(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op)
{
    off_t  length;

    length = r->connection->sent - r->header_size;

    if (length > 0) {
        return ngx_sprintf(buf, "%O", length);
    }

    *buf = '0';

    return buf + 1;
}


static u_char *
ngx_http_tskvlog_request_length(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op)
{
    return ngx_sprintf(buf, "%O", r->request_length);
}


static u_char *
ngx_http_tskvlog_copy_long(ngx_http_request_t *r, u_char *buf,
    ngx_http_tskvlog_op_t *op)
{
    return ngx_cpymem(buf, (u_char *) op->data, op->len);
}


static ngx_int_t
ngx_http_tskvlog_variable_compile(ngx_conf_t *cf, ngx_http_tskvlog_op_t *op,
    ngx_str_t *name, ngx_str_t *value)
{
    ngx_int_t  index;

    index = ngx_http_get_variable_index(cf, value);
    if (index == NGX_ERROR) {
        return NGX_ERROR;
    }

    op->len = 0;
    op->getlen = ngx_http_tskvlog_variable_getlen;
    op->run = ngx_http_tskvlog_variable;
    op->data = index;
    op->name = *name;

    return NGX_OK;
}


static size_t
ngx_http_tskvlog_variable_getlen(ngx_http_request_t *r, uintptr_t data)
{
    uintptr_t                   len;
    ngx_http_variable_value_t  *value;

    value = ngx_http_get_indexed_variable(r, data);

    if (value == NULL || value->not_found) {
        return 1;
    }

    len = ngx_http_tskvlog_escape(NULL, value->data, value->len);

    value->escape = len ? 1 : 0;

    return value->len + len;
}


static u_char *
ngx_http_tskvlog_variable(ngx_http_request_t *r, u_char *buf, ngx_http_tskvlog_op_t *op)
{
    ngx_http_variable_value_t  *value;

    value = ngx_http_get_indexed_variable(r, op->data);

    if (value == NULL || value->not_found) {
        *buf = '-';
        return buf + 1;
    }

    if (value->escape == 0) {
        return ngx_cpymem(buf, value->data, value->len);

    } else {
        return (u_char *) ngx_http_tskvlog_escape(buf, value->data, value->len);
    }
}


static uintptr_t
ngx_http_tskvlog_escape(u_char *dst, u_char *src, size_t size)
{
    ngx_uint_t      n;

    static uint32_t   escape[] = {
        0x00002601, /* 0000 0000 0000 0000  0010 0110 0000 0001 */

                    /* ?>=< ;:98 7654 3210  /.-, +*)( '&%$ #"!  */
        0x00000004, /* 0000 0000 0000 0000  0000 0000 0000 0100 */

                    /* _^]\ [ZYX WVUT SRQP  ONML KJIH GFED CBA@ */
        0x10000000, /* 0001 0000 0000 0000  0000 0000 0000 0000 */

                    /*  ~}| {zyx wvut srqp  onml kjih gfed cba` */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */

        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
        0x00000000, /* 0000 0000 0000 0000  0000 0000 0000 0000 */
    };


    if (dst == NULL) {

        /* find the number of the characters to be escaped */

        n = 0;

        while (size) {
            if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
                n++;
            }
            src++;
            size--;
        }

        return (uintptr_t) n;
    }

    while (size) {
        if (escape[*src >> 5] & (1 << (*src & 0x1f))) {
            *dst++ = '\\';

            switch (*src) {
                case '\0':
                    *dst++ = '0';
                    break;
                case '\t':
                    *dst++ = 't';
                    break;
                case '\n':
                    *dst++ = 'n';
                    break;
                case '\r':
                    *dst++ = 'r';
                    break;
                case '\"':
                    *dst++ = '"';
                    break;
                case '\\':
                    *dst++ = '\\';
                    break;
            }
            src++;
        } else {
            *dst++ = *src++;
        }
        size--;
    }

    return (uintptr_t) dst;
}


static void *
ngx_http_tskvlog_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_tskvlog_main_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tskvlog_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    if (ngx_array_init(&conf->formats, cf->pool, 4, sizeof(ngx_http_tskvlog_fmt_t))
        != NGX_OK)
    {
        return NULL;
    }

    return conf;
}


static void *
ngx_http_tskvlog_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_tskvlog_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tskvlog_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    return conf;
}


static char *
ngx_http_tskvlog_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_tskvlog_loc_conf_t *prev = parent;
    ngx_http_tskvlog_loc_conf_t *conf = child;

    if (conf->logs || conf->off) {
        return NGX_CONF_OK;
    }

    conf->logs = prev->logs;
    conf->off = prev->off;

    return NGX_CONF_OK;
}


static char *
ngx_http_tskvlog_set_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_tskvlog_loc_conf_t *llcf = conf;

    ssize_t                            size;
    ngx_int_t                          gzip;
    ngx_uint_t                         i;
    ngx_msec_t                         flush;
    ngx_str_t                         *value, name, s;
    ngx_http_tskvlog_t                *log;
    ngx_http_tskvlog_buf_t            *buffer;
    ngx_http_tskvlog_fmt_t            *fmt;
    ngx_http_tskvlog_main_conf_t      *lmcf;
    ngx_syslog_peer_t                 *peer;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (value[1].len == 3 && ngx_strncmp(value[1].data, "off", 3) == 0) {
        llcf->off = 1;
        if (cf->args->nelts == 2) {
            return NGX_CONF_OK;
        }

        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[2]);
        return NGX_CONF_ERROR;
    }

    if (cf->args->nelts < 3) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid number of arguments in \"tskv_log\" directive");
        return NGX_CONF_ERROR;
    }

    if (llcf->logs == NULL) {
        llcf->logs = ngx_array_create(cf->pool, 2, sizeof(ngx_http_tskvlog_t));
        if (llcf->logs == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    lmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_tskvlog_module);

    log = ngx_array_push(llcf->logs);
    if (log == NULL) {
        return NGX_CONF_ERROR;
    }

    ngx_memzero(log, sizeof(ngx_http_tskvlog_t));

    if (ngx_strncmp(value[1].data, "syslog:", 7) == 0) {
        peer = ngx_pcalloc(cf->pool, sizeof(ngx_syslog_peer_t));
        if (peer == NULL) {
            return NGX_CONF_ERROR;
        }

        if (ngx_syslog_process_conf(cf, peer) != NGX_CONF_OK) {
            return NGX_CONF_ERROR;
        }

        log->syslog_peer = peer;
    } else {
        log->file = ngx_conf_open_file(cf->cycle, &value[1]);
        if (log->file == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    name = value[2];

    fmt = lmcf->formats.elts;
    for (i = 0; i < lmcf->formats.nelts; i++) {
        if (fmt[i].name.len == name.len
            && ngx_strcasecmp(fmt[i].name.data, name.data) == 0)
        {
            log->format = &fmt[i];
            break;
        }
    }

    if (log->format == NULL) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "unknown log format \"%V\"", &name);
        return NGX_CONF_ERROR;
    }

    size = 0;
    flush = 0;
    gzip = 0;

    for (i = 3; i < cf->args->nelts; i++) {

        if (ngx_strncmp(value[i].data, "buffer=", 7) == 0) {
            s.len = value[i].len - 7;
            s.data = value[i].data + 7;

            size = ngx_parse_size(&s);

            if (size == NGX_ERROR || size == 0) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid buffer size \"%V\"", &s);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "flush=", 6) == 0) {
            s.len = value[i].len - 6;
            s.data = value[i].data + 6;

            flush = ngx_parse_time(&s, 0);

            if (flush == (ngx_msec_t) NGX_ERROR || flush == 0) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid flush time \"%V\"", &s);
                return NGX_CONF_ERROR;
            }

            continue;
        }

        if (ngx_strncmp(value[i].data, "gzip", 4) == 0
            && (value[i].len == 4 || value[i].data[4] == '='))
        {
#if (NGX_ZLIB)
            if (size == 0) {
                size = 64 * 1024;
            }

            if (value[i].len == 4) {
                gzip = Z_BEST_SPEED;
                continue;
            }

            s.len = value[i].len - 5;
            s.data = value[i].data + 5;

            gzip = ngx_atoi(s.data, s.len);

            if (gzip < 1 || gzip > 9) {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "invalid compression level \"%V\"", &s);
                return NGX_CONF_ERROR;
            }

            continue;

#else
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "nginx was built without zlib support");
            return NGX_CONF_ERROR;
#endif
        }

        if (ngx_strncmp(value[i].data, "if=", 3) == 0) {
            s.len = value[i].len - 3;
            s.data = value[i].data + 3;

            ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

            ccv.cf = cf;
            ccv.value = &s;
            ccv.complex_value = ngx_palloc(cf->pool,
                                           sizeof(ngx_http_complex_value_t));
            if (ccv.complex_value == NULL) {
                return NGX_CONF_ERROR;
            }

            if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
                return NGX_CONF_ERROR;
            }

            log->filter = ccv.complex_value;

            continue;
        }


        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid parameter \"%V\"", &value[i]);
        return NGX_CONF_ERROR;
    }

    if (flush && size == 0) {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "no buffer is defined for access_log \"%V\"",
                           &value[1]);
        return NGX_CONF_ERROR;
    }

    if (size) {

        if (log->syslog_peer) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "logs to syslog cannot be buffered");
            return NGX_CONF_ERROR;
        }

        if (log->file->data) {
            buffer = log->file->data;

            if (buffer->last - buffer->start != size
                || buffer->flush != flush
                || buffer->gzip != gzip)
            {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "tskv_log \"%V\" already defined "
                                   "with conflicting parameters",
                                   &value[1]);
                return NGX_CONF_ERROR;
            }

            return NGX_CONF_OK;
        }

        buffer = ngx_pcalloc(cf->pool, sizeof(ngx_http_tskvlog_buf_t));
        if (buffer == NULL) {
            return NGX_CONF_ERROR;
        }

        buffer->start = ngx_pnalloc(cf->pool, size);
        if (buffer->start == NULL) {
            return NGX_CONF_ERROR;
        }

        buffer->pos = buffer->start;
        buffer->last = buffer->start + size;

        if (flush) {
            buffer->event = ngx_pcalloc(cf->pool, sizeof(ngx_event_t));
            if (buffer->event == NULL) {
                return NGX_CONF_ERROR;
            }

            buffer->event->data = log->file;
            buffer->event->handler = ngx_http_tskvlog_flush_handler;
            buffer->event->log = &cf->cycle->new_log;

            buffer->flush = flush;
        }

        buffer->gzip = gzip;

        log->file->flush = ngx_http_tskvlog_flush;
        log->file->data = buffer;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_tskvlog_set_format(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_tskvlog_main_conf_t *lmcf = conf;

    ngx_str_t               *value;
    ngx_uint_t               i;
    ngx_http_tskvlog_fmt_t  *fmt;

    if (cf->cmd_type != NGX_HTTP_MAIN_CONF) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "the \"tskv_log_format\" directive may be used "
                           "only on \"http\" level");
    }

    value = cf->args->elts;

    fmt = lmcf->formats.elts;
    for (i = 0; i < lmcf->formats.nelts; i++) {
        if (fmt[i].name.len == value[1].len
            && ngx_strcmp(fmt[i].name.data, value[1].data) == 0)
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "duplicate \"tskv_log_format\" name \"%V\"",
                               &value[1]);
            return NGX_CONF_ERROR;
        }
    }

    fmt = ngx_array_push(&lmcf->formats);
    if (fmt == NULL) {
        return NGX_CONF_ERROR;
    }

    fmt->name = value[1];

    fmt->flushes = ngx_array_create(cf->pool, 4, sizeof(ngx_int_t));
    if (fmt->flushes == NULL) {
        return NGX_CONF_ERROR;
    }

    fmt->ops = ngx_array_create(cf->pool, 16, sizeof(ngx_http_tskvlog_op_t));
    if (fmt->ops == NULL) {
        return NGX_CONF_ERROR;
    }

    return ngx_http_tskvlog_compile_format(cf, fmt, cf->args, 2);
}

static ngx_int_t
ngx_http_tskvlog_add_var(ngx_conf_t *cf, ngx_array_t *ops, ngx_str_t *name,
    ngx_str_t *var, ngx_array_t *flushes)
{
    ngx_int_t               *flush;
    ngx_uint_t               i;
    ngx_http_tskvlog_op_t   *op;
    ngx_http_tskvlog_var_t  *v;

    /* Check for duplicate elements */
    op = ops->elts;
    for (i = 0; i < ops->nelts; i++) {
        if (op[i].name.len == name->len &&
            ngx_strncmp(op[i].name.data, name->data, name->len) == 0) {

            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "duplicate variable \"%V\"", name);
            return NGX_ERROR;
        }
    }

    /* Insert new element */
    op = ngx_array_push(ops);
    if (op == NULL) {
        return NGX_ERROR;
    }

    for (v = ngx_http_tskvlog_vars; v->name.len; v++) {

        if (v->name.len == var->len
            && ngx_strncmp(v->name.data, var->data, var->len) == 0)
        {
            op->len = v->len;
            op->getlen = NULL;
            op->run = v->run;
            op->data = 0;
            op->name = *name;

            return NGX_OK;
        }
    }

    if (ngx_http_tskvlog_variable_compile(cf, op, name, var) != NGX_OK) {
        return NGX_ERROR;
    }

    if (flushes) {

        flush = ngx_array_push(flushes);
        if (flush == NULL) {
            return NGX_ERROR;
        }

        *flush = op->data; /* variable index */
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_tskvlog_add_const(ngx_conf_t *cf, ngx_array_t *ops, ngx_str_t *name,
    ngx_str_t *val)
{
    ngx_uint_t               i;
    ngx_http_tskvlog_op_t   *op;

    /* Check for duplicate elements */
    op = ops->elts;
    for (i = 0; i < ops->nelts; i++) {
        if (op[i].name.len == name->len &&
            ngx_strncmp(op[i].name.data, name->data, name->len) == 0) {

            ngx_conf_log_error(NGX_LOG_ERR, cf, 0, "duplicate variable \"%V\"", name);
            return NGX_ERROR;
        }
    }

    /* Insert new element */
    op = ngx_array_push(ops);
    if (op == NULL) {
        return NGX_ERROR;
    }

    /*
    p = ngx_pnalloc(cf->pool, val->len);
    if (p == NULL) {
        return NGX_CONF_ERROR;
    }
    ngx_memcpy(p, val->data, val->len);
    */

    op->len = val->len;
    op->getlen = NULL;
    op->run = ngx_http_tskvlog_copy_long;
    op->data = (uintptr_t)(val->data);
    op->name = *name;

    return NGX_OK;
}

static char *
ngx_http_tskvlog_compile_format(ngx_conf_t *cf, ngx_http_tskvlog_fmt_t *fmt,
    ngx_array_t *args, ngx_uint_t s)
{
    u_char                          *data, *p, ch;
    int                              is_variable;
    size_t                           i;
    ngx_uint_t                       put_default_vars;
    ngx_str_t                       *value, var, name;
    ngx_http_tskvlog_op_t           *op;
    ngx_http_tskvlog_default_var_t  *dfv;

    /* Push tskv format name */
    p = ngx_pnalloc(cf->pool, fmt->name.len);
    if (p == NULL) {
        return NGX_CONF_ERROR;
    }
    ngx_memcpy(p, fmt->name.data, fmt->name.len);

    op = ngx_array_push(fmt->ops);
    if (op == NULL) {
        return NGX_CONF_ERROR;
    }

    op->len = fmt->name.len;
    op->getlen = NULL;
    op->run = ngx_http_tskvlog_copy_long;
    op->data = (uintptr_t)p;
    op->name = ngx_http_tskv_format;

    if (args == NULL) {
        return NGX_CONF_OK;
    }

    put_default_vars = 1;
    value = args->elts;

    if (s < args->nelts && ngx_strncmp(value[s].data, "default_vars=", 13) == 0) {
        data = value[s].data + 13;

        if (ngx_strcmp(data, "no") == 0) {
            put_default_vars = 0;
        } else if (ngx_strcmp(data, "yes") == 0) {
            put_default_vars = 1;
        } else if (ngx_strcmp(data, "default") != 0) {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "unknown default_vars value \"%s\"", data);
            return NGX_CONF_ERROR;
        }

        s++;
    }

    /* Push default fields:
        cookies
        method - GET/POST/HEAD/etc...
        protocol - HTTP/1.1
        referer
        request
        status - 200
        user_agent
    */

    if (put_default_vars == 1) {
       for (dfv = ngx_http_tskvlog_default_vars; dfv->name.len; dfv++) {
           if (ngx_http_tskvlog_add_var(cf, fmt->ops, &dfv->name, &dfv->var, fmt->flushes) != NGX_OK) {
               return NGX_CONF_ERROR;
           }
       }
    }

    for ( /* void */ ; s < args->nelts; s++) {

        i = 0;

        while (i < value[s].len) {

            is_variable = 0;
            name.len = 0;
            data = &value[s].data[i];

            /* Skip whitespaces */
            while (i < value[s].len && value[s].data[i] == ' ') {
                i++;
            }

            if (value[s].data[i] != '$') {
                name.data = &value[s].data[i];
                for (name.len = 0; i < value[s].len; i++, name.len++) {
                    ch = value[s].data[i];

                    if ((ch >= 'A' && ch <= 'Z')
                        || (ch >= 'a' && ch <= 'z')
                        || (ch >= '0' && ch <= '9')
                        || ch == '_')
                    {
                        continue;
                    }

                    break;
                }

                if (name.len == 0) {
                    goto invalid;
                }

                if (value[s].data[i] != '=') {
                    goto invalid;
                }
                i++;
            }

            if (value[s].data[i] == '$') {

                if (++i == value[s].len) {
                    goto invalid;
                }

                is_variable = 1;
            }

            var.data = &value[s].data[i];

            for (var.len = 0; i < value[s].len; i++, var.len++) {
                ch = value[s].data[i];

                if ((ch >= 'A' && ch <= 'Z')
                    || (ch >= 'a' && ch <= 'z')
                    || (ch >= '0' && ch <= '9')
                    || ch == '_')
                {
                    continue;
                }

                break;
            }

            if (var.len == 0) {
                goto invalid;
            }

            if (is_variable) {

                /* Use variable name by default */
                if (name.len == 0) {
                    name = var;
                }

                if (ngx_http_tskvlog_add_var(cf, fmt->ops, &name, &var, fmt->flushes) != NGX_OK) {
                    return NGX_CONF_ERROR;
                }
            } else {

                if (name.len == 0) {
                    return NGX_CONF_ERROR;
                }

                if (ngx_http_tskvlog_add_const(cf, fmt->ops, &name, &var) != NGX_OK) {
                    return NGX_CONF_ERROR;
                }
            }
        }
    }

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%s\"", data);

    return NGX_CONF_ERROR;
}


static ngx_int_t
ngx_http_tskvlog_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_tskvlog_handler;

    return NGX_OK;
}
