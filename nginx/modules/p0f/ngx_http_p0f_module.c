#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <string.h>

#include <kernel/p0f/bpf/p0f_bpf.h>
#include <kernel/p0f/load/bpf_load.h>

#include <contrib/libs/util-linux/include/linux_version.h>

#include "p0f_bridge.h"

#define P0F_MAP_DEFAULT_LIMIT ((size_t)10000)

typedef struct {
    ngx_flag_t enabled;
    size_t map_limit;
    int prog_fd;
    int map_fd;
} ngx_http_p0f_main_conf_t;

static ngx_int_t ngx_http_p0f_module_init(ngx_cycle_t* cycle);

static ngx_int_t ngx_http_p0f_add_variables(ngx_conf_t* cf);

static void* ngx_http_p0f_create_main_conf(ngx_conf_t* cf);

static ngx_int_t ngx_http_p0f_fingerprint(ngx_http_request_t* r,
                                          ngx_http_variable_value_t* v,
                                          uintptr_t data);

static int p0f_attach(ngx_http_p0f_main_conf_t* ctx, ngx_log_t* log,
                      ngx_socket_t socket);

static ngx_command_t ngx_http_p0f_commands[] = {
    {ngx_string("p0f"), NGX_HTTP_MAIN_CONF | NGX_CONF_FLAG,
     ngx_conf_set_flag_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(ngx_http_p0f_main_conf_t, enabled), NULL},
    {ngx_string("p0f_map_size"), NGX_HTTP_MAIN_CONF | NGX_CONF_TAKE1,
     ngx_conf_set_size_slot, NGX_HTTP_MAIN_CONF_OFFSET,
     offsetof(ngx_http_p0f_main_conf_t, map_limit), NULL},
    ngx_null_command};

static ngx_http_module_t ngx_http_p0f_module_ctx = {
        ngx_http_p0f_add_variables, /* preconfiguration */
        NULL,                       /* postconfiguration */

        ngx_http_p0f_create_main_conf, /* create main configuration */
        NULL,                          /* init main configuration */

        NULL, /* create server configuration */
        NULL, /* merge server configuration */

        NULL, /* create location configuration */
        NULL  /* merge location configuration */
    };

ngx_module_t ngx_http_p0f_module = {
    NGX_MODULE_V1,
    &ngx_http_p0f_module_ctx, /* module context */
    ngx_http_p0f_commands,    /* module directives */
    NGX_HTTP_MODULE,          /* module type */
    NULL,                     /* init master */
    ngx_http_p0f_module_init, /* init module */
    NULL,                     /* init process */
    NULL,                     /* init thread */
    NULL,                     /* exit thread */
    NULL,                     /* exit process */
    NULL,                     /* exit master */
    NGX_MODULE_V1_PADDING};

static ngx_http_variable_t ngx_http_p0f_vars[] = {
    {ngx_string("p0f_fingerprint"), NULL, ngx_http_p0f_fingerprint, 0,
     NGX_HTTP_VAR_NOCACHEABLE, 0},
    ngx_http_null_variable,
};

