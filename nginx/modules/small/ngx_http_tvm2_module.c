#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <ngx_queue.h>
#include <library/c/tvmauth/tvmauth.h>
#include <library/c/tvmauth/deprecated.h>
#include "ngx_http_fake_connection.h"


const ngx_msec_t one_hour = 60 * 60 * 1000;
const ngx_str_t x_user_ticket_uids = ngx_string("X-User-Ticket-Uids");
const ngx_str_t x_user_ticket_def_uid = ngx_string("X-User-Ticket-Default-Uid");
const ngx_str_t x_user_ticket_Scopes = ngx_string("X-User-Ticket-Scopes");

typedef struct {
    ngx_http_complex_value_t   *ticket;
    ngx_http_complex_value_t   *user_ticket;
    ngx_array_t                *sources;     /* array of uint32_t */
    ngx_array_t                *scopes;     /* array of ngx_str_t */
} ngx_http_tvm2_loc_conf_t;

typedef struct {
    ngx_uint_t                  service_id;
    ngx_str_t                   keys_url;

    ngx_event_t                 timer_ev;
    struct TA_TServiceContext *service_context;
    struct TA_TUserContext    *user_context;
} ngx_http_tvm2_srv_conf_t;


static ngx_int_t ngx_http_tvm2_handler(ngx_http_request_t *r);
static ngx_int_t ngx_http_tvm2_user_ticket_handler(ngx_http_request_t *r);
static void ngx_http_tvm2_service_ticket_cleanup(void *data);
char * ngx_conf_set_uint_array_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);
static void *ngx_http_tvm2_create_srv_conf(ngx_conf_t *cf);
static char *ngx_http_tvm2_merge_srv_conf(ngx_conf_t *cf,
    void *parent, void *child);
static void *ngx_http_tvm2_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_tvm2_merge_loc_conf(ngx_conf_t *cf,
    void *parent, void *child);
static void ngx_http_tvm2_cleanup(void *data);
static ngx_int_t ngx_http_tvm2_init(ngx_conf_t *cf);
static ngx_int_t ngx_http_tvm2_init_process(ngx_cycle_t *cycle);


static ngx_command_t  ngx_http_tvm2_commands[] = {

    { ngx_string("tvm2_service_id"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_num_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_tvm2_srv_conf_t, service_id),
      NULL },

    { ngx_string("tvm2_keys_url"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_CONF_TAKE1,
      ngx_conf_set_str_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_tvm2_srv_conf_t, keys_url),
      NULL },

    { ngx_string("tvm2_ticket"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_tvm2_loc_conf_t, ticket),
      NULL },

    { ngx_string("tvm2_user_ticket"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF
                        |NGX_CONF_TAKE1,
      ngx_http_set_complex_value_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_tvm2_loc_conf_t, user_ticket),
      NULL },

    { ngx_string("tvm2_allow_scope"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_str_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_tvm2_loc_conf_t, scopes),
      NULL },

    { ngx_string("tvm2_allow_src"),
      NGX_HTTP_MAIN_CONF|NGX_HTTP_SRV_CONF|NGX_HTTP_LOC_CONF|NGX_HTTP_LMT_CONF
                        |NGX_CONF_TAKE1,
      ngx_conf_set_uint_array_slot,
      NGX_HTTP_LOC_CONF_OFFSET,
      offsetof(ngx_http_tvm2_loc_conf_t, sources),
      NULL },

      ngx_null_command
};



static ngx_http_module_t  ngx_http_tvm2_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_tvm2_init,                    /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    ngx_http_tvm2_create_srv_conf,         /* create server configuration */
    ngx_http_tvm2_merge_srv_conf,          /* merge server configuration */

    ngx_http_tvm2_create_loc_conf,         /* create location configuration */
    ngx_http_tvm2_merge_loc_conf           /* merge location configuration */
};


