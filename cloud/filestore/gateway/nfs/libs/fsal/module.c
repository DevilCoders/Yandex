#include "internal.h"

#include <cloud/filestore/gateway/nfs/libs/api/service.h>

#include <contrib/restricted/nfs_ganesha/src/include/FSAL/fsal_commonlib.h>
#include <contrib/restricted/nfs_ganesha/src/include/config_parsing.h>
#include <contrib/restricted/nfs_ganesha/src/include/fsal_convert.h>

////////////////////////////////////////////////////////////////////////////////
// Module configuration parameters

static struct config_item yfs_module_params[] = {
    CONF_ITEM_PATH("config_path", 1, MAXPATHLEN, NULL, yfs_fsal_module, config_path),
    CONF_ITEM_STR("filesystem", 1, MAXPATHLEN, NULL, yfs_fsal_module, filesystem_id),
    CONF_ITEM_STR("client", 1, MAXPATHLEN, NULL, yfs_fsal_module, client_id),
    CONFIG_EOL
};

static struct config_block yfs_module_param_block = {
    .blk_desc.name = "YFS",
    .blk_desc.type = CONFIG_BLOCK,
    .blk_desc.u.blk.init = noop_conf_init,
    .blk_desc.u.blk.params = yfs_module_params,
    .blk_desc.u.blk.commit = noop_conf_commit
};

////////////////////////////////////////////////////////////////////////////////
// Export configuration parameters

static struct config_item yfs_export_params[] = {
    CONF_ITEM_NOOP("name"),
    CONFIG_EOL
};

static struct config_block yfs_export_param_block = {
    .blk_desc.name = "FSAL",
    .blk_desc.type = CONFIG_BLOCK,
    .blk_desc.u.blk.init = noop_conf_init,
    .blk_desc.u.blk.params = yfs_export_params,
    .blk_desc.u.blk.commit = noop_conf_commit
};

////////////////////////////////////////////////////////////////////////////////

static fsal_status_t yfs_init_config(
    struct fsal_module *fsal_hdl,
    config_file_t config_struct,
    struct config_error_type *err_type)
{
    struct yfs_fsal_module *mod = YFS_MODULE_FROM_FSAL(fsal_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "Setup YFS module.");

    retval = load_config_from_parse(
        config_struct,
        &yfs_module_param_block,
        mod,
        true,
        err_type);

    if (retval < 0) {
        retval = EINVAL;
        fsal_error = posix2fsal_error(retval);
        goto out;
    }

    display_fsinfo(&mod->module);

out:
    return fsalstat(fsal_error, retval);
}

static fsal_status_t yfs_create_export(
    struct fsal_module *fsal_hdl,
    void *parse_node,
    struct config_error_type *err_type,
    const struct fsal_up_vector *up_ops)
{
    struct yfs_fsal_module *mod = YFS_MODULE_FROM_FSAL(fsal_hdl);
    struct yfs_fsal_export *exp = yfs_alloc_export();

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "Setup YFS export.");

    retval = load_config_from_node(
        parse_node,
        &yfs_export_param_block,
        exp,
        true,
        err_type);

    if (retval < 0) {
        retval = EINVAL;
        fsal_error = posix2fsal_error(retval);
        goto err_free;
    }

    retval = YFS_CALL(create,
        yfs_service_factory_get(),
        mod->config_path,
        mod->filesystem_id,
        mod->client_id,
        &exp->service);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto err_free;
    }

    retval = fsal_attach_export(fsal_hdl, &exp->export.exports);
    if (retval != 0) {
        fsal_error = posix2fsal_error(retval);
        goto err_free;
    }

    exp->export.fsal = fsal_hdl;
    exp->export.up_ops = up_ops;

    // save the root path
    exp->root_path = gsh_strdup(op_ctx->ctx_export->fullpath);

    op_ctx->fsal_export = &exp->export;
    return fsalstat(fsal_error, retval);

err_free:
    free_export_ops(&exp->export);
    gsh_free(exp);

    return fsalstat(fsal_error, retval);
}

void yfs_module_ops_init(struct fsal_ops *ops)
{
    ops->init_config = yfs_init_config;
    ops->create_export = yfs_create_export;
}
