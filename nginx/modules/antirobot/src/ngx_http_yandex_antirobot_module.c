#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#define ANTIROBOT_CTRL_HDR "X-ForwardToUser-Y"

typedef struct {
    ngx_http_complex_value_t *uri;
    ngx_array_t              *vars;
    ngx_array_t              *antirobot_bypass;
#ifdef MORE_OPT_HEADER_ACCESS
    ngx_str_t                *ctrl_header_lower;
    ngx_uint_t                ctrl_header_hash;
#endif
} ngx_http_antirobot_request_conf_t;


typedef struct {
    ngx_uint_t                  done;
    ngx_uint_t                  status;
    ngx_uint_t                  need_forward:1;
    ngx_http_request_t         *subrequest;
    ngx_chain_t                 subrequest_response_body;
    off_t                       subrequest_response_body_len;
    unsigned                    waiting_request_body:1;
    unsigned                    request_body_done:1;
} ngx_http_antirobot_request_ctx_t;


typedef struct {
    ngx_int_t                 index;
    ngx_http_complex_value_t  value;
    ngx_http_set_variable_pt  set_handler;
} ngx_http_antirobot_request_variable_t;


static ngx_int_t ngx_http_antirobot_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_http_antirobot_request_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_antirobot_request_done(ngx_http_request_t *r,
    void *data, ngx_int_t rc);
static ngx_int_t ngx_http_antirobot_request_set_variables(ngx_http_request_t *r,
    ngx_http_antirobot_request_conf_t *arcf, ngx_http_antirobot_request_ctx_t *ctx);
static ngx_int_t ngx_http_antirobot_request_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);
static void *ngx_http_antirobot_request_create_conf(ngx_conf_t *cf);
static char *ngx_http_antirobot_request_merge_conf(ngx_conf_t *cf,
    void *parent, void *child);
static ngx_int_t ngx_http_antirobot_request_init(ngx_conf_t *cf);
static char *ngx_http_antirobot_request_set(ngx_conf_t *cf, ngx_command_t *cmd,
    void *conf);
