#include "internal.h"

#include <cloud/filestore/gateway/nfs/libs/api/service.h>

#include <contrib/restricted/nfs_ganesha/src/include/config_parsing.h>
#include <contrib/restricted/nfs_ganesha/src/include/display.h>

#include <ctype.h>
#include <stdio.h>

////////////////////////////////////////////////////////////////////////////////

static struct config_item yfs_backend_params[] = {
    CONF_ITEM_PATH("config_path", 1, MAXPATHLEN, NULL, yfs_recovery_backend, config_path),
    CONF_ITEM_STR("filesystem", 1, MAXPATHLEN, NULL, yfs_recovery_backend, filesystem_id),
    CONF_ITEM_STR("client", 1, MAXPATHLEN, NULL, yfs_recovery_backend, client_id),
    CONFIG_EOL
};

static struct config_block yfs_backend_param_block = {
    .blk_desc.name = "YFS",
    .blk_desc.type = CONFIG_BLOCK,
    .blk_desc.u.blk.init = noop_conf_init,
    .blk_desc.u.blk.params = yfs_backend_params,
    .blk_desc.u.blk.commit = noop_conf_commit
};

////////////////////////////////////////////////////////////////////////////////

static bool yfs_is_printable(const char *value, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        if (!isprint(value[i]) || (value[i] == '/')) {
            return false;
        }
    }
    return true;
}

static int yfs_convert_opaque_val(
    struct display_buffer *buf,
    const char *value,
    size_t len)
{
    int b_left = display_start(buf);
    if (b_left <= 0) {
        return 0;
    }

    // If the value is empty, display EMPTY value
    if (len <= 0 || len > (size_t) b_left) {
        return 0;
    }

    // If the value is NULL, display NULL value
    if (value == NULL) {
        return 0;
    }

    // Determine if the value is entirely printable characters,
    // and it contains no slash character (reserved for filename)
    bool is_printable = yfs_is_printable(value, len);
    if (is_printable) {
        b_left = display_len_cat(buf, value, len);
    } else {
        b_left = display_opaque_bytes(buf, (char *) value, len);
    }

    if (b_left <= 0) {
        return 0;
    }

    return b_left;
}

static void yfs_generate_client_key(
    const nfs_client_id_t *client_id,
    char *buf,
    size_t len)
{
    (void) snprintf(buf, len, "%lu", (uint64_t) client_id->cid_clientid);
}

static char *yfs_generate_client_tag(
    const nfs_client_id_t *client_id,
    size_t* len)
{
    // get the caller's IP addr
    const char *client_addr_str;
    if (client_id->gsh_client != NULL) {
        client_addr_str = client_id->gsh_client->hostaddr_str;
    } else {
        client_addr_str = "(unknown)";
    }

    size_t client_addr_str_len = strlen(client_addr_str);

    // encode cid
    char cid_str[MAXPATHLEN];

    struct display_buffer buf = {sizeof(cid_str), cid_str, cid_str};
    (void) yfs_convert_opaque_val(
        &buf,
        client_id->cid_client_record->cr_client_val,
        client_id->cid_client_record->cr_client_val_len);

    size_t cid_str_len = display_buffer_len(&buf);

    // combine all together
    size_t total_len = client_addr_str_len + 1 + cid_str_len + 1;
    char *str = gsh_malloc(total_len);

    (void) snprintf(str, total_len, "%s;%s", client_addr_str, cid_str);

    if (len) {
        *len = total_len;
    }
    return str;
}

////////////////////////////////////////////////////////////////////////////////

static int yfs_recovery_load_config(
    config_file_t parse_tree,
    struct config_error_type *err_type)
{
    LogDebug(COMPONENT_CLIENTID, "Setup YFS recovery backend.");

    return load_config_from_parse(
        parse_tree,
        &yfs_backend_param_block,
        yfs_backend(),
        true,
        err_type);
}