ngx_module_t  ngx_http_tvm2_module = {
    NGX_MODULE_V1,
    &ngx_http_tvm2_module_ctx,             /* module context */
    ngx_http_tvm2_commands,                /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    ngx_http_tvm2_init_process,            /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_tvm2_handler(ngx_http_request_t *r)
{
    uint32_t                   *s;
    uint32_t                    src;
    ngx_int_t                   res;
    ngx_uint_t                  i;
    ngx_log_t                  *log;
    ngx_http_tvm2_srv_conf_t   *tscf;
    ngx_http_tvm2_loc_conf_t   *tlcf;
    ngx_str_t                   ticket_body;
    struct TA_TCheckedServiceTicket  *ticket;
    ngx_pool_cleanup_t         *cln;

    tscf = ngx_http_get_module_srv_conf(r, ngx_http_tvm2_module);

    log = r->connection->log;
    tlcf = ngx_http_get_module_loc_conf(r, ngx_http_tvm2_module);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "TVM2 handler");

    if (tlcf->ticket == NULL) {
        return NGX_DECLINED;
    }

    if (ngx_http_complex_value(r, tlcf->ticket, &ticket_body) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "TVM2: unable to get ticket value");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0,
            "TVM2: ticket value is %V", &ticket_body);

    if (ticket_body.len == 0) {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                "TVM2: empty ticket body");
        return NGX_HTTP_FORBIDDEN;
    }

    cln = ngx_pool_cleanup_add(r->pool, 0);
    if (cln == NULL) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    cln->handler = ngx_http_tvm2_service_ticket_cleanup;

    res = TA_CheckServiceTicket(tscf->service_context, (char*)ticket_body.data, ticket_body.len, &ticket);
    if (res != TA_EC_OK) {
        ngx_log_error(NGX_LOG_INFO, log, res,
                      "TVM2: unable to validate ticket: %s", TA_ErrorCodeToString(res));
        return NGX_HTTP_FORBIDDEN;
    }

    cln->data = ticket;

    // Check is source client_id matches
    if (tlcf->sources) {
        s = tlcf->sources->elts;
        res = TA_GetServiceTicketSrc(ticket, &src);
        if (res != TA_EC_OK) {
            ngx_log_error(NGX_LOG_INFO, log, res,
                          "TVM2: unable to get source client_id from ticket: %s", TA_ErrorCodeToString(res));
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
        }

        ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0,
                      "TVM2: source client_id is %d", src);
        for (i = 0; i < tlcf->sources->nelts; i++) {
            if (s[i] == src) {
                return NGX_OK;
            }
        }

        return NGX_HTTP_FORBIDDEN;
    }


    return NGX_DECLINED;
}


static void
ngx_http_tvm2_service_ticket_cleanup(void *data)
{
    struct TA_TCheckedServiceTicket  *ticket = (struct TA_TCheckedServiceTicket*)data;
    if (ticket) {
        TA_DeleteServiceTicket(ticket);
    }
}


static ngx_int_t
ngx_http_tvm2_add_header(ngx_http_request_t *r, const ngx_str_t *name, ngx_str_t *value)
{
    ngx_table_elt_t            *h;

    h = ngx_list_push(&r->headers_in.headers);

    if (h == NULL) {
        return NGX_ERROR;
    }

    h->key = *name;
    h->hash = ngx_hash_key_lc(h->key.data, h->key.len);

    h->value = *value;

    return NGX_OK;
}