static void ngx_http_antirobot_post_read_body(ngx_http_request_t *r);
static ngx_int_t
ngx_http_antirobot_status_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_command_t  ngx_http_yandex_antirobot_commands[] = {

    { ngx_string("antirobot_request"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_antirobot_request_conf_t, uri),
      NULL },

    { ngx_string("antirobot_request_set"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE2,
      ngx_http_antirobot_request_set,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },

    { ngx_string("antirobot_bypass"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_1MORE,
      ngx_http_set_predicate_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_antirobot_request_conf_t, antirobot_bypass),
      NULL },

#ifdef MORE_OPT_HEADER_ACCESS
    { ngx_string("antirobot_request_ctrl"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_CONF_TAKE1,
      ngx_http_antirobot_ctrl_header,
      NGX_HTTP_LOC_CONF_OFFSET,
      0,
      NULL },
#endif
      ngx_null_command
};

static ngx_str_t  ngx_http_antirobot_status = ngx_string("antirobot_status");

static ngx_http_module_t  ngx_http_yandex_antirobot_module_ctx = {
    ngx_http_antirobot_add_variables,           /* preconfiguration */
    ngx_http_antirobot_request_init,            /* postconfiguration */

    NULL,                                       /* create main configuration */
    NULL,                                       /* init main configuration */

    NULL,                                       /* create server configuration */
    NULL,                                       /* merge server configuration */

    ngx_http_antirobot_request_create_conf,     /* create location configuration */
    ngx_http_antirobot_request_merge_conf       /* merge location configuration */
};


ngx_module_t  ngx_http_yandex_antirobot_module = {
    NGX_MODULE_V1,
    &ngx_http_yandex_antirobot_module_ctx, /* module context */
    ngx_http_yandex_antirobot_commands,    /* module directives */
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


ngx_int_t
ngx_http_custom_subrequest(ngx_http_request_t *r,
    ngx_str_t *uri, ngx_str_t *args, ngx_http_request_t **psr,
    ngx_http_post_subrequest_t *ps, ngx_http_request_body_t *request_body,
    ngx_uint_t flags)
{
    ngx_time_t                    *tp;
    ngx_connection_t              *c;
    ngx_http_request_t            *sr;
    ngx_http_core_srv_conf_t      *cscf;
    ngx_http_postponed_request_t  *pr, *p;

    if (r->main->subrequests == 0) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                      "antirobot subrequests cycle while processing \"%V\"",
                      uri);
        return NGX_ERROR;
    }

    /*
     * 1 extra request is reserved for other purposes.
     * count is 8-bit unsigned int.
     */
    if (r->main->count >= 254) {
        ngx_log_error(NGX_LOG_CRIT, r->connection->log, 0,
                      "request reference counter overflow "
                      "while processing \"%V\"", uri);
        return NGX_ERROR;
    }

    sr = ngx_pcalloc(r->pool, sizeof(ngx_http_request_t));
    if (sr == NULL) {
        return NGX_ERROR;
    }

    sr->signature = NGX_HTTP_MODULE;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif
    sr->connection = c;

    sr->ctx = ngx_pcalloc(r->pool, sizeof(void *) * ngx_http_max_module);
    if (sr->ctx == NULL) {
        return NGX_ERROR;
    }

    if (ngx_list_init(&sr->headers_out.headers, r->pool, 20,
                      sizeof(ngx_table_elt_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);
    sr->main_conf = cscf->ctx->main_conf;
    sr->srv_conf = cscf->ctx->srv_conf;
    sr->loc_conf = cscf->ctx->loc_conf;

    sr->pool = r->pool;

    sr->headers_in = r->headers_in;

    sr->request_body = request_body;

#if (NGX_HTTP_SPDY)
    sr->spdy_stream = r->spdy_stream;
#endif

#if (NGX_HTTP_V2)
    sr->stream = r->stream;
#endif

    sr->method = r->method;
    sr->http_version = r->http_version;

    sr->request_line = r->request_line;
    sr->uri = *uri;

    if (args) {
        sr->args = *args;
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http subrequest \"%V?%V\"", uri, &sr->args);

    sr->subrequest_in_memory = (flags & NGX_HTTP_SUBREQUEST_IN_MEMORY) != 0;
    sr->waited = (flags & NGX_HTTP_SUBREQUEST_WAITED) != 0;

    sr->unparsed_uri = r->unparsed_uri;
    sr->method_name = r->method_name;
    sr->http_protocol = r->http_protocol;

    ngx_http_set_exten(sr);

    sr->main = r->main;
    sr->parent = r;
    sr->post_subrequest = ps;
    sr->read_event_handler = ngx_http_request_empty_handler;
    sr->write_event_handler = ngx_http_handler;

    if (c->data == r && r->postponed == NULL) {
        c->data = sr;
    }

    sr->variables = r->variables;

    sr->log_handler = r->log_handler;

    pr = ngx_palloc(r->pool, sizeof(ngx_http_postponed_request_t));
    if (pr == NULL) {
        return NGX_ERROR;
    }

    pr->request = sr;
    pr->out = NULL;
    pr->next = NULL;

    if (r->postponed) {
        for (p = r->postponed; p->next; p = p->next) { /* void */ }
        p->next = pr;

    } else {
        r->postponed = pr;
    }

    sr->internal = 1;

    sr->discard_body = r->discard_body;


    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "http subrequest discard_body: %d", r->discard_body);

    sr->expect_tested = 1;
    sr->main_filter_need_in_memory = r->main_filter_need_in_memory;

    sr->uri_changes = NGX_HTTP_MAX_URI_CHANGES + 1;

    tp = ngx_timeofday();
    sr->start_sec = tp->sec;
    sr->start_msec = tp->msec;

    r->main->count++;

    *psr = sr;

    return ngx_http_post_request(sr, NULL);
}


static ngx_int_t
ngx_http_antirobot_do_request(ngx_http_request_t *r, ngx_http_antirobot_request_conf_t *arcf,
    ngx_http_antirobot_request_ctx_t *ctx, ngx_str_t* uri)
{
    ngx_http_request_t                 *sr;
    ngx_http_post_subrequest_t         *ps;
    ngx_http_request_body_t            *srb;
    ngx_connection_t                   *c;
    ngx_chain_t                        *cl, *new_cl;
    off_t                               size;
    ngx_buf_t                          *b, *buf;
    ngx_uint_t                          sr_flags;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ps = ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));
    if (ps == NULL) {
        return NGX_ERROR;
    }

    ps->handler = ngx_http_antirobot_request_done;
    ps->data = ctx;

    /*
     * allocate fake request body to avoid attempts to read it and to make
     * sure real body file (if already has been read) won't be closed by upstream
     */
    srb = ngx_pcalloc(r->pool, sizeof(ngx_http_request_body_t));
    if (srb == NULL) {
        return NGX_ERROR;
    }

    if ((r->method & (NGX_HTTP_POST|NGX_HTTP_PUT)) &&
        !r->discard_body)
    {
        if (r->request_body != NULL && r->request_body->bufs != NULL) {

            ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                           "antirobot got POST request, [%d]",
                           r->request_body_in_single_buf);

            srb->bufs = new_cl = ngx_alloc_chain_link(r->pool);
            if (new_cl == NULL) {
                return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }

            if (r->request_body->bufs->next != NULL) {

                /* more than one buffer...we should copy the data out... */
                ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                                "antirobot got more than one buf");

                /* XXX: check that buffers are actually copied */
                for (cl = r->request_body->bufs; cl; cl = cl->next) {
                    b = cl->buf;
                    size = ngx_buf_size(b);

                    buf = ngx_create_temp_buf(r->pool, size);
                    if (buf == NULL) {
                        return NGX_HTTP_INTERNAL_SERVER_ERROR;
                    }
                    ngx_memcpy(buf->pos, b->pos, size);

                    if (ngx_buf_in_memory(buf)) {
                        buf->start = buf->pos = b->pos;
                        buf->end = buf->last = b->pos + size;
                    } else {
                        ngx_log_error(NGX_LOG_ERR, c->log, 0,
                                    "this should not happen [case 0]!");
                        return NGX_DECLINED;
                    }
                    buf->last_buf = b->last_buf;

                    new_cl->buf = buf;
                    if (cl->next) {
                        new_cl->next = ngx_alloc_chain_link(r->pool);
                        if (new_cl->next == NULL) {
                            return NGX_HTTP_INTERNAL_SERVER_ERROR;
                        }
                        new_cl = new_cl->next;
                    } else {
                        new_cl->next = NULL;
                    }
                    srb->buf = buf;
                }
            } else {
                b = r->request_body->bufs->buf;
                size = ngx_buf_size(b);
                if (size == 0) {
                    return NGX_ERROR;
                }
                ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                               "antirobot got POST request, size %d", size);


                srb->buf = ngx_create_temp_buf(r->pool, size);
                if (srb->buf == NULL) {
                    return NGX_HTTP_INTERNAL_SERVER_ERROR;
                }

                if (ngx_buf_in_memory(b)) {
                    srb->buf->last = ngx_copy(srb->buf->pos, b->pos, size);

                    srb->bufs->buf = srb->buf;
                    srb->bufs->next = NULL;
                } else {
                    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                                    "antirobot request_body stored in temp file");

                    srb->buf->last_buf = b->last_buf;
                    srb->buf->in_file = 0;
                    srb->buf->last_in_chain = b->last_in_chain;
                    srb->buf->temporary = 0;
                    srb->buf->memory = 1;

                    size = ngx_read_file(b->file, srb->buf->pos, ngx_buf_size(b), b->file_pos);
                    if (size != ngx_buf_size(b)) {
                        ngx_log_error(NGX_LOG_ERR, c->log, 0,
                                    "Error reading temp file");
                        return NGX_ERROR;
                    }

                    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
                                    "antirobot request_body: %*s", size, srb->buf->pos);

                    srb->buf->start = srb->buf->pos;
                    srb->buf->last = srb->buf->pos + size;
                    srb->buf->end = srb->buf->pos + size;

                    srb->bufs->buf = srb->buf;
                    srb->bufs->next = NULL;
                }
            }
        }
    }

    sr_flags = NGX_HTTP_SUBREQUEST_WAITED | NGX_HTTP_SUBREQUEST_IN_MEMORY;

    if (ngx_http_custom_subrequest(r, uri, &r->args, &sr, ps, srb, sr_flags)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    ctx->subrequest = sr;

    return NGX_AGAIN;
}


