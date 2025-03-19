#include "internal.h"

#include <cloud/filestore/gateway/nfs/libs/api/service.h>

#include <contrib/restricted/nfs_ganesha/src/include/FSAL/fsal_commonlib.h>

////////////////////////////////////////////////////////////////////////////////

struct yfs_fsal_export *yfs_alloc_export()
{
    struct yfs_fsal_export *exp = gsh_calloc(1, sizeof(struct yfs_fsal_export));

    fsal_export_init(&exp->export);

    yfs_export_ops_init(&exp->export.exp_ops);
    yfs_handle_ops_init(&exp->handle_ops);

    LogDebug(COMPONENT_FSAL, "Allocating exp=%p", exp);
    return exp;
}

static void yfs_release_export(struct fsal_export *exp_hdl)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(exp_hdl);

    LogDebug(COMPONENT_FSAL, "Releasing exp=%p", exp);

    fsal_detach_export(exp->export.fsal, &exp->export.exports);
    free_export_ops(&exp->export);

    if (exp->root_handle != NULL) {
        yfs_free_handle(exp->root_handle);
        exp->root_handle = NULL;
    }

    if (exp->service != NULL) {
        (void) YFS_CALL(destroy, yfs_service_factory_get(), exp->service);
        exp->service = NULL;
    }

    gsh_free(exp->root_path);
    gsh_free(exp);
}

static fsal_status_t yfs_lookup_path(
    struct fsal_export *exp_hdl,
    const char *path,
    struct fsal_obj_handle **obj_hdl,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(exp_hdl);

    if (strcmp(path, exp->root_path) != 0) {
        LogCrit(COMPONENT_FSAL, "Attempt to lookup non-root path %s", path);
        return fsalstat(ERR_FSAL_NOENT, ENOENT);
    }

    if (exp->root_handle == NULL) {
        struct attrlist attrs;
        attrs.valid_mask = ATTR_FILEID | ATTR_TYPE | ATTR_MODE;
        attrs.fileid = E_ROOT_NODE_ID;
        attrs.type = DIRECTORY;
        attrs.mode = 0777;
        // TODO

        exp->root_handle = yfs_alloc_handle(exp, NULL, exp->root_path, &attrs);
    }

    *obj_hdl = &exp->root_handle->obj_handle;

    if (attrs_out != NULL) {
        fsal_copy_attrs(attrs_out, &exp->root_handle->attrs, false);
    }

    return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

static fsal_status_t yfs_wire_to_host(
    struct fsal_export *exp_hdl,
    fsal_digesttype_t in_type,
    struct gsh_buffdesc *fh_desc,
    int flags)
{
    struct yfs_fsal_host_handle *hhdl = fh_desc->addr;

    switch (in_type) {
        case FSAL_DIGEST_NFSV3:
        case FSAL_DIGEST_NFSV4:
            fh_desc->len = sizeof(struct yfs_fsal_host_handle);

            // convert byte order to host
            hhdl->handle = le64toh(hhdl->handle);
            hhdl->ino = le64toh(hhdl->ino);
            break;

        default:
            return fsalstat(ERR_FSAL_SERVERFAULT, 0);
    }

    return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

static fsal_status_t yfs_create_handle(
    struct fsal_export *exp_hdl,
    struct gsh_buffdesc *fh_desc,
    struct fsal_obj_handle **handle,
    struct attrlist *attrs_out)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(exp_hdl);
    struct yfs_fsal_host_handle *hhdl = fh_desc->addr;

    if (fh_desc->len != sizeof(struct yfs_fsal_host_handle)) {
        return fsalstat(ERR_FSAL_INVAL, 0);
    }

    // TODO
    printf("yfs_create_handle\n");
    return fsalstat(ERR_FSAL_NOTSUPP, 0);
}

static fsal_status_t yfs_get_dynamic_info(
    struct fsal_export *exp_hdl,
    struct fsal_obj_handle *obj_hdl,
    fsal_dynamicfsinfo_t *info)
{
    struct yfs_fsal_export *exp = YFS_EXPORT_FROM_FSAL(exp_hdl);

    fsal_errors_t fsal_error = ERR_FSAL_NO_ERROR;
    int retval = 0;

    LogDebug(COMPONENT_FSAL, "GetDynamicInfo");

    struct statfs st = {};
    retval = YFS_CALL(statfs, exp->service, E_ROOT_NODE_ID, &st);

    if (retval < 0) {
        fsal_error = yfs_service2fsal_error(&retval);
        goto out;
    }

    memset(info, 0, sizeof(fsal_dynamicfsinfo_t));
    // TODO

out:
    return fsalstat(fsal_error, retval);
}

static struct state_t *yfs_alloc_state(
    struct fsal_export *exp_hdl,
    enum state_type state_type,
    struct state_t *related_state)
{
    // TODO
    printf("yfs_alloc_state\n");
    return NULL;
}

static void yfs_free_state(struct fsal_export *exp_hdl, struct state_t *state)
{
    // TODO
    printf("yfs_free_state\n");
}

void yfs_export_ops_init(struct export_ops *ops)
{
    ops->release = yfs_release_export;
    ops->lookup_path = yfs_lookup_path;
    ops->wire_to_host = yfs_wire_to_host;
    ops->create_handle = yfs_create_handle;
    ops->get_fs_dynamic_info = yfs_get_dynamic_info;
    ops->alloc_state = yfs_alloc_state;
    ops->free_state = yfs_free_state;
}