static ngx_int_t ngx_http_p0f_add_variables(ngx_conf_t* cf) {
    ngx_http_variable_t *var, *v;

    for (v = ngx_http_p0f_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

static void* ngx_http_p0f_create_main_conf(ngx_conf_t* cf) {
    ngx_http_p0f_main_conf_t* conf;

    conf = (ngx_http_p0f_main_conf_t*)(ngx_pcalloc(
        cf->pool, sizeof(ngx_http_p0f_main_conf_t)));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }
    conf->enabled = NGX_CONF_UNSET;
    conf->map_limit = NGX_CONF_UNSET_SIZE;
    return conf;
}

static void fixup_map(struct bpf_map_data* map, int idx, void* context) {
    if (strncmp(map->name, "p0f_map", 8) == 0) {
        ngx_http_p0f_main_conf_t* ctx = (ngx_http_p0f_main_conf_t*)context;
        ctx->map_fd = map->fd;
        size_t map_limit = ctx->map_limit;
        if (map_limit == NGX_CONF_UNSET_SIZE) {
            map_limit = P0F_MAP_DEFAULT_LIMIT;
        }

        map->def.max_entries = map_limit;
    }
}

#define SO_ATTACH_BPF 50 // defined in asm-generic/socket.h

static ngx_int_t ngx_http_p0f_module_init(ngx_cycle_t* cycle) {
    ngx_http_p0f_main_conf_t* mod =
        ngx_http_cycle_get_module_main_conf(cycle, ngx_http_p0f_module);

    ngx_log_t* log = cycle->log;
    ngx_log_error(NGX_LOG_INFO, log, 0, "Initializing p0f module");

    if (mod->enabled > 0) {
        uint32_t version = get_linux_version();
        if (version < KERNEL_VERSION(4, 18, 0)) {
            ngx_log_error(NGX_LOG_EMERG, log, 0,
                          "Could not load p0f module: module requires linux kernel "
                          "version >= 4.18");
            mod->enabled = 0;
            return NGX_ERROR;
        }

        ngx_log_error(NGX_LOG_INFO, log, 0, "Module p0f is enabled");
        if (mod->map_limit != NGX_CONF_UNSET_SIZE) {
            ngx_log_debug(NGX_LOG_DEBUG_ALL, log, 0, "p0f_map_size = %lu", mod->map_limit);
        }

        int bpf_fd = load_bpf_image_fixup_map(
            _binary_p0f_bpf_start, _binary_p0f_bpf_len(), fixup_map, &mod);
        if (bpf_fd < 0) {
            ngx_log_error(NGX_LOG_EMERG, log, errno, "Failed to load BPF: %s",
                          strerror(errno));
            return NGX_ERROR;
        }

        mod->prog_fd = bpf_fd;

        ngx_array_t* listening = &cycle->listening;
        ngx_listening_t* listener = listening->elts;
        for (ngx_uint_t i = 0; i < listening->nelts; i++, listener++) {
            p0f_attach(mod, log, listener->fd);
        }

        ngx_log_error(NGX_LOG_INFO, log, 0,
                      "Module p0f is loaded. Use $p0f_fingerprint variable to "
                      "access fingerprints");
    }

    return NGX_OK;
}

static int p0f_attach(ngx_http_p0f_main_conf_t* ctx, ngx_log_t* log,
                      ngx_socket_t socket) {
    if (socket != -1) {
        int err = setsockopt(socket, SOL_SOCKET, SO_ATTACH_BPF, &ctx->prog_fd,
                             sizeof(ctx->prog_fd));
        if (err) {
            ngx_log_error(NGX_LOG_EMERG, log, 0,
                          "Failed to attach BPF fd %d for socket %d: %s",
                          ctx->prog_fd, socket, strerror(errno));
        } else {
            ngx_log_error(NGX_LOG_INFO, log, 0, "attached p0f_bpf to socket %d",
                          socket);
        }

        return err;
    }

    return -1;
}

#undef KEY_LOG
#define KEY_LOG(...) \
    ngx_log_debug(NGX_LOG_DEBUG_HTTP, connection->log, 0, __VA_ARGS__)

static ngx_int_t ngx_http_p0f_extract_fingerprint(
    ngx_http_request_t* r, ngx_http_p0f_req_ctx_t* req_ctx,
    ngx_http_p0f_main_conf_t* ctx, p0f_key_t* key) {
    p0f_value_t value = {0};

    ngx_connection_t* connection = r->connection;

#if (NGX_HTTP_V2)
    if (r->stream) {
        connection = r->stream->connection->connection;
    }
#endif

    if (connection->log->log_level & NGX_LOG_DEBUG_HTTP) {
        KEY_DEBUG(key->hash);
    }

    if (bpf_map_lookup_elem(ctx->map_fd, key, &value) == 0) {
        ngx_str_t p0f_fingerprint = format_p0f_to_ngx_str(r, req_ctx, &value);
        if (p0f_fingerprint.len == 0) {
            ngx_log_error(NGX_LOG_INFO, connection->log, 0,
                          "failed to format p0f fingerprint");
            return NGX_DECLINED;
        }

        ngx_log_debug(NGX_LOG_DEBUG_HTTP, connection->log, 0,
                      "found p0f fingerprint: %V", &p0f_fingerprint);

        req_ctx->p0f = value;
        req_ctx->p0f_fingerprint = p0f_fingerprint;

        return NGX_OK;
    } else {
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, connection->log, 0,
                      "p0f fingerprint not found");
    }
    return NGX_OK;
}

static ngx_int_t ngx_http_p0f_fingerprint(ngx_http_request_t* r,
                                          ngx_http_variable_value_t* v,
                                          uintptr_t data) {
    ngx_http_p0f_main_conf_t* mod =
        ngx_http_get_module_main_conf(r, ngx_http_p0f_module);
    ngx_str_t fingerprint = ngx_null_string;
    ngx_int_t result = NGX_OK;

    ngx_connection_t* connection = r->connection;
#if (NGX_HTTP_V2)
    if (r->stream) {
        connection = r->stream->connection->connection;
    }
#endif

    if (mod->enabled > 0) {
        ngx_http_p0f_req_ctx_t req_ctx = {0};

        struct sockaddr* const local = connection->local_sockaddr;
        struct sockaddr* const remote = connection->sockaddr;

        const in_port_t remote_port = ngx_inet_get_port(remote);
        const in_port_t local_port = ngx_inet_get_port(local);

        p0f_key_t key = {.ports = {{0}, remote_port, local_port}};

        switch (remote->sa_family) {
            case AF_INET6: {
                struct sockaddr_in6* remote_ipv6 = (struct sockaddr_in6*)remote;
                key.ip6.src_addr = remote_ipv6->sin6_addr;
                break;
            }
            case AF_INET:
            default: {
                struct sockaddr_in* remote_ip = (struct sockaddr_in*)remote;
                key.ip.src_addr = remote_ip->sin_addr;
                break;
            }
        }

        result = ngx_http_p0f_extract_fingerprint(r, &req_ctx, mod, &key);
        fingerprint = req_ctx.p0f_fingerprint;
    } else {
        ngx_log_error(
            NGX_LOG_EMERG, connection->log, 0,
            "You're trying to access $p0f_fingerprint variable while "
            "module p0f is disabled. Please check your configuration, "
            "it should contain \"http { p0f on; }\" to use $p0f_fingerprint.");
    }

    v->no_cacheable = 1;
    if (fingerprint.data != NULL) {
        v->data = ngx_pnalloc(r->pool, fingerprint.len);
        if (v->data == NULL) {
            ngx_log_error(NGX_LOG_EMERG, connection->log, 0,
                          "allocation of value data failed");
            return NGX_ERROR;
        }
        ngx_memcpy(v->data, fingerprint.data, fingerprint.len);
        v->len = fingerprint.len;
        v->valid = 1;
        v->not_found = 0;
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, connection->log, 0,
                      "set valid $p0f_fingerprint value data: %V", &fingerprint);
    } else {
        v->valid = 0;
        v->not_found = 1;
        ngx_log_debug(NGX_LOG_DEBUG_HTTP, connection->log, 0,
                      "$p0f_fingerprint value data is invalid");
    }

    return result;
}