static ngx_int_t
ngx_http_antirobot_request_handler(ngx_http_request_t *r)
{
    ngx_http_request_t                 *sr;
    ngx_connection_t                   *c;
    ngx_int_t                           rc;
    ngx_uint_t                          i;
    ngx_table_elt_t                    *h, *ho;
    ngx_http_antirobot_request_ctx_t   *ctx;
    ngx_http_antirobot_request_conf_t  *arcf;
    ngx_list_part_t                    *part;
    ngx_str_t                           uri;

    arcf = ngx_http_get_module_loc_conf(r, ngx_http_yandex_antirobot_module);
    /*
    should we skip processing for internal requests as a fool-protection?
    if (r->internal) {
        return NGX_DECLINED;
    }
    */

    if (arcf->uri == NULL) {
        return NGX_DECLINED;
    }

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "antirobot request handler");

    if (ngx_http_complex_value(r, arcf->uri, &uri) != NGX_OK) {
        return NGX_ERROR;
    }

    if (uri.len == 0 || (uri.len == 3 && ngx_strncmp(uri.data, "off", 3) == 0)) {
        return NGX_DECLINED;
    }

    ctx = ngx_http_get_module_ctx(r, ngx_http_yandex_antirobot_module);
    if (ctx == NULL) {
        ctx = ngx_pcalloc(r->pool, sizeof(ngx_http_antirobot_request_ctx_t));
        if (ctx == NULL) {
            return NGX_ERROR;
        }

        ngx_http_set_ctx(r, ctx, ngx_http_yandex_antirobot_module);
    }

    if (ngx_http_test_predicates(r, arcf->antirobot_bypass) == NGX_DECLINED) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0, "antirobot bypass was set");
        ctx->done = 1;
        ctx->need_forward = 0;
        ctx->status = NGX_HTTP_OK;

        if (ngx_http_antirobot_request_set_variables(r, arcf, ctx) != NGX_OK) {
            return NGX_ERROR;
        }

        return NGX_DECLINED;
    }

    if (ctx->waiting_request_body) {
        return NGX_AGAIN;
    }

    if (ctx->request_body_done == 1 && !ctx->done) {
        ctx->request_body_done = 0;
        return ngx_http_antirobot_do_request(r, arcf, ctx, &uri);
    }

    if ((r->method & (NGX_HTTP_POST|NGX_HTTP_PUT)) &&
        !r->discard_body &&
        !r->request_body &&
        !ctx->subrequest)
    {
        rc = ngx_http_read_client_request_body(r, ngx_http_antirobot_post_read_body);
        if (rc == NGX_ERROR || rc > NGX_OK) {
#if 0 && (nginx_version < 1002006)                                               \
    || (nginx_version >= 1003000 && nginx_version < 1003009)
            r->main->count--;
#endif
            return rc;
        }

        if (rc == NGX_AGAIN) {
            ctx->waiting_request_body = 1;
            return NGX_AGAIN;
        }

        return ngx_http_antirobot_do_request(r, arcf, ctx, &uri);
    }

    if (!ctx->subrequest) {
        return ngx_http_antirobot_do_request(r, arcf, ctx, &uri);
    }

    if (!ctx->done) {
        return NGX_AGAIN;
    }

    /*
     * as soon as we are done - explicitly set variables to make
     * sure they will be available after internal redirects
     */
    if (ngx_http_antirobot_request_set_variables(r, arcf, ctx) != NGX_OK) {
        return NGX_ERROR;
    }

    if (r->request_body != NULL) {
        r->main->count--;
    }

    if (ctx->need_forward &&
        ctx->status == NGX_HTTP_FORBIDDEN)
    {
        return ctx->status;
    }

    if (ctx->need_forward) {
        sr = ctx->subrequest;

        part = &sr->headers_out.headers.part;
        h = part->elts;

        for (i = 0; /* void */; i++) {
            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }

                part = part->next;
                h = part->elts;
                i = 0;
            }

            if (h) {
                /*
                 * do not display control header, yes, I know this is overkill and
                 * very unoptimized, need to digg harder at upstream processing
                 * and cpu-memory tradeoff
                 */
                 if (ngx_strncmp(h[i].key.data, ANTIROBOT_CTRL_HDR,
                        sizeof(ANTIROBOT_CTRL_HDR) - 1)
                        == 0)
                 {
                     continue;
                 }

                 ho = ngx_list_push(&r->headers_out.headers);
                 if (ho == NULL) {
                     return NGX_HTTP_INTERNAL_SERVER_ERROR;
                 }

                 /*
                  * make shure header will not be silently discarded
                  */
                  ho->hash = 1;

                  /*
                   * make sure header data will not be freed before
                   * main request will be finished,
                   * pointer copying will be enough, but i'm paranoid
                   */
                  ho->key.data = ngx_pstrdup(r->pool, &h[i].key);
                  ho->key.len = h[i].key.len;

                  ho->value.data = ngx_pstrdup(r->pool, &h[i].value);
                  ho->value.len = h[i].value.len;
            }
        }

        ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                       "antirobot response status: %d", ctx->status);

        rc = ctx->status;
        r->err_status = ctx->status;
        r->headers_out.status = ctx->status;

        /* proxying antirobot request content - we need to send
         * headers manually calling ngx_http_send_header(...)
         */
        if (ctx->status == NGX_HTTP_OK) {
            /* saving Content-Type and Content-Length from antirobot response */
            r->headers_out.content_type_len = sr->headers_out.content_type_len;
            r->headers_out.content_type = sr->headers_out.content_type;
            r->headers_out.content_type_lowcase = NULL;

            r->headers_out.charset.len = sr->headers_out.charset.len;
            r->headers_out.charset.data = sr->headers_out.charset.data;

            if (ctx->subrequest_response_body_len) {
                r->headers_out.content_length_n = ctx->subrequest_response_body_len;
            }

            if (r->headers_out.content_length) {
                r->headers_out.content_length->hash = 0;
                r->headers_out.content_length = NULL;
            }

            rc = ngx_http_send_header(r);
            if (rc == NGX_ERROR) {
                return rc;
            }

            /* return antirobot response body */
            if (ctx->subrequest_response_body.buf) {
                ngx_http_output_filter(r, &ctx->subrequest_response_body);
            }
        } else {
            /* proxying redirect */
            rc = ngx_http_special_response_handler(r, rc);
        }

        ngx_http_finalize_request(r, rc);
        return NGX_DONE;
    }

    return NGX_DECLINED;
}