static ngx_int_t
ngx_http_tvm2_user_ticket_handler(ngx_http_request_t *r)
{
    ngx_int_t                   res;
    ngx_log_t                  *log;
    ngx_http_tvm2_srv_conf_t   *tscf;
    ngx_http_tvm2_loc_conf_t   *tlcf;
    ngx_str_t                   ticket_body;
    struct TA_TCheckedUserTicket     *ticket;
    ngx_str_t                   value;
    u_char                     *p;
    uint64_t                    uid;
    size_t                      count, i;

    tscf = ngx_http_get_module_srv_conf(r, ngx_http_tvm2_module);

    log = r->connection->log;
    tlcf = ngx_http_get_module_loc_conf(r, ngx_http_tvm2_module);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0, "TVM2 user ticket handler");

    if (tlcf->user_ticket == NULL) {
        return NGX_DECLINED;
    }

    if (ngx_http_complex_value(r, tlcf->user_ticket, &ticket_body) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, 0,
                      "TVM2: unable to get ticket value");
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, log, 0,
            "TVM2: ticket value is %V", &ticket_body);

    if (ticket_body.len == 0) {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                "TVM2: empty ticket body");
        return NGX_HTTP_FORBIDDEN;
    }

    res = TA_CheckUserTicket(tscf->user_context, (char*)ticket_body.data, ticket_body.len, &ticket);
    if (res != TA_EC_OK) {
        ngx_log_error(NGX_LOG_INFO, log, res,
                      "TVM2: unable to validate ticket: %s", TA_ErrorCodeToString(res));
        return NGX_HTTP_FORBIDDEN;
    }

    //
    // Set default uid
    //

    value.len = NGX_INT64_LEN;
    value.data = ngx_palloc(r->pool, value.len);
    if (value.data == NULL) {
        goto cleanup;
    }

    res = TA_GetUserTicketDefaultUid(ticket, &uid);
    if (res != TA_EC_OK) {
        ngx_log_error(NGX_LOG_INFO, log, res,
                      "TVM2: unable to get default uid: %s", TA_ErrorCodeToString(res));
        goto cleanup;
    }

    p = ngx_sprintf(value.data, "%uL", uid);
    value.len = p - value.data;

    if (ngx_http_tvm2_add_header(r, &x_user_ticket_def_uid, &value) != NGX_OK) {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                      "TVM2: unable to set header: %V", &x_user_ticket_def_uid);
        goto cleanup;
    }

    //
    // Set uids
    //

    res = TA_GetUserTicketUidsCount(ticket, &count);
    if (res != TA_EC_OK) {
        ngx_log_error(NGX_LOG_INFO, log, res,
                      "TVM2: unable to get uids count: %s", TA_ErrorCodeToString(res));
        goto cleanup;
    }

    value.len = count * (NGX_INT64_LEN + 1);
    value.data = ngx_palloc(r->pool, value.len);
    if (value.data == NULL) {
        goto cleanup;
    }

    p = value.data;
    for (i = 0; i < count; i++)
    {
        res = TA_GetUserTicketUid(ticket, i, &uid);
        if (res != TA_EC_OK) {
            ngx_log_error(NGX_LOG_INFO, log, res,
                          "TVM2: unable to get uid: %s", TA_ErrorCodeToString(res));
            goto cleanup;
        }

        p = ngx_sprintf(p, "%uL,", uid);
    }

    value.len = p - value.data - 1;

    if (ngx_http_tvm2_add_header(r, &x_user_ticket_uids, &value) != NGX_OK) {
        ngx_log_error(NGX_LOG_INFO, log, 0,
                      "TVM2: unable to set header: %V", &x_user_ticket_uids);
        goto cleanup;
    }

    //
    // Delete ticket
    //

    TA_DeleteUserTicket(ticket);

    return NGX_DECLINED;

cleanup:
    TA_DeleteUserTicket(ticket);

    return NGX_ERROR;
}


char *
ngx_conf_set_uint_array_slot(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    char  *p = conf;

    ngx_str_t         *value;
    uint32_t          *s;
    ngx_int_t          res;
    ngx_array_t      **a;
    ngx_conf_post_t   *post;

    a = (ngx_array_t **) (p + cmd->offset);

    if (*a == NGX_CONF_UNSET_PTR) {
        *a = ngx_array_create(cf->pool, 4, sizeof(uint32_t));
        if (*a == NULL) {
            return NGX_CONF_ERROR;
        }
    }

    s = ngx_array_push(*a);
    if (s == NULL) {
        return NGX_CONF_ERROR;
    }

    value = cf->args->elts;

    res = ngx_atoi(value[1].data, value[1].len);
    if (res == NGX_ERROR) {
        return NGX_CONF_ERROR;
    }

    *s = (uint32_t)res;

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, s);
    }

    return NGX_CONF_OK;
}


static void *
ngx_http_tvm2_create_srv_conf(ngx_conf_t *cf)
{
    ngx_pool_cleanup_t     *cln;
    ngx_http_tvm2_srv_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tvm2_srv_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->service_id = NGX_CONF_UNSET_UINT;

    cln = ngx_pool_cleanup_add(cf->pool, 0);
    if (cln == NULL) {
        return NULL;
    }

    cln->handler = ngx_http_tvm2_cleanup;
    cln->data = conf;

    return conf;
}


