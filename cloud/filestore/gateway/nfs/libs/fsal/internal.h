#pragma once

#include <contrib/restricted/nfs_ganesha/src/include/config.h>
#include <contrib/restricted/nfs_ganesha/src/include/fsal.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct yfs_service;

struct yfs_fsal_module;
struct yfs_fsal_export;
struct yfs_fsal_obj_handle;

////////////////////////////////////////////////////////////////////////////////

struct yfs_fsal_module {
    struct fsal_module module;

    // configuration parameters
    const char *config_path;
    const char *filesystem_id;
    const char *client_id;
};

#define YFS_MODULE_FROM_FSAL(fsal) \
    container_of((fsal), struct yfs_fsal_module, module)

void yfs_module_ops_init(struct fsal_ops *ops);

////////////////////////////////////////////////////////////////////////////////

struct yfs_fsal_export {
    struct fsal_export export;
    struct fsal_obj_ops handle_ops;

    // filestore API
    struct yfs_service *service;

    // exported filesystem root
    char *root_path;
    struct yfs_fsal_obj_handle *root_handle;
};

#define YFS_EXPORT_FROM_FSAL(fsal) \
    container_of((fsal), struct yfs_fsal_export, export)

void yfs_export_ops_init(struct export_ops *ops);

struct yfs_fsal_export *yfs_alloc_export();

////////////////////////////////////////////////////////////////////////////////

struct yfs_fsal_host_handle {
    uint64_t handle;
    uint64_t ino;
};

struct yfs_fsal_obj_handle {
    struct fsal_obj_handle obj_handle;

    // link to parent node
    struct fsal_obj_handle *parent;

    //
    struct yfs_fsal_host_handle host_handle;

    // node attributes
    char *name;
    struct attrlist attrs;
    struct fsal_share share;
};

#define YFS_HANDLE_FROM_FSAL(fsal) \
    container_of((fsal), struct yfs_fsal_obj_handle, obj_handle)

void yfs_handle_ops_init(struct fsal_obj_ops *ops);

struct yfs_fsal_obj_handle *yfs_alloc_handle(
    struct yfs_fsal_export *exp,
    struct yfs_fsal_obj_handle *parent,
    const char *name,
    struct attrlist *attrs);

void yfs_free_handle(struct yfs_fsal_obj_handle *hdl);

////////////////////////////////////////////////////////////////////////////////

fsal_errors_t yfs_service2fsal_error(int *retval);

#if defined(__cplusplus)
}   // extern "C"
#endif
