
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
#include <json-c/json.h>

/* ISO8601 timestamp = "1970-09-28T12:00:00+06:00" */
/* HTTP Time format = "Mon, 28 Sep 1970 06:00:00 GMT" */
#define TIME_LEN  (sizeof("1970-09-28T12:00:00") - 1)
#define UTC_TIME_LEN  (sizeof("1970-09-28T12:00:00Z") - 1)
#define TIMEZONE_OFF  sizeof("28/Sep/1970:12:00:00")

typedef struct ngx_http_json_log_op_s  ngx_http_json_log_op_t;

typedef struct json_object * (*ngx_http_json_log_op_run_pt) (ngx_http_request_t *r,
    ngx_http_json_log_op_t *op);


struct ngx_http_json_log_op_s {
    size_t                      len;
    ngx_http_json_log_op_run_pt      run;
    uintptr_t                   data;
    ngx_str_t                   name;
};


typedef struct {
    ngx_str_t                   name;
    ngx_array_t                *flushes;
    ngx_array_t                *ops;        /* array of ngx_http_json_log_op_t */
} ngx_http_json_log_fmt_t;


typedef struct {
    ngx_array_t                 formats;    /* array of ngx_http_json_log_fmt_t */
} ngx_http_json_log_main_conf_t;


typedef struct {
    u_char                     *start;
    u_char                     *pos;
    u_char                     *last;

    ngx_event_t                *event;
    ngx_msec_t                  flush;
    ngx_int_t                   gzip;
} ngx_http_json_log_buf_t;


typedef struct {
    ngx_open_file_t            *file;
    time_t                      disk_full_time;
    time_t                      error_log_time;
    ngx_http_json_log_fmt_t     *format;
} ngx_http_json_log_t;


typedef struct {
    ngx_http_json_log_t         *log;
} ngx_http_json_log_loc_conf_t;


typedef struct {
    ngx_str_t                   name;
    ngx_http_json_log_op_run_pt      run;
} ngx_http_json_log_var_t;

typedef struct {
    ngx_str_t                   name;
    ngx_str_t                   var;
} ngx_http_json_log_default_var_t;


static void ngx_http_json_log_write(ngx_http_request_t *r, ngx_http_json_log_t *log,
    u_char *buf, size_t len);

#if (NGX_ZLIB)
static ssize_t ngx_http_json_log_gzip(ngx_fd_t fd, u_char *buf, size_t len,
    ngx_int_t level, ngx_log_t *log);

static void *ngx_http_json_log_gzip_alloc(void *opaque, u_int items, u_int size);
static void ngx_http_json_log_gzip_free(void *opaque, void *address);
#endif

static void ngx_http_json_log_flush(ngx_open_file_t *file, ngx_log_t *log);
static void ngx_http_json_log_flush_handler(ngx_event_t *ev);