static void
ngx_http_antirobot_post_read_body(ngx_http_request_t *r)
{
    ngx_http_antirobot_request_ctx_t  *ctx;
    ngx_connection_t                  *c;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ctx = ngx_http_get_module_ctx(r, ngx_http_yandex_antirobot_module);
    if (ctx == NULL) {
      ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                           "antirobot POST ctx=NULL");
    }

    if (!r->main) {
      ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                           "antirobot POST main=NULL");
    }

    ngx_log_debug2(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "antirobot post read for the access phase: wait:%ud c:%ud",
                   (unsigned) ctx->waiting_request_body, r->main->count);

    r->write_event_handler = ngx_http_core_run_phases;

#if 0 && (nginx_version < 1002006)                                               \
    || (nginx_version >= 1003000 && nginx_version < 1003009)
    r->main->count--;
#endif

    if (ctx->waiting_request_body) {
        ctx->request_body_done = 1;
        ctx->waiting_request_body = 0;
        ngx_http_core_run_phases(r);
    }
}


static ngx_int_t
ngx_http_antirobot_request_done(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_http_antirobot_request_ctx_t   *ctx = data;
    ngx_connection_t                   *c;

#ifdef MORE_OPT_HEADER_ACCESS
    ngx_http_antirobot_request_conf_t  *arcf;
#endif

    ngx_list_part_t           *part;
    ngx_table_elt_t           *h;
    ngx_uint_t                 i;
    off_t                      size;
    ngx_buf_t                 *b;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "antirobot request done s:%d", r->headers_out.status);

    ctx->done = 1;
    ctx->need_forward = 0;
    ctx->status = r->headers_out.status;

    /*
     * prevent nginx from killing parent request
     * in case of error while requesting antirobot
     */
    rc = NGX_DECLINED;