static char *
ngx_http_tvm2_merge_srv_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_tvm2_srv_conf_t  *prev = parent;
    ngx_http_tvm2_srv_conf_t  *conf = child;

    ngx_conf_merge_uint_value(conf->service_id,
                              prev->service_id, 0);

    ngx_conf_merge_str_value(conf->keys_url,
                             prev->keys_url, "");

    if (conf->service_id > 0 && conf->keys_url.len == 0) {
        ngx_conf_log_error(NGX_LOG_ERR, cf, 0,
                "tvm2_keys_url is required");
        return NGX_CONF_ERROR;
    }

    return NGX_CONF_OK;
}


static void *
ngx_http_tvm2_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_tvm2_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_tvm2_loc_conf_t));
    if (conf == NULL) {
        return NULL;
    }

    conf->ticket = NULL;
    conf->user_ticket = NULL;
    conf->sources = NGX_CONF_UNSET_PTR;
    conf->scopes = NGX_CONF_UNSET_PTR;

    return conf;
}


static char *
ngx_http_tvm2_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_tvm2_loc_conf_t  *prev = parent;
    ngx_http_tvm2_loc_conf_t  *conf = child;

    if (conf->ticket == NULL) {
        conf->ticket = prev->ticket;
    }

    if (conf->user_ticket == NULL) {
        conf->user_ticket = prev->user_ticket;
    }

    ngx_conf_merge_ptr_value(conf->sources,
                             prev->sources, NULL);

    ngx_conf_merge_ptr_value(conf->scopes,
                             prev->scopes, NULL);

    return NGX_CONF_OK;
}


static void
ngx_http_tvm2_cleanup(void *data)
{
    ngx_http_tvm2_srv_conf_t  *conf = data;

    if (conf->service_context) {
        TA_DeleteServiceContext(conf->service_context);
    }

    if (conf->user_context) {
        TA_DeleteUserContext(conf->user_context);
    }
}


static ngx_int_t
ngx_http_tvm2_init(ngx_conf_t *cf)
{
    ngx_http_handler_pt        *h;
    ngx_http_core_main_conf_t  *cmcf;

    cmcf = ngx_http_conf_get_module_main_conf(cf, ngx_http_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_ACCESS_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_tvm2_handler;

    h = ngx_array_push(&cmcf->phases[NGX_HTTP_REWRITE_PHASE].handlers);
    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_http_tvm2_user_ticket_handler;

    return NGX_OK;
}


static ngx_int_t
ngx_http_tvm2_load_keys_handler(ngx_http_request_t *r, void *data, ngx_int_t rc)
{
    ngx_http_tvm2_srv_conf_t  *tscf;
    ngx_httt_fake_conn_ctx_t  *fctx;
    ngx_chain_t               *cl;
    ngx_buf_t                 *b;
    ngx_str_t                  public_keys;
    u_char                    *p;
    ngx_int_t                  res;
    struct TA_TServiceContext *service_context, *old;
    struct TA_TUserContext    *user_context, *old_user;

    tscf = ngx_http_get_module_srv_conf(r, ngx_http_tvm2_module);

    fctx = (ngx_httt_fake_conn_ctx_t*)data;
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, r->connection->log, 0, "TVM2 load keys handler");

    public_keys.len = 0;
    for (cl = fctx->data; cl; cl = cl->next)
    {
        b = cl->buf;
        public_keys.len += ngx_buf_size(b);
    }

    public_keys.data = ngx_palloc(r->pool, public_keys.len);
    p = public_keys.data;

    for (cl = fctx->data; cl; cl = cl->next)
    {
        b = cl->buf;
        p = ngx_cpymem(p, b->pos, ngx_buf_size(b));
    }

    res = TA_CreateServiceContext((uint32_t)(tscf->service_id), "FAKE", 4, (char *)public_keys.data, public_keys.len, &service_context);
    if (res != TA_EC_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, res,
                "TVM2: failed to load public keys: TA_CreateServiceContext failed: %s", TA_ErrorCodeToString(res));
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "TVM2: public keys: len: %d, content_length: %d, value: %V", (int)public_keys.len, (int)r->headers_out.content_length_n, &public_keys);

        ngx_add_timer(&tscf->timer_ev, 10 * 1000);
        return NGX_OK;
    } else {
        old = tscf->service_context;
        tscf->service_context = service_context;
        TA_DeleteServiceContext(old);

        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                "TVM2: successfully loaded public keys for service tickets");
    }

    res = TA_CreateUserContext(TA_BE_PROD, (char *)public_keys.data, public_keys.len, &user_context);
    if (res != TA_EC_OK) {
        ngx_log_error(NGX_LOG_ERR, r->connection->log, res,
                "TVM2: failed to load public keys: TA_CreateUserContext failed: %s", TA_ErrorCodeToString(res));
        ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
                "TVM2: public keys (%d bytes): %V", (int)public_keys.len, &public_keys);
        ngx_add_timer(&tscf->timer_ev, 10 * 1000);
        return NGX_OK;
    } else {
        old_user = tscf->user_context;
        tscf->user_context = user_context;
        TA_DeleteUserContext(old_user);

        ngx_log_error(NGX_LOG_INFO, r->connection->log, 0,
                "TVM2: successfully loaded public keys for user tickets");
    }

    ngx_add_timer(&tscf->timer_ev, one_hour);

    return NGX_OK;
}