static int yfs_recovery_init()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Init YFS recovery backend.");

    int retval = YFS_CALL(create,
        yfs_service_factory_get(),
        backend->config_path,
        backend->filesystem_id,
        backend->client_id,
        &backend->service);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to init YFS backend");
    }

    return retval;
}

static void yfs_recovery_shutdown()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Shutdown YFS recovery backend.");

    if (backend->service != NULL) {
        (void) YFS_CALL(destroy, yfs_service_factory_get(), backend->service);
        backend->service = NULL;
    }
}

struct yfs_read_clids_state {
    struct yfs_readclients_cb cb;
    add_clid_entry_hook add_clid;
    add_rfh_entry_hook add_rfh;
};

static int yfs_read_clids_callback(
    struct yfs_readclients_cb *cb,
    const struct yfs_cluster_client *client)
{
    struct yfs_read_clids_state *state = container_of(cb, struct yfs_read_clids_state, cb);

    bool is_printable = yfs_is_printable(client->opaque, client->opaque_len);
    if (is_printable && (client->opaque_len < PATH_MAX)) {
        struct clid_entry *entry = state->add_clid((char *) client->opaque);
        // TODO
    } else {
        LogEvent(COMPONENT_CLIENTID, "Invalid client entry found: %s",
            client->client_id);
    }

    return 0;
}

static void yfs_read_clids(
    nfs_grace_start_t *gsp,
    add_clid_entry_hook add_clid,
    add_rfh_entry_hook add_rfh)
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Start grace period");

    int retval = YFS_CALL(updatecluster,
        backend->service,
        backend->client_id,
        E_CLUSTER_START_GRACE);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to update cluster state");
        return;
    }

    LogDebug(COMPONENT_CLIENTID, "Read clients");

    struct yfs_read_clids_state state = {
        .cb.invoke = yfs_read_clids_callback,
        .add_clid = add_clid,
        .add_rfh = add_rfh,
    };

    retval = YFS_CALL(readclients,
        backend->service,
        backend->client_id,
        &state.cb);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to read clients");
        return;
    }
}

static void yfs_add_clid(nfs_client_id_t *client_id)
{
    struct yfs_recovery_backend *backend = yfs_backend();

    char key[MAXPATHLEN];
    yfs_generate_client_key(client_id, key, sizeof(key));

    size_t recov_tag_len;
    char *recov_tag = yfs_generate_client_tag(client_id, &recov_tag_len);

    LogDebug(COMPONENT_CLIENTID, "Store client %s :: %s", key, recov_tag);

    struct yfs_cluster_client client = {
        .client_id = key,
        .opaque = recov_tag,
        .opaque_len = recov_tag_len - 1,    // not counting null character
    };

    int retval = YFS_CALL(addclient,
        backend->service,
        backend->client_id,
        &client);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to store client %lu",
            client_id->cid_clientid);
        gsh_free(recov_tag);
        return;
    }

    client_id->cid_recov_tag = recov_tag;
}

static void yfs_rm_clid(nfs_client_id_t *client_id)
{
    struct yfs_recovery_backend *backend = yfs_backend();

    char key[MAXPATHLEN];
    yfs_generate_client_key(client_id, key, sizeof(key));

    LogDebug(COMPONENT_CLIENTID, "Remove client %s", key);

    int retval = YFS_CALL(removeclient,
        backend->service,
        backend->client_id,
        key);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to remove client %lu",
            client_id->cid_clientid);
        return;
    }

    if (client_id->cid_recov_tag) {
        gsh_free(client_id->cid_recov_tag);
        client_id->cid_recov_tag = NULL;
    }
}

static void yfs_add_revoke_fh(nfs_client_id_t *client_id, nfs_fh4 *fh)
{
    LogDebug(COMPONENT_CLIENTID, "yfs_add_revoke_fh");

    // TODO
}

static void yfs_end_grace()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Stop grace period");

    int retval = YFS_CALL(updatecluster,
        backend->service,
        backend->client_id,
        E_CLUSTER_STOP_GRACE);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to update cluster state");
        return;
    }
}

