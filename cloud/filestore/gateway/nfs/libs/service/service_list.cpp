#include "service.h"

namespace NCloud::NFileStore::NGateway {

namespace {

////////////////////////////////////////////////////////////////////////////////

class TDirectoryHandle
{
private:
    const ui64 NodeId;

    NProto::TListNodesResponse Response;
    ui64 PageOffset = 0;
    bool CanFetchNextPage = true;

public:
    TDirectoryHandle(ui64 nodeId)
        : NodeId(nodeId)
    {}

    ui64 GetNodeId() const
    {
        return NodeId;
    }

    const TString& GetCookie() const
    {
        return Response.GetCookie();
    }

    bool MoreDataAvailable() const
    {
        return CanFetchNextPage;
    }

    int ReadDir(yfs_readdir_cb* cb, ui64 offset)
    {
        Y_ENSURE(offset >= PageOffset);
        Y_ENSURE(Response.NodesSize() == Response.NamesSize());

        size_t count = 0;
        for (size_t i = offset - PageOffset; i < Response.NodesSize(); ++i) {
            const auto& name = Response.GetNames(i);
            const auto& node = Response.GetNodes(i);

            struct stat attr;
            ConvertAttr(node, attr);

            int retval = YFS_CALL(invoke, cb, name.c_str(), &attr, i);
            // TODO
            Y_UNUSED(retval);

            ++count;
        }

        return count;
    }

    void Reset()
    {
        Response.Clear();
        PageOffset = 0;
        CanFetchNextPage = true;
    }

    void Update(const NProto::TListNodesResponse& response, ui64 offset)
    {
        Response.CopyFrom(response);
        PageOffset = offset;
        CanFetchNextPage = bool(Response.GetCookie());
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

int TFileStoreService::OpenDir(ui64 ino, void** handle)
{
    STORAGE_TRACE("OpenDir #" << ino);

    *handle = new TDirectoryHandle(ino);
    return 0;
}

int TFileStoreService::ReadDir(void* h, yfs_readdir_cb* cb, ui64 offset)
{
    auto* handle = static_cast<TDirectoryHandle*>(h);
    STORAGE_TRACE("ReadDir #" << handle->GetNodeId()
        << " offset:" << offset);

    if (!offset) {
        // rewind
        handle->Reset();
    }

    size_t count = 0;
    for (;;) {
        // read current page
        int retval = handle->ReadDir(cb, offset + count);
        if (retval < 0) {
            return retval;
        }

        if (retval > 0) {
            count += retval;
            continue;
        }

        if (!handle->MoreDataAvailable()) {
            break;
        }

        // fetch next page
        auto callContext = PrepareCallContext();

        auto request = CreateRequest<NProto::TListNodesRequest>(handle->GetNodeId());
        request->SetCookie(handle->GetCookie());

        auto future = Session->ListNodes(
            std::move(callContext),
            std::move(request));

        const auto& response = future.GetValue(Config->GetRequestTimeout());
        retval = CheckResponse(response);
        if (retval < 0) {
            return retval;
        }

        // switch to next page
        handle->Update(response, offset + count);
    }

    return count;
}

int TFileStoreService::ReleaseDir(void* h)
{
    auto* handle = static_cast<TDirectoryHandle*>(h);
    STORAGE_TRACE("ReleaseDir #" << handle->GetNodeId());

    delete handle;
    return 0;
}

}   // namespace NCloud::NFileStore::NGateway
