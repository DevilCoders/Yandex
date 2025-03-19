#include "internal.h"

#include <contrib/restricted/nfs_ganesha/src/include/FSAL/fsal_init.h>

////////////////////////////////////////////////////////////////////////////////

static const char yfs_fsal_name[] = "YFS";

static struct yfs_fsal_module yfs_fsal = {
    .module = {
        .fs_info = {
            // limits
            .maxfilesize = INT64_MAX,
            .maxlink = 0,
            .maxnamelen = MAXNAMLEN,
            .maxpathlen = MAXPATHLEN,
            .maxread = FSAL_MAXIOSIZE,
            .maxwrite = FSAL_MAXIOSIZE,
            // features
            .acl_support = false,
            .cansettime = true,
            .case_insensitive = false,
            .case_preserving = true,
            .chown_restricted = true,
            .homogenous = true,
            .link_support = true,
            .lock_support = true,
            .named_attr = true,
            .supported_attrs = ATTRS_POSIX,
            .symlink_support = true,
            .unique_handles = true,
        }
    }
};

////////////////////////////////////////////////////////////////////////////////

MODULE_INIT void yfs_init()
{
    LogDebug(COMPONENT_FSAL, "YFS module initializing.");

    int retval = register_fsal(
        &yfs_fsal.module,
        yfs_fsal_name,
        FSAL_MAJOR_VERSION,
        FSAL_MINOR_VERSION,
        FSAL_ID_NO_PNFS);

    if (retval < 0) {
        LogFatal(COMPONENT_FSAL, "YFS module failed to register.");
    }

    yfs_module_ops_init(&yfs_fsal.module.m_ops);
}

MODULE_FINI void yfs_finish()
{
    LogDebug(COMPONENT_FSAL, "YFS module finishing.");

    int retval = unregister_fsal(&yfs_fsal.module);
    if (retval < 0) {
        LogFatal(COMPONENT_FSAL, "Unable to unload YFS module.");
    }
}
