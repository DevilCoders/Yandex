#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "ngx_http_fake_connection.h"

ngx_str_t http_get_method = ngx_string("GET");
ngx_str_t http_protocol_11 = ngx_string("HTTP/1.0");


static void ngx_http_fake_conn_handler(ngx_event_t *ev);
static ngx_chain_t * ngx_http_fake_conn_send_chain(ngx_connection_t *c, ngx_chain_t *in, off_t limit);

ngx_connection_t *
ngx_http_create_fake_connection(ngx_http_core_srv_conf_t *conf, ngx_log_t *log)
{
    ngx_log_t                 *fc_log;
    ngx_pool_t                *pool;
    ngx_event_t               *rev, *wev;
    ngx_connection_t          *fc;
    ngx_http_connection_t     *hc;
    ngx_http_log_ctx_t        *ctx;
    ngx_httt_fake_conn_ctx_t  *fctx;
    ngx_http_addr_conf_t      *addr_conf;
    struct sockaddr_un        *un;

    pool = ngx_create_pool(4096, log);
    if (pool == NULL) {
        return NULL;
    }

    fc = ngx_get_connection(-1, log);
    if (fc == NULL) {
        return NULL;
    }

    rev = ngx_pcalloc(pool, sizeof(ngx_event_t));
    if (rev == NULL) {
        return NULL;
    }

    wev = ngx_pcalloc(pool, sizeof(ngx_event_t));
    if (wev == NULL) {
        return NULL;
    }

    hc = ngx_pcalloc(pool, sizeof(ngx_http_connection_t));
    if (hc == NULL) {
        return NULL;
    }

    addr_conf = ngx_pcalloc(pool, sizeof(ngx_http_addr_conf_t));
    if (addr_conf == NULL) {
        return NULL;
    }

    fc_log = ngx_pcalloc(pool, sizeof(ngx_log_t));
    if (fc_log == NULL) {
        return NULL;
    }

    ctx = ngx_pcalloc(pool, sizeof(ngx_http_log_ctx_t));
    if (ctx == NULL) {
        return NULL;
    }

    un = ngx_pcalloc(pool, sizeof(struct sockaddr_un));
    if (un == NULL) {
        return NULL;
    }

    fctx = ngx_pcalloc(pool, sizeof(ngx_httt_fake_conn_ctx_t));
    if (fctx == NULL) {
        return NULL;
    }

    ctx->connection = fc;
    ctx->request = NULL;

    ngx_memcpy(fc_log, log, sizeof(ngx_log_t));

    log->data = ctx;

    ngx_memzero(rev, sizeof(ngx_event_t));

    rev->data = fctx;
    rev->ready = 0;
    rev->handler = ngx_http_fake_conn_handler;
    rev->log = log;

    ngx_memcpy(wev, rev, sizeof(ngx_event_t));

    wev->write = 1;
    wev->active = 1;

    un->sun_family = AF_UNIX;
    strcpy(un->sun_path, "/dev/null");

    addr_conf->default_server = conf;
    hc->addr_conf = addr_conf;

    /* the default server configuration for the address:port */
    hc->conf_ctx = conf->ctx;

    fc->fd = -1;
    fc->data = hc;
    fc->read = rev;
    fc->write = wev;
    fc->send_chain = ngx_http_fake_conn_send_chain;
    fc->sendfile = 0;
    fc->sent = 0;
    fc->log = log;
    fc->buffered = NGX_LOWLEVEL_BUFFERED;
    fc->sndlowat = 1;
    fc->tcp_nodelay = NGX_TCP_NODELAY_DISABLED;
    fc->sockaddr = (struct sockaddr *)un;
    fc->socklen = sizeof(struct sockaddr_un);
    fc->pool = pool;
    //fc->addr_text = ngx_string("/dev/null");

    return fc;
}

static void
ngx_http_fake_conn_handler(ngx_event_t *ev)
{
    ngx_connection_t          *fc;
    ngx_http_request_t        *r;

    fc = ev->data;
    r = fc->data;

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, r->connection->log, 0,
                   "http fake connection handler");
}