static ngx_int_t
ngx_http_tvm2_load_keys_request(ngx_http_core_srv_conf_t *cscf)
{
    ngx_http_tvm2_srv_conf_t    *tscf;
    ngx_connection_t            *fc;
    ngx_http_request_t          *r;
    ngx_http_post_subrequest_t  *ps;

    tscf = ngx_http_conf_get_module_srv_conf(cscf, ngx_http_tvm2_module);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "TVM2 create fake connection");
    fc = ngx_http_create_fake_connection(cscf, ngx_cycle->log);
    if (fc == NULL) {
        return NGX_ERROR;
    }

    ps = ngx_palloc(fc->pool, sizeof(ngx_http_post_subrequest_t));
    if (ps == NULL) {
        return NGX_ERROR;
    }

    ps->handler = ngx_http_tvm2_load_keys_handler;

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "TVM2 create fake request");
    r = ngx_http_create_fake_request(fc, &tscf->keys_url, ps);
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "TVM2 create fake request done");

    if (r == NULL) {
        return NGX_ERROR;
    }

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "TVM2 process request");
    ngx_http_process_request(r);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0, "TVM2 done");
    return NGX_OK;
}


static void
ngx_http_tvm2_timer_handler(ngx_event_t *ev)
{
    ngx_http_core_srv_conf_t    *cscf;

    cscf = ev->data;
    if (ngx_http_tvm2_load_keys_request(cscf) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, ev->log, 0, "TVM2 failed to create load keys request");
    }
}


static ngx_int_t
ngx_http_tvm2_init_process(ngx_cycle_t *cycle)
{
    ngx_uint_t                   i;
    ngx_http_conf_ctx_t         *http_ctx;
    ngx_http_core_srv_conf_t    *cscf, **cscfp;
    ngx_http_core_main_conf_t   *cmcf;
    ngx_http_tvm2_srv_conf_t    *tscf;

    http_ctx = (ngx_http_conf_ctx_t*) ngx_get_conf(cycle->conf_ctx, ngx_http_module);

    ngx_log_debug(NGX_LOG_DEBUG_HTTP, cycle->log, 0, "TVM2 init process trace");

    if (http_ctx== NULL) {
        return NGX_OK;
    }

    cmcf = http_ctx->main_conf[ngx_http_core_module.ctx_index];
    if (http_ctx== NULL) {
        return NGX_OK;
    }

    cscfp = (ngx_http_core_srv_conf_t**)cmcf->servers.elts;
    for (i = 0; i < cmcf->servers.nelts; i++) {
        cscf = cscfp[i];
        tscf = ngx_http_conf_get_module_srv_conf(cscf, ngx_http_tvm2_module);
        if (tscf->service_id > 0) {

            tscf->timer_ev.log = cycle->log;
            tscf->timer_ev.data = cscf;
            tscf->timer_ev.handler = ngx_http_tvm2_timer_handler;
            tscf->timer_ev.cancelable = 1;

            if (ngx_http_tvm2_load_keys_request(cscf) != NGX_OK) {
                ngx_log_error(NGX_LOG_ERR, cycle->log, 0, "TVM2 failed to create load keys request");
            }
        }
    }

    return NGX_OK;
}