#ifdef MORE_OPT_HEADER_ACCESS
    arcf = ngx_http_get_module_loc_conf(r, ngx_http_yandex_antirobot_module);

    /*
     * header was not set in config, hash the default header
     * do it once
     */
    if (arcf->ctrl_header_hash == 0) {
        arcf->ctrl_header_hash = ngx_hash_key(arcf->ctrl_header.data,
                                              arcf->ctrl_header.len);
    }


    h = ngx_hash_find(&r->headers_out.headers, arcf->ctrl_header_hash,
                      arcf->ctrl_header.data, arcf->ctrl_header.len);

    if (h) {
        if (h.value.len > 0 && h.value.data[0] == '1') {
            ctx->need_forward = 1;
            break;
        }
    }
#endif

    part = &r->headers_out.headers.part;
    h = part->elts;

    for (i = 0; ; i++) {
        if (i >= part->nelts) {
            if (part->next == NULL)
                break;

            part = part->next;
            h = part->elts;
            i = 0;
        }


        if (ngx_strncmp(h[i].key.data, ANTIROBOT_CTRL_HDR,
                        sizeof(ANTIROBOT_CTRL_HDR) - 1)
            == 0)
        {
            if (h[i].value.len > 0 && h[i].value.data[0] == '1') {
                ctx->need_forward = 1;
                break;
            }
        }
    }

    ngx_log_debug1(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "antirobot request need_forward:%d", ctx->need_forward);

    if (!ctx->need_forward || ctx->status != NGX_HTTP_OK) {
        return rc;
    }

    if (r->upstream) {
        size = r->upstream->buffer.last - r->upstream->buffer.pos;
        if (size == 0) {
            return rc;
        }

        b = ngx_create_temp_buf(r->pool, size);
        if (b == NULL) {
            return NGX_ERROR;
        }

        b->last = ngx_cpymem(b->start, r->upstream->buffer.pos, size);

        b->last_buf = 1;
        b->last_in_chain = 1;

        ctx->subrequest_response_body.buf = b;
        ctx->subrequest_response_body.next = NULL;
        ctx->subrequest_response_body_len = size;
    }


    return rc;
}