static struct json_object *ngx_http_json_log_time(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_utc_time(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_timezone(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_request_time(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_status(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_bytes_sent(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_body_bytes_sent(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static struct json_object *ngx_http_json_log_request_length(ngx_http_request_t *r, ngx_http_json_log_op_t *op);

static ngx_int_t ngx_http_json_log_add_var(ngx_conf_t *cf, ngx_array_t *ops, ngx_str_t *name,
    ngx_str_t *var, ngx_array_t *flushes);
static ngx_int_t ngx_http_json_log_variable_compile(ngx_conf_t *cf,
    ngx_http_json_log_op_t *op, ngx_str_t *name, ngx_str_t *value);
static struct json_object *ngx_http_json_log_variable(ngx_http_request_t *r, ngx_http_json_log_op_t *op);
static uintptr_t ngx_http_json_log_escape(u_char *dst, u_char *src, size_t size);


static void *ngx_http_json_log_create_main_conf(ngx_conf_t *cf);
static void *ngx_http_json_log_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_json_log_merge_loc_conf(ngx_conf_t *cf, void *parent,
    void *child);
static char *ngx_http_json_log_set_log(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_json_log_set_format(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static char *ngx_http_json_log_compile_format(ngx_conf_t *cf,
    ngx_http_json_log_fmt_t *fmt, ngx_array_t *args, ngx_uint_t s);
static ngx_int_t ngx_http_json_log_init(ngx_conf_t *cf);


static ngx_command_t  ngx_http_json_log_commands[] = {

    { ngx_string("json_log_format"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_2MORE,
      ngx_http_json_log_set_format,
      NGX_HTTP_MAIN_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("json_log"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LIF_CONF
                        |NGX_HTTP_LMT_CONF|NGX_CONF_2MORE,
      ngx_http_json_log_set_log,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_http_module_t  ngx_http_json_log_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_json_log_init,                     /* postconfiguration */

    ngx_http_json_log_create_main_conf,         /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    ngx_http_json_log_create_loc_conf,          /* create location configuration */
    ngx_http_json_log_merge_loc_conf            /* merge location configuration */
};


ngx_module_t  ngx_http_json_log_module = {
    NGX_MODULE_V1,
    &ngx_http_json_log_module_ctx,              /* module context */
    ngx_http_json_log_commands,                 /* module directives */
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


static ngx_http_json_log_var_t  ngx_http_json_log_vars[] = {
    { ngx_string("timestamp"), ngx_http_json_log_time },
    { ngx_string("utctimestamp"), ngx_http_json_log_utc_time },
    { ngx_string("timezone"), ngx_http_json_log_timezone },
    { ngx_string("request_time"), ngx_http_json_log_request_time },
    { ngx_string("status"), ngx_http_json_log_status },
    { ngx_string("bytes_sent"), ngx_http_json_log_bytes_sent },
    { ngx_string("body_bytes_sent"), ngx_http_json_log_body_bytes_sent },
    { ngx_string("request_length"), ngx_http_json_log_request_length },

    { ngx_null_string, NULL }
};

static ngx_int_t
ngx_http_json_log_handler(ngx_http_request_t *r)
{
#define MAX_KEY_LENGTH             1024
    char                           key_buff[MAX_KEY_LENGTH+1];
    char                          *json_line;
    u_char                   *line, *p;
    size_t                    len, json_line_len;
    ngx_uint_t                i;
    json_object                   *json_obj;
    ngx_http_json_log_t           *log;
    ngx_http_json_log_op_t        *op;
    ngx_http_json_log_buf_t       *buffer;
    ngx_http_json_log_loc_conf_t  *lcf;

    lcf = ngx_http_get_module_loc_conf(r, ngx_http_json_log_module);

    log = lcf->log;
    if (log == NULL) {
        return NGX_OK;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http json got log");


    if (ngx_time() == log->disk_full_time) {

        /*
         * on FreeBSD writing to a full filesystem with enabled softupdates
         * may block process for much longer time than writing to non-full
         * filesystem, so we skip writing to a log for one second
         */

        return NGX_OK;
    }

    ngx_http_script_flush_no_cacheable_variables(r, log->format->flushes);

    json_obj = json_object_new_object();
    if (json_obj == NULL) {
        return NGX_ERROR;
    }

    op = log->format->ops->elts;
    for (i = 0; i < log->format->ops->nelts; i++) {
        ngx_cpystrn((u_char *)key_buff, op[i].name.data, ngx_min(op[i].name.len+1, MAX_KEY_LENGTH));

        json_object_object_add(json_obj, key_buff, op[i].run(r, &op[i]));
    }

    json_line = (char*)json_object_to_json_string(json_obj);
    if (json_line == NULL) {
        goto err_out_free;
    }

    json_line_len = ngx_strlen(json_line);
    len = json_line_len + NGX_LINEFEED_SIZE;

    buffer = log->file ? log->file->data : NULL;

    if (buffer) {

        if (len > (size_t) (buffer->last - buffer->pos)) {

            ngx_http_json_log_write(r, log, buffer->start,
                               buffer->pos - buffer->start);

            buffer->pos = buffer->start;
        }

        if (len <= (size_t) (buffer->last - buffer->pos)) {

            p = buffer->pos;

            if (buffer->event && p == buffer->start) {
                ngx_add_timer(buffer->event, buffer->flush);
            }

            p = ngx_cpymem(p, json_line, json_line_len);
            json_object_put(json_obj);

            ngx_linefeed(p);

            buffer->pos = p;

            return NGX_OK;
        }

        if (buffer->event && buffer->event->timer_set) {
            ngx_del_timer(buffer->event);
        }
    }

    line = ngx_pnalloc(r->pool, len + NGX_LINEFEED_SIZE);
    if (line == NULL) {
        goto err_out_free;
    }

    p = ngx_cpymem(line, json_line, json_line_len);
    json_object_put(json_obj);

    ngx_linefeed(p);

    ngx_http_json_log_write(r, log, line, p - line);


    return NGX_OK;

err_out_free:
    json_object_put(json_obj);
    return NGX_ERROR;
}


static void
ngx_http_json_log_write(ngx_http_request_t *r, ngx_http_json_log_t *log, u_char *buf,
    size_t len)
{
    u_char              *name;
    time_t               now;
    ssize_t              n;
    ngx_err_t            err;
#if (NGX_ZLIB)
    ngx_http_json_log_buf_t  *buffer;
#endif

    name = log->file->name.data;

#if (NGX_ZLIB)
    buffer = log->file->data;

    if (buffer && buffer->gzip) {
        n = ngx_http_json_log_gzip(log->file->fd, buf, len, buffer->gzip,
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
ngx_http_json_log_gzip(ngx_fd_t fd, u_char *buf, size_t len, ngx_int_t level,
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

    zstream.zalloc = ngx_http_json_log_gzip_alloc;
    zstream.zfree = ngx_http_json_log_gzip_free;
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
ngx_http_json_log_gzip_alloc(void *opaque, u_int items, u_int size)
{
    ngx_pool_t *pool = opaque;

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, pool->log, 0,
                   "gzip alloc: n:%ud s:%ud", items, size);

    return ngx_palloc(pool, items * size);
}


static void
ngx_http_json_log_gzip_free(void *opaque, void *address)
{
#if 0
    ngx_pool_t *pool = opaque;

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, pool->log, 0, "gzip free: %p", address);
#endif
}

#endif


static void
ngx_http_json_log_flush(ngx_open_file_t *file, ngx_log_t *log)
{
    size_t               len;
    ssize_t              n;
    ngx_http_json_log_buf_t  *buffer;

    buffer = file->data;

    len = buffer->pos - buffer->start;

    if (len == 0) {
        return;
    }

#if (NGX_ZLIB)
    if (buffer->gzip) {
        n = ngx_http_json_log_gzip(file->fd, buffer->start, len, buffer->gzip, log);
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
ngx_http_json_log_flush_handler(ngx_event_t *ev)
{
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ev->log, 0,
                   "http log buffer flush handler");

    ngx_http_json_log_flush(ev->data, ev->log);
}


static struct json_object *
ngx_http_json_log_time(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    return json_object_new_string_len((char *)ngx_cached_http_log_iso8601.data, TIME_LEN);
}

static struct json_object *
ngx_http_json_log_utc_time(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    u_char buf[UTC_TIME_LEN + 1];
    u_char *p;

    p = ngx_cpymem(buf, ngx_cached_http_log_iso8601.data, sizeof("1970-09-28T") - 1);
    p = ngx_cpymem(p, ngx_cached_http_time.data + sizeof("Mon, 28 Sep 1970 ") - 1, sizeof("06:00:00") - 1);
    *p = 'Z';

    return json_object_new_string_len((char *)buf, UTC_TIME_LEN);
}

static struct json_object *
ngx_http_json_log_timezone(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    return json_object_new_string_len((char *)ngx_cached_http_log_time.data + TIMEZONE_OFF, sizeof("+0600") - 1);
}

static struct json_object *
ngx_http_json_log_request_time(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    ngx_time_t      *tp;
    ngx_msec_int_t   ms;

    tp = ngx_timeofday();

    ms = (ngx_msec_int_t)
             ((tp->sec - r->start_sec) * 1000 + (tp->msec - r->start_msec));
    ms = ngx_max(ms, 0);

    return json_object_new_double((double)ms / 1000);
}


static struct json_object *
ngx_http_json_log_status(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
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

    return json_object_new_int(status);
}


static struct json_object *
ngx_http_json_log_bytes_sent(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    return json_object_new_int64(r->connection->sent);
}


/*
 * although there is a real $body_bytes_sent variable,
 * this log operation code function is more optimized for logging
 */

static struct json_object *
ngx_http_json_log_body_bytes_sent(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    return json_object_new_int64(r->connection->sent - r->header_size);
}


static struct json_object *
ngx_http_json_log_request_length(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    return json_object_new_int64(r->request_length);
}


static ngx_int_t
ngx_http_json_log_variable_compile(ngx_conf_t *cf, ngx_http_json_log_op_t *op,
    ngx_str_t *name, ngx_str_t *value)
{
    ngx_int_t  index;

    index = ngx_http_get_variable_index(cf, value);
    if (index == NGX_ERROR) {
        return NGX_ERROR;
    }

    op->run = ngx_http_json_log_variable;
    op->data = index;
    op->name = *name;

    return NGX_OK;
}


static struct json_object *
ngx_http_json_log_variable(ngx_http_request_t *r, ngx_http_json_log_op_t *op)
{
    ngx_http_variable_value_t  *value;
    uintptr_t                   len;

    value = ngx_http_get_indexed_variable(r, op->data);

    if (value == NULL || value->not_found) {
        return NULL;
    }

    len = ngx_http_json_log_escape(NULL, value->data, value->len);
    value->escape = len ? 1 : 0;
    return json_object_new_string_len((char *)value->data, value->len);
    /*
    if (value->escape == 0) {
        return ngx_cpymem(buf, value->data, value->len);

    } else {
        return (u_char *) ngx_http_json_log_escape(buf, value->data, value->len);
    }
    */
}


static uintptr_t
ngx_http_json_log_escape(u_char *dst, u_char *src, size_t size)
{
    ngx_uint_t      n;
    static u_char   hex[] = "0123456789ABCDEF";

    static uint32_t   escape[] = {
        0xffffffff, /* 1111 1111 1111 1111  1111 1111 1111 1111 */

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
                switch (*src) {
                    case '\0':
                    case '\t':
                    case '\n':
                    case '\r':
                    case '\"':
                    case '\\':
                        n++;
                        break;
                    default:
                        n += 3;
                }
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
                default:
                    *dst++ = 'x';
                    *dst++ = hex[*src >> 4];
                    *dst++ = hex[*src & 0xf];
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
ngx_http_json_log_create_main_conf(ngx_conf_t *cf)
{
    ngx_http_json_log_main_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_json_log_main_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    if (ngx_array_init(&conf->formats, cf->pool, 4, sizeof(ngx_http_json_log_fmt_t))
        != NGX_OK)
    {
        return NULL;
    }

    return conf;
}


static void *
ngx_http_json_log_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_json_log_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_json_log_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    return conf;
}


static char *
ngx_http_json_log_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_json_log_loc_conf_t *prev = parent;
    ngx_http_json_log_loc_conf_t *conf = child;

    if (conf->log) {
        return NGX_CONF_OK;
    }

    conf->log = prev->log;

    return NGX_CONF_OK;
}


static char *
ngx_http_json_log_set_log(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_json_log_loc_conf_t *llcf = conf;

    ssize_t                     size;
    ngx_int_t                   gzip;
    ngx_uint_t                  i;
    ngx_msec_t                  flush;
    ngx_str_t                  *value, name, s;
    ngx_http_json_log_t             *log;
    ngx_http_json_log_buf_t         *buffer;
    ngx_http_json_log_fmt_t         *fmt;
    ngx_http_json_log_main_conf_t   *lmcf;

    value = cf->args->elts;

    lmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_json_log_module);

    log = llcf->log;
    if (log == NULL) {
        llcf->log = ngx_pcalloc(cf->pool, sizeof(ngx_http_json_log_t));
        if (llcf->log == NULL) {
            return NGX_CONF_ERROR;
        }
        log = llcf->log;
        log->disk_full_time = 0;
        log->error_log_time = 0;
    }

    ngx_memzero(log, sizeof(ngx_http_json_log_t));

    log->file = ngx_conf_open_file(cf->cycle, &value[1]);
    if (log->file == NULL) {
        return NGX_CONF_ERROR;
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

        if (log->file->data) {
            buffer = log->file->data;

            if (buffer->last - buffer->start != size
                || buffer->flush != flush
                || buffer->gzip != gzip)
            {
                ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                                   "access_log \"%V\" already defined "
                                   "with conflicting parameters",
                                   &value[1]);
                return NGX_CONF_ERROR;
            }

            return NGX_CONF_OK;
        }

        buffer = ngx_pcalloc(cf->pool, sizeof(ngx_http_json_log_buf_t));
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
            buffer->event->handler = ngx_http_json_log_flush_handler;
            buffer->event->log = &cf->cycle->new_log;

            buffer->flush = flush;
        }

        buffer->gzip = gzip;

        log->file->flush = ngx_http_json_log_flush;
        log->file->data = buffer;
    }

    return NGX_CONF_OK;
}


static char *
ngx_http_json_log_set_format(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_json_log_main_conf_t *lmcf = conf;

    ngx_str_t           *value;
    ngx_uint_t           i;
    ngx_http_json_log_fmt_t  *fmt;

    if (cf->cmd_type != NGX_HTTP_MAIN_CONF) {
        ngx_conf_log_error(NGX_LOG_WARN, cf, 0,
                           "the \"log_format\" directive may be used "
                           "only on \"http\" level");
    }

    value = cf->args->elts;

    fmt = lmcf->formats.elts;
    for (i = 0; i < lmcf->formats.nelts; i++) {
        if (fmt[i].name.len == value[1].len
            && ngx_strcmp(fmt[i].name.data, value[1].data) == 0)
        {
            ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                               "duplicate \"log_format\" name \"%V\"",
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

    fmt->ops = ngx_array_create(cf->pool, 16, sizeof(ngx_http_json_log_op_t));
    if (fmt->ops == NULL) {
        return NGX_CONF_ERROR;
    }

    return ngx_http_json_log_compile_format(cf, fmt, cf->args, 2);
}

static ngx_int_t
ngx_http_json_log_add_var(ngx_conf_t *cf, ngx_array_t *ops, ngx_str_t *name,
    ngx_str_t *var, ngx_array_t *flushes)
{
    ngx_int_t               *flush;
    ngx_uint_t               i;
    ngx_http_json_log_op_t   *op;
    ngx_http_json_log_var_t  *v;

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

    for (v = ngx_http_json_log_vars; v->name.len; v++) {

        if (v->name.len == var->len
            && ngx_strncmp(v->name.data, var->data, var->len) == 0)
        {
            op->run = v->run;
            op->data = 0;
            op->name = *name;

            return NGX_OK;
        }
    }

    if (ngx_http_json_log_variable_compile(cf, op, name, var) != NGX_OK) {
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

static char *
ngx_http_json_log_compile_format(ngx_conf_t *cf, ngx_http_json_log_fmt_t *fmt,
    ngx_array_t *args, ngx_uint_t s)
{
    u_char              *data, ch;
    size_t               i;
    ngx_str_t           *value, var, name;

    value = args->elts;

    for ( /* void */ ; s < args->nelts; s++) {

        i = 0;

        while (i < value[s].len) {

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
                        || ch == '_'
                        || ch == '@')
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

                /* Use variable name by default */
                if (name.len == 0) {
                    name = var;
                }

                if (ngx_http_json_log_add_var(cf, fmt->ops, &name, &var, fmt->flushes) != NGX_OK) {
                    return NGX_CONF_ERROR;
                }

                continue;
            } else {
                goto invalid;
            }

            i++;
        }
    }

    return NGX_CONF_OK;

invalid:

    ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "invalid parameter \"%s\"", data);

    return NGX_CONF_ERROR;
}


static ngx_int_t
ngx_http_json_log_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_LOG_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_json_log_handler;

    return NGX_OK;
}