static ngx_chain_t *
ngx_http_fake_conn_send_chain(ngx_connection_t *c, ngx_chain_t *in, off_t limit)
{
    off_t                      sent = 0;
    off_t                      size, size_to_copy;
    ngx_chain_t               *link, *cl, **ll;
    ngx_httt_fake_conn_ctx_t  *fctx;
    ngx_http_request_t        *r;
    int                        last = 0;

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, c->log, 0, "ngx_http_fake_conn_send_chain");
    fctx = c->write->data;
    r = c->data;
    r->lingering_close = 0;

    ll = &fctx->data;
    for (cl = fctx->data; cl; cl = cl->next) {
        ll = &cl->next;
    }

    if (limit == 0) {
        limit = NGX_MAX_UINT32_VALUE;
    }

    if (fctx->header_remain == -1) {
        fctx->header_remain = r->header_size;
    }

    for (link = in; link && sent < limit; link = link->next)
    {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, c->log, 0, "ngx_http_fake_conn_send_chain buf last: %d, last_in_chain: %d, flush: %d, sync: %d, next: %p",
                (int)link->buf->last_buf, (int)link->buf->last_in_chain, (int)link->buf->flush, (int)link->buf->sync, link->next);

        if (link->buf->last_buf) {
            last = 1;
        }

        if (ngx_buf_special(link->buf)) {
            continue;
        }

        if (!ngx_buf_in_memory(link->buf)) {
            ngx_log_error(NGX_LOG_ERR, c->log, 0, "Fake connection doesn't support file buffers");
            return NGX_CHAIN_ERROR;
        }

        size = ngx_buf_size(link->buf);
        size = ngx_min(size, limit - sent);
        //ngx_log_debug(NGX_LOG_DEBUG_HTTP, c->log, 0, "ngx_http_fake_conn_send_chain got %lu bytes", size);

        if (fctx->header_remain >= 0) {
            size_to_copy = ngx_min(size, fctx->header_remain);
            fctx->header_remain -= size_to_copy;
            size_to_copy = size - size_to_copy;
        } else {
            size_to_copy = size;
        }

        if (size_to_copy) {
            cl = ngx_alloc_chain_link(r->pool);
            cl->next = NULL;
            cl->buf = ngx_create_temp_buf(r->pool, size_to_copy);
            cl->buf->last = ngx_cpymem(cl->buf->last, link->buf->pos, size_to_copy);
            //ngx_log_debug(NGX_LOG_DEBUG_HTTP, c->log, 0, "ngx_http_fake_conn_send_chain copied %lu bytes", size_to_copy);
            //ngx_log_debug(NGX_LOG_DEBUG_HTTP, c->log, 0, "%*s", (int)size_to_copy, cl->buf->pos);

            *ll = cl;
            ll = &cl->next;
        }

        link->buf->pos += size;
        sent += size;
    }

    if (last) {
        c->buffered = 0;
        if (r->post_subrequest) {
            r->post_subrequest->handler(r, r->post_subrequest->data, 0);
        }
    }

    return link;
}


ngx_http_request_t *
ngx_http_create_fake_request(ngx_connection_t *fc, ngx_str_t *uri, ngx_http_post_subrequest_t *ps)
{
    u_char                    *p;
    ngx_http_request_t        *r;
    ngx_http_core_srv_conf_t  *cscf;
    ngx_httt_fake_conn_ctx_t  *fctx;

    r = ngx_http_create_request(fc);
    if (r == NULL) {
        return NULL;
    }

    fctx = fc->write->data;
    fctx->r = r;
    fctx->header_remain = -1;

    if (ps) {
        ps->data = fctx;
        r->post_subrequest = ps;
    }

    fc->data = r;
    r->valid_location = 1;
    r->lingering_close = 0;
    r->keepalive = 0;

    cscf = ngx_http_get_module_srv_conf(r, ngx_http_core_module);

    r->header_in = ngx_create_temp_buf(r->pool,
                                       cscf->client_header_buffer_size);
    if (r->header_in == NULL) {
        ngx_http_free_request(r, NGX_HTTP_INTERNAL_SERVER_ERROR);
        return NULL;
    }

    r->headers_in.connection_type = NGX_HTTP_CONNECTION_CLOSE;

    r->http_major = 1;
    r->http_minor = 0;
    r->http_version = 1001;
    r->http_protocol = http_protocol_11;

    r->method_name = http_get_method;
    r->method = NGX_HTTP_GET;

    r->uri_start = uri->data;
    r->uri_end = uri->data + uri->len;

    if (ngx_http_parse_uri(r) != NGX_OK) {
        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                      "client sent invalid URI: \"%*s\"",
                      r->uri_end - r->uri_start, r->uri_start);

        return NULL;
    }

    if (ngx_http_process_request_uri(r) != NGX_OK) {
        /*
         * request has been finalized already
         * in ngx_http_process_request_uri()
         */
        return NULL;
    }

    r->request_line.len = r->method_name.len + 1
                          + r->unparsed_uri.len + 1
                          + r->http_protocol.len;

    p = ngx_pnalloc(r->pool, r->request_line.len + 1);
    if (p == NULL) {
        return NULL;
    }

    r->request_line.data = p;

    p = ngx_cpymem(p, r->method_name.data, r->method_name.len);

    *p++ = ' ';

    p = ngx_cpymem(p, r->unparsed_uri.data, r->unparsed_uri.len);

    *p++ = ' ';

    ngx_memcpy(p, r->http_protocol.data, r->http_protocol.len + 1);

    /* some modules expect the space character after method name */
    r->method_name.data = r->request_line.data;

    r->http_state = NGX_HTTP_PROCESS_REQUEST_STATE;
    r->internal = 1;
    r->main_filter_need_in_memory = 1;

    return r;
}
