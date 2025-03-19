#include "internal.h"

#include <contrib/restricted/nfs_ganesha/src/include/FSAL/fsal_init.h>

////////////////////////////////////////////////////////////////////////////////

static const char yfs_recovery_backend_name[] = "YFS";

static struct yfs_recovery_backend yfs_recovery_backend = {
};

struct yfs_recovery_backend *yfs_backend()
{
    return &yfs_recovery_backend;
}

////////////////////////////////////////////////////////////////////////////////

MODULE_INIT void yfs_recovery_init()
{
    LogDebug(COMPONENT_CLIENTID, "YFS recovery backend initializing");

    int retval = register_recovery_backend(
        &yfs_recovery_backend.backend,
        yfs_recovery_backend_name);

    if (retval < 0) {
        LogFatal(COMPONENT_CLIENTID, "YFS recovery backend failed to register.");
    }

    yfs_recovery_backend_ops_init(&yfs_recovery_backend.backend);
}

MODULE_FINI void yfs_recovery_finish()
{
    LogDebug(COMPONENT_CLIENTID, "YFS recovery backend finishing.");

    int retval = unregister_recovery_backend(&yfs_recovery_backend.backend);
    if (retval < 0) {
        LogFatal(COMPONENT_CLIENTID, "Unable to unload YFS recovery backend.");
    }
}
