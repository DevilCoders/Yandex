#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <sys/stat.h>
#include <sys/statfs.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define YFS_CALL(method, impl, ...) (impl)->method((impl), __VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////
// Keep this in sync with filestore/public/protos

enum ENodeConstants {
    E_INVALID_NODE_ID   = 0,
    E_ROOT_NODE_ID      = 1,
};

enum ESetAttrFlags {
    F_SET_ATTR_MODE     = 1 << 0,
    F_SET_ATTR_UID      = 1 << 1,
    F_SET_ATTR_GID      = 1 << 2,
    F_SET_ATTR_SIZE     = 1 << 3,
    F_SET_ATTR_ATIME    = 1 << 4,
    F_SET_ATTR_MTIME    = 1 << 5,
};

enum EClusterNodeFlags {
    F_NODE_ONLINE           = 1 << 0,
    F_NODE_NEED_RECOVERY    = 1 << 1,
    F_NODE_GRACE_ENFORCING  = 1 << 2,
};

enum EClusterStateUpdate {
    E_CLUSTER_START_GRACE   = 0,
    E_CLUSTER_STOP_GRACE    = 1,
    E_CLUSTER_JOIN_GRACE    = 2,
};

struct yfs_cluster_node {
    const char *node_id;
    uint32_t flags;
    uint32_t num_clients;
};

struct yfs_cluster_client {
    const char *client_id;
    const char *opaque;
    uint32_t opaque_len;
};

////////////////////////////////////////////////////////////////////////////////

struct yfs_readnodes_cb {
    int (*invoke)(struct yfs_readnodes_cb *cb, const struct yfs_cluster_node *node);
};

struct yfs_readclients_cb {
    int (*invoke)(struct yfs_readclients_cb *cb, const struct yfs_cluster_client *client);
};

struct yfs_readdir_cb {
    int (*invoke)(struct yfs_readdir_cb *cb, const char *name, const struct stat *attr, uint64_t offset);
};

////////////////////////////////////////////////////////////////////////////////

struct yfs_service {
    //
    // FileSystem information
    //

    int (*statfs)(struct yfs_service *svc, uint64_t ino, struct statfs *stat);

    //
    // Cluster state
    //

    int (*readnodes)(struct yfs_service *svc, struct yfs_readnodes_cb *cb);
    int (*addclient)(struct yfs_service *svc, const char *node_id, const struct yfs_cluster_client *client);
    int (*removeclient)(struct yfs_service *svc, const char *node_id, const char *client_id);
    int (*readclients)(struct yfs_service *svc, const char *node_id, struct yfs_readclients_cb *cb);
    int (*updatecluster)(struct yfs_service *svc, const char *node_id, uint32_t update);

    //
    // Nodes
    //

    int (*lookup)(struct yfs_service *svc, uint64_t parent, const char *name, struct stat *attr);
    int (*mkdir)(struct yfs_service *svc, uint64_t parent, const char *name, mode_t mode, struct stat *attr);
    int (*rmdir)(struct yfs_service *svc, uint64_t parent, const char *name);
    int (*mknode)(struct yfs_service *svc, uint64_t parent, const char *name, mode_t mode, dev_t rdev, struct stat *attr);
    int (*unlink)(struct yfs_service *svc, uint64_t parent, const char *name);
    int (*rename)(struct yfs_service *svc, uint64_t parent, const char *name, uint64_t newparent, const char *newname);
    int (*symlink)(struct yfs_service *svc, uint64_t parent, const char *name, const char *link, struct stat *attr);
    int (*link)(struct yfs_service *svc, uint64_t parent, const char *name, uint64_t ino, struct stat *attr);
    // int (*readlink)(struct yfs_service *svc, uint64_t ino, char *buffer, uint32_t size);
    // int (*access)(struct yfs_service *svc, uint64_t ino, int mask);

    //
    // Node attributes
    //

    int (*getattr)(struct yfs_service *svc, uint64_t ino, struct stat *attr);
    int (*setattr)(struct yfs_service *svc, uint64_t ino, const struct stat *attr, uint32_t flags);

    //
    // Extended node attributes
    //

    // int (*setxattr)(struct yfs_service *svc, uint64_t ino, const char *name, const char *buffer, uint32_t size, uint32_t flags);
    // int (*getxattr)(struct yfs_service *svc, uint64_t ino, const char *name, char *buffer, uint32_t size);
    // int (*listxattr)(struct yfs_service *svc, uint64_t ino, char *buffer, uint32_t size);
    // int (*removexattr)(struct yfs_service *svc, uint64_t ino, const char *name);

    //
    // Directory listing
    //

    int (*opendir)(struct yfs_service *svc, uint64_t ino, void **handle);
    int (*readdir)(struct yfs_service *svc, void *handle, struct yfs_readdir_cb *cb, uint64_t offset);
    int (*releasedir)(struct yfs_service *svc, void *handle);

    //
    // Read & write files
    //

    // int (*create)(struct yfs_service *svc, uint64_t parent, const char *name, mode_t mode, void **handle);
    // int (*open)(struct yfs_service *svc, uint64_t ino, void **handle);
    // int (*read)(struct yfs_service *svc, void *handle, char *buffer, uint32_t size, uint64_t offset);
    // int (*write)(struct yfs_service *svc, void *handle, const char *buffer, uint32_t size, uint64_t offset);
    // int (*release)(struct yfs_service *svc, void *handle);

    //
    // Locking
    //

    // int (*getlock)(struct yfs_service *svc, uint64_t ino, struct flock *lock);
    // int (*setlock)(struct yfs_service *svc, uint64_t ino, struct flock *lock, int sleep);
    // int (*flock)(struct yfs_service *svc, uint64_t ino, int op);
};

////////////////////////////////////////////////////////////////////////////////

struct yfs_service_factory {
    //
    // Factory methods
    //

    int (*create)(
        struct yfs_service_factory *sf,
        const char *config_path,
        const char *filesystem_id,
        const char *client_id,
        struct yfs_service **svc);

    int (*destroy)(struct yfs_service_factory *sf, struct yfs_service *svc);
};

////////////////////////////////////////////////////////////////////////////////

void yfs_service_factory_init(struct yfs_service_factory *sf);
struct yfs_service_factory *yfs_service_factory_get();

#if defined(__cplusplus)
}   // extern "C"
#endif
