#include "service.h"

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

TFileStoreService::TFileStoreService(
        ILoggingServicePtr logging,
        NClient::ISessionPtr session,
        TFileStoreServiceConfigPtr config)
    : Logging(std::move(logging))
    , Session(std::move(session))
    , Config(std::move(config))
{
    Log = Logging->CreateLog("NFS");

    Init(this);
}

void TFileStoreService::Start()
{
    auto future = Session->CreateSession();

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        // TODO
    }
}

void TFileStoreService::Stop()
{
    auto future = Session->DestroySession();

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        // TODO
    }
}

static ELogPriority GetErrorPriority(ui32 code)
{
    if (FACILITY_FROM_CODE(code) == FACILITY_FILESTORE) {
        return TLOG_DEBUG;
    } else {
        return TLOG_ERR;
    }
}

int TFileStoreService::CheckError(const NProto::TError& error) const
{
    if (HasError(error)) {
        STORAGE_LOG(GetErrorPriority(error.GetCode()),
            "request failed: " << FormatError(error));
        return -ErrnoFromError(error.GetCode());
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////

template <typename Method, typename... Args>
int TFileStoreService::Call(Method&& m, yfs_service *svc, Args&&... args) noexcept
{
    auto* pThis = static_cast<TFileStoreService*>(svc);
    auto& Log = pThis->Log;
    try {
        return (pThis->*m)(std::forward<Args>(args)...);
    } catch (const TServiceError& e) {
        STORAGE_ERROR("unexpected error: "
            << FormatResultCode(e.GetCode()) << " " << e.GetMessage());
        return -ErrnoFromError(e.GetCode());
    } catch (...) {
        STORAGE_ERROR("unexpected error: " << CurrentExceptionMessage());
        return -EFAULT;
    }
}

void TFileStoreService::Init(yfs_service *svc)
{
#define CALL(m, svc, ...) \
    TFileStoreService::Call(&TFileStoreService::m, svc, __VA_ARGS__)

    //
    // FileSystem information
    //

    svc->statfs = [] (struct yfs_service *svc, uint64_t ino, struct statfs *stat) {
        return CALL(StatFs, svc, ino, stat);
    };

    //
    // Cluster state
    //

    svc->readnodes = [] (struct yfs_service *svc, struct yfs_readnodes_cb *cb) {
        return CALL(ReadNodes, svc, cb);
    };
    svc->addclient = [] (struct yfs_service *svc, const char *node_id, const struct yfs_cluster_client *client) {
        return CALL(AddClient, svc, node_id, client);
    };
    svc->removeclient = [] (struct yfs_service *svc, const char *node_id, const char *client_id) {
        return CALL(RemoveClient, svc, node_id, client_id);
    };
    svc->readclients = [] (struct yfs_service *svc, const char *node_id, struct yfs_readclients_cb *cb) {
        return CALL(ReadClients, svc, node_id, cb);
    };
    svc->updatecluster = [] (struct yfs_service *svc, const char *node_id, uint32_t update) {
        return CALL(UpdateCluster, svc, node_id, update);
    };

    //
    // Nodes
    //

    svc->lookup = [] (yfs_service *svc, uint64_t parent, const char *name, struct stat *attr) {
        return CALL(Lookup, svc, parent, name, attr);
    };
    svc->mkdir = [] (yfs_service *svc, uint64_t parent, const char *name, mode_t mode, struct stat *attr) {
        return CALL(MkDir, svc, parent, name, mode, attr);
    };
    svc->rmdir = [] (yfs_service *svc, uint64_t parent, const char *name) {
        return CALL(RmDir, svc, parent, name);
    };
    svc->mknode = [] (yfs_service *svc, uint64_t parent, const char *name, mode_t mode, dev_t rdev, struct stat *attr) {
        return CALL(MkNode, svc, parent, name, mode, rdev, attr);
    };
    svc->unlink = [] (yfs_service *svc, uint64_t parent, const char *name) {
        return CALL(Unlink, svc, parent, name);
    };
    svc->rename = [] (yfs_service *svc, uint64_t parent, const char *name, uint64_t newparent, const char *newname) {
        return CALL(Rename, svc, parent, name, newparent, newname);
    };
    svc->symlink = [] (yfs_service *svc, uint64_t parent, const char *name, const char *link, struct stat *attr) {
        return CALL(SymLink, svc, parent, name, link, attr);
    };
    svc->link = [] (yfs_service *svc, uint64_t parent, const char *name, uint64_t ino, struct stat *attr) {
        return CALL(Link, svc, parent, name, ino, attr);
    };
    // svc->readlink = [] (yfs_service *svc, uint64_t ino, char *buffer, uint32_t size) {
    //     return CALL(ReadLink, svc, ino, TStringBuf(buffer, size));
    // };
    // svc->access = [] (yfs_service *svc, uint64_t ino, int mask) {
    //     return CALL(Access, svc, ino, mask);
    // };

    //
    // Node attributes
    //

    svc->getattr = [] (yfs_service *svc, uint64_t ino, struct stat *attr) {
        return CALL(GetAttr, svc, ino, attr);
    };
    svc->setattr = [] (yfs_service *svc, uint64_t ino, const struct stat *attr, uint32_t flags) {
        return CALL(SetAttr, svc, ino, attr, flags);
    };

    //
    // Extended node attributes
    //

    // svc->setxattr = [] (yfs_service *svc, uint64_t ino, const char *name, const char *buffer, uint32_t size, uint32_t flags) {
    //     return CALL(SetXAttr, svc, ino, name, TStringBuf(buffer, size), flags);
    // };
    // svc->getxattr = [] (yfs_service *svc, uint64_t ino, const char *name, char *buffer, uint32_t size) {
    //     return CALL(GetXAttr, svc, ino, name, TStringBuf(buffer, size));
    // };
    // svc->listxattr = [] (yfs_service *svc, uint64_t ino, char *buffer, uint32_t size) {
    //     return CALL(ListXAttr, svc, ino, TStringBuf(buffer, size));
    // };
    // svc->removexattr = [] (yfs_service *svc, uint64_t ino, const char *name) {
    //     return CALL(RemoveXAttr, svc, ino, name);
    // };

    //
    // Directory listing
    //

    svc->opendir = [] (yfs_service *svc, uint64_t ino, void **handle) {
        return CALL(OpenDir, svc, ino, handle);
    };
    svc->readdir = [] (struct yfs_service *svc, void *handle, struct yfs_readdir_cb *cb, uint64_t offset) {
        return CALL(ReadDir, svc, handle, cb, offset);
    };
    svc->releasedir = [] (yfs_service *svc, void *handle) {
        return CALL(ReleaseDir, svc, handle);
    };

    //
    // Read & write files
    //

    // svc->create = [] (yfs_service *svc, uint64_t parent, const char *name, mode_t mode, void **handle) {
    //     return CALL(Create, svc, parent, name, mode, handle);
    // };
    // svc->open = [] (yfs_service *svc, uint64_t ino, void **handle) {
    //     return CALL(Open, svc, ino, handle);
    // };
    // svc->read = [] (yfs_service *svc, void *handle, char *buffer, uint32_t size, uint64_t offset) {
    //     return CALL(Read, svc, handle, TStringBuf(buffer, size), offset);
    // };
    // svc->write = [] (yfs_service *svc, void *handle, const char *buffer, uint32_t size, uint64_t offset) {
    //     return CALL(Write, svc, handle, TStringBuf(buffer, size), offset);
    // };
    // svc->release = [] (yfs_service *svc, void *handle) {
    //     return CALL(Release, svc, handle);
    // };

    //
    // Locking
    //

    // svc->getlock = [] (yfs_service *svc, uint64_t ino, struct flock *lock) {
    //     return CALL(GetLock, svc, ino, lock);
    // };
    // svc->setlock = [] (yfs_service *svc, uint64_t ino, struct flock *lock, int sleep) {
    //     return CALL(SetLock, svc, ino, lock, sleep);
    // };
    // svc->flock = [] (yfs_service *svc, uint64_t ino, int op) {
    //     return CALL(FLock, svc, ino, op);
    // };

#undef CALL
}

}   // namespace NCloud::NFileStore::NGateway
