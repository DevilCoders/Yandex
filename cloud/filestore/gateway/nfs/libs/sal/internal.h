#pragma once

#include <contrib/restricted/nfs_ganesha/src/include/config.h>
#include <contrib/restricted/nfs_ganesha/src/include/sal_functions.h>

#if defined(__cplusplus)
extern "C" {
#endif

struct yfs_service;

////////////////////////////////////////////////////////////////////////////////

struct yfs_recovery_backend {
    struct nfs4_recovery_backend backend;

    // configuration parameters
    const char *config_path;
    const char *filesystem_id;
    const char *client_id;

    // filestore API
    struct yfs_service *service;
};

struct yfs_recovery_backend *yfs_backend();

void yfs_recovery_backend_ops_init(struct nfs4_recovery_backend *backend);

#if defined(__cplusplus)
}   // extern "C"
#endif