struct yfs_readnodes_state {
    struct yfs_readnodes_cb cb;
    const char *node_id;
    bool is_member;
    size_t total_nodes;
    size_t need_recovery;
    size_t grace_enforcing;
};

static int yfs_readnodes_callback(
    struct yfs_readnodes_cb *cb,
    const struct yfs_cluster_node *node)
{
    struct yfs_readnodes_state *state = container_of(cb, struct yfs_readnodes_state, cb);

    if (strcmp(state->node_id, node->node_id) == 0) {
        state->is_member = true;
    }

    ++state->total_nodes;

    if (node->flags & F_NODE_NEED_RECOVERY) {
        ++state->need_recovery;
    }
    if (node->flags & F_NODE_GRACE_ENFORCING) {
        ++state->grace_enforcing;
    }

    return 0;
}

static void yfs_maybe_start_grace()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Check if local grace period should be started");

    struct yfs_readnodes_state state = {
        .cb.invoke = yfs_readnodes_callback,
        .node_id = backend->client_id,
    };

    int retval = YFS_CALL(readnodes, backend->service, &state.cb);
    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to query cluster state");
        return;
    }

    if (state.need_recovery) {
        nfs_grace_start_t gsp = { .event = EVENT_JUST_GRACE };
        nfs_start_grace(&gsp);
    }

    LogDebug(COMPONENT_CLIENTID, "Persist confirmed clients");
}

static bool yfs_try_lift_grace()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Check if grace period could be lifted");

    struct yfs_readnodes_state state = {
        .cb.invoke = yfs_readnodes_callback,
        .node_id = backend->client_id,
    };

    int retval = YFS_CALL(readnodes, backend->service, &state.cb);
    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to query cluster state");
        return false;
    }

    return state.need_recovery == 0;
}

static void yfs_set_grace_enforcing()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Set grace enforcing");

    int retval = YFS_CALL(updatecluster,
        backend->service,
        backend->client_id,
        E_CLUSTER_JOIN_GRACE);

    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to update cluster state");
        return;
    }
}

static bool yfs_check_grace_enforcing()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Check if grace enforced by cluster");

    struct yfs_readnodes_state state = {
        .cb.invoke = yfs_readnodes_callback,
        .node_id = backend->client_id,
    };

    int retval = YFS_CALL(readnodes, backend->service, &state.cb);
    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to query cluster state");
        return false;
    }

    return state.grace_enforcing == state.total_nodes;
}

static bool yfs_is_member()
{
    struct yfs_recovery_backend *backend = yfs_backend();

    LogDebug(COMPONENT_CLIENTID, "Check for cluster membership");

    struct yfs_readnodes_state state = {
        .cb.invoke = yfs_readnodes_callback,
        .node_id = backend->client_id,
    };

    int retval = YFS_CALL(readnodes, backend->service, &state.cb);
    if (retval < 0) {
        LogEvent(COMPONENT_CLIENTID, "Failed to query cluster state");
        return false;
    }

    return state.is_member;
}

static int yfs_get_nodeid(char **pnodeid)
{
    *pnodeid = gsh_strdup(yfs_backend()->client_id);
    return 0;
}

void yfs_recovery_backend_ops_init(struct nfs4_recovery_backend *backend)
{
    backend->load_config_from_parse = yfs_recovery_load_config;
    backend->recovery_init = yfs_recovery_init;
    backend->recovery_shutdown = yfs_recovery_shutdown;
    backend->recovery_read_clids = yfs_read_clids;
    backend->add_clid = yfs_add_clid;
    backend->rm_clid = yfs_rm_clid;
    backend->add_revoke_fh = yfs_add_revoke_fh;
    backend->end_grace = yfs_end_grace;
    backend->maybe_start_grace = yfs_maybe_start_grace;
    backend->try_lift_grace = yfs_try_lift_grace;
    backend->set_enforcing = yfs_set_grace_enforcing;
    backend->grace_enforcing = yfs_check_grace_enforcing;
    backend->is_member = yfs_is_member;
    backend->get_nodeid = yfs_get_nodeid;
}
