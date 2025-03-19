#pragma once

#include "public.h"

#include "config.h"
#include "convert.h"

#include <cloud/filestore/gateway/nfs/libs/api/service.h>

#include <cloud/filestore/libs/client/session.h>
#include <cloud/filestore/libs/service/context.h>
#include <cloud/filestore/libs/service/request.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/startable.h>
#include <cloud/storage/core/libs/diagnostics/logging.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

class TFileStoreService final
    : public yfs_service
    , public IStartable
{
private:
    const ILoggingServicePtr Logging;
    const NClient::ISessionPtr Session;
    const TFileStoreServiceConfigPtr Config;

    TLog Log;

    bool ShouldStop = false;

public:
    TFileStoreService(
        ILoggingServicePtr logging,
        NClient::ISessionPtr session,
        TFileStoreServiceConfigPtr config);

    //
    // IStartable
    //

    void Start() override;
    void Stop() override;

    //
    // FileSystem information
    //

    int StatFs(ui64 ino, struct statfs* stat);

    //
    // Cluster state
    //

    int ReadNodes(yfs_readnodes_cb* cb);
    int AddClient(TString nodeId, const yfs_cluster_client* client);
    int RemoveClient(TString nodeId, TString clientId);
    int ReadClients(TString nodeId, yfs_readclients_cb* cb);
    int UpdateCluster(TString nodeId, ui32 update);

    //
    // Nodes
    //

    int Lookup(ui64 parent, TString name, struct stat* attr);
    int MkDir(ui64 parent, TString name, mode_t mode, struct stat* attr);
    int RmDir(ui64 parent, TString name);
    int MkNode(ui64 parent, TString name, mode_t mode, dev_t rdev, struct stat* attr);
    int Unlink(ui64 parent, TString name);
    int Rename(ui64 parent, TString name, ui64 newparent, TString newname);
    int SymLink(ui64 parent, TString name, TString target, struct stat* attr);
    int Link(ui64 parent, TString name, ui64 ino, struct stat* attr);
    // int ReadLink(ui64 ino, TStringBuf buffer);
    // int Access(ui64 ino, int mask);

    //
    // Node attributes
    //

    int GetAttr(ui64 ino, struct stat* attr);
    int SetAttr(ui64 ino, const struct stat* attr, ui32 flags);

    //
    // Extended node attributes
    //

    // int SetXAttr(ui64 ino, TString name, TStringBuf buffer, ui32 flags);
    // int GetXAttr(ui64 ino, TString name, TStringBuf buffer);
    // int ListXAttr(ui64 ino, TStringBuf buffer);
    // int RemoveXAttr(ui64 ino, TString name);

    //
    // Directory listing
    //

    int OpenDir(ui64 ino, void** handle);
    int ReadDir(void* handle, yfs_readdir_cb* cb, ui64 offset);
    int ReleaseDir(void* handle);

    //
    // Read & write files
    //

    // int Create(ui64 parent, TString name, mode_t mode, void** handle);
    // int Open(ui64 ino, void** handle);
    // int Read(void* handle, TStringBuf buffer, ui64 offset);
    // int Write(void* handle, TStringBuf buffer, ui64 offset);
    // int Release(void* handle);

    //
    // Locking
    //

    // int GetLock(ui64 ino, struct flock* lock);
    // int SetLock(ui64 ino, struct flock* lock, int sleep);
    // int FLock(ui64 ino, int op);

private:
    static void Init(yfs_service* svc);

    template <typename Method, typename... Args>
    static int Call(Method&& m, yfs_service* svc, Args&&... args) noexcept;

    static TCallContextPtr PrepareCallContext()
    {
        return MakeIntrusive<TCallContext>(CreateRequestId());
    }

    template <typename T>
    static std::shared_ptr<T> CreateRequest()
    {
        return std::make_shared<T>();
    }

    template <typename T>
    static std::shared_ptr<T> CreateRequest(ui64 node)
    {
        auto request = CreateRequest<T>();
        request->SetNodeId(node);

        return request;
    }

    template <typename T>
    int CheckResponse(const T& response) const
    {
        return CheckError(response.GetError());
    }

    int CheckError(const NProto::TError& error) const;
};

}   // namespace NCloud::NFileStore::NGateway