static ngx_int_t
ngx_http_antirobot_request_set_variables(ngx_http_request_t *r,
    ngx_http_antirobot_request_conf_t *arcf, ngx_http_antirobot_request_ctx_t *ctx)
{
    ngx_str_t                          val;
    ngx_http_variable_t               *v;
    ngx_http_variable_value_t         *vv;
    ngx_http_antirobot_request_variable_t  *av, *last;
    ngx_http_core_main_conf_t         *cmcf;
    ngx_connection_t                  *c;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "antirobot request set variables");

    if (arcf->vars == NULL) {
        return NGX_OK;
    }

    cmcf = ngx_http_get_module_main_conf(r, ngx_http_core_module);
    v = cmcf->variables.elts;

    av = arcf->vars->elts;
    last = av + arcf->vars->nelts;

    while (av < last) {
        /*
         * explicitly set new value to make sure it will be available after
         * internal redirects
         */

        vv = &r->variables[av->index];

        if (ngx_http_complex_value(ctx->subrequest, &av->value, &val)
            != NGX_OK)
        {
            return NGX_ERROR;
        }

        vv->valid = 1;
        vv->not_found = 0;
        vv->data = val.data;
        vv->len = val.len;

        if (av->set_handler) {
            /*
             * set_handler only available in cmcf->variables_keys, so we store
             * it explicitly
             */

            av->set_handler(r, vv, v[av->index].data);
        }

        av++;
    }

    return NGX_OK;
}


static ngx_int_t
ngx_http_antirobot_request_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_connection_t              *c;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "antirobot request variable");

    v->not_found = 1;

    return NGX_OK;
}


static ngx_int_t
ngx_http_antirobot_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t  *var;

    var = ngx_http_add_variable(cf, &ngx_http_antirobot_status, NGX_HTTP_VAR_NOHASH);
    if (var == NULL) {
        return NGX_ERROR;
    }
    var->get_handler = ngx_http_antirobot_status_variable;

    return NGX_OK;
}


static void *
ngx_http_antirobot_request_create_conf(ngx_conf_t *cf)
{
    ngx_http_antirobot_request_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_antirobot_request_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    /*
     * set by ngx_pcalloc():
     *
     *     conf->uri = NULL;
     */

    conf->vars = NGX_CONF_UNSET_PTR;
    conf->antirobot_bypass = NGX_CONF_UNSET_PTR;

    return conf;
}


static char *
ngx_http_antirobot_request_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_antirobot_request_conf_t *prev = parent;
    ngx_http_antirobot_request_conf_t *conf = child;

    if (conf->uri == NULL) {
        conf->uri = prev->uri;
    }

    ngx_conf_merge_ptr_value(conf->vars, prev->vars, NULL);

    ngx_conf_merge_ptr_value(conf->antirobot_bypass,
                             prev->antirobot_bypass, NULL);

#ifdef MORE_OPT_HEADER_ACCESS
    ngx_conf_merge_str_value(conf->ctr_header_lowcase, prev->ctr_header_lowcase,
                             ANTIROBOT_CTRL_HDR);
    ngx_conf_merge_value(conf->ctr_header_hash, prev->ctr_header_hash, 0);
#endif

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_antirobot_request_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_antirobot_request_handler;

    return NGX_OK;
}


#ifdef MORE_OPT_HEADER_ACCESS
static char *
ngx_http_antirobot_ctrl_header(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_antirobot_request_conf_t *arcf = conf;
    u_char                            *lowcase_key;

    if (arcf->ctrl_header_lowcase_key != NULL) {
        return "is duplicate";
    }

    value = cf->args->elts;

    ngx_strlow(value[1].data, value[1].data, value[1].len);

    arcf->ctrl_header_lowcase = value[1];
    arcf->ctrl_header_hash = ngx_hash_key(value[1].data, value[1].len);

    return NGX_CONF_OK;
}
#endif


static char *
ngx_http_antirobot_request_set(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_antirobot_request_conf_t *arcf = conf;

    ngx_str_t                         *value;
    ngx_http_variable_t               *v;
    ngx_http_antirobot_request_variable_t  *av;
    ngx_http_compile_complex_value_t   ccv;

    value = cf->args->elts;

    if (value[1].data[0] != '$') {
        ngx_conf_log_error(NGX_LOG_EMERG, cf, 0,
                           "invalid variable name \"%V\"", &value[1]);
        return NGX_CONF_ERROR;
    }

    value[1].len--;
    value[1].data++;

    if (arcf->vars == NGX_CONF_UNSET_PTR) {
        arcf->vars = ngx_array_create(cf->pool, 1,
                                      sizeof(ngx_http_antirobot_request_variable_t));
        if (arcf->vars == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    av = ngx_array_push(arcf->vars);
    if (av == NULL) {
        return NGX_CONF_ERROR;
    }

    v = ngx_http_add_variable(cf, &value[1], NGX_HTTP_VAR_CHANGEABLE);
    if (v == NULL) {
        return NGX_CONF_ERROR;
    }

    av->index = ngx_http_get_variable_index(cf, &value[1]);
    if (av->index == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    if (v->get_handler == NULL) {
        v->get_handler = ngx_http_antirobot_request_variable;
        v->data = (uintptr_t) av;
    }

    av->set_handler = v->set_handler;

    ngx_memzero(&ccv, sizeof(ngx_http_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[2];
    ccv.complex_value = &av->value;

    if (ngx_http_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static ngx_int_t
ngx_http_antirobot_status_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data)
{
    ngx_http_antirobot_request_ctx_t   *ctx;
    ngx_connection_t                   *c;
    size_t                              len;
    u_char                             *value = NULL;

    c = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        c = r->stream->connection->connection;
    }
#endif

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, c->log, 0,
                   "ngx_http_antirobot_status_variable");


    ctx = ngx_http_get_module_ctx(r, ngx_http_yandex_antirobot_module);
    if (ctx == NULL) {
        v->not_found = 1;
        return NGX_OK;
    }

    if (!ctx->done) {
        v->not_found = 1;
        return NGX_OK;
    }

    len = 0;
    if (!ctx->need_forward) {
        if (ctx->status == NGX_HTTP_OK) {
            len = sizeof("bypass") - 1;

            value = (u_char *) ngx_pcalloc(r->pool, len);
            if (value == NULL) {
                v->not_found = 1;
                return NGX_OK;
            }

            ngx_memcpy(value, "bypass", len);

        } else {
            len = sizeof("error") - 1;

            value = (u_char *) ngx_pcalloc(r->pool, len);
            if (value == NULL) {
                v->not_found = 1;
                return NGX_OK;
            }

            ngx_memcpy(value, "error", len);
        }

        v->data = value;
        v->len = len;
        v->valid = 1;
        v->no_cacheable = 0;
        v->not_found = 0;

        return NGX_OK;
    }

    /* antirobot set some action */
    if (ctx->status == NGX_HTTP_MOVED_TEMPORARILY ||
        ctx->status == NGX_HTTP_OK)
    {
        len = sizeof("captcha") - 1;

        value = (u_char *) ngx_pcalloc(r->pool, len);
        if (value == NULL) {
            v->not_found = 1;
            return NGX_OK;
        }

        ngx_memcpy(value, "captcha", len);

    } else if (ctx->status == NGX_HTTP_FORBIDDEN) {
        len = sizeof("denied") - 1;

        value = (u_char *) ngx_pcalloc(r->pool, len);
        if (value == NULL) {
            v->not_found = 1;
            return NGX_OK;
        }

        ngx_memcpy(value, "denied", len);

    }

    if (value == NULL || len == 0) {
        v->not_found = 1;
        return NGX_OK;
    }

    v->data = value;
    v->len = len;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}
