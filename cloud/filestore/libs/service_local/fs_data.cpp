#include "fs.h"

#include "lowlevel.h"

#include <util/string/builder.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

NProto::TCreateHandleResponse TLocalFileSystem::CreateHandle(
    const NProto::TCreateHandleRequest& request)
{
    STORAGE_TRACE("CreateHandle " << DumpMessage(request)
        << " flags: " << HandleFlagsToString(request.GetFlags())
        << " mode: " << request.GetMode());

    auto session = GetSession(request);
    auto node = session->LookupNode(request.GetNodeId());
    if (!node) {
        return TErrorResponse(ErrorInvalidParent(request.GetNodeId()));
    }

    const int flags = HandleFlagsToSystem(request.GetFlags());
    const int mode = request.GetMode() ? request.GetMode() : Config->GetDefaultPermissions();

    TFile handle;
    TFileStat stat;
    if (const auto& pathname = request.GetName()) {
        handle = node->OpenHandle(pathname, flags, mode);

        auto newnode = TIndexNode::Create(*node, pathname);
        stat = newnode->Stat();

        session->TryInsertNode(std::move(newnode));
    } else {
        handle = node->OpenHandle(flags);
        stat = node->Stat();
    }

    session->InsertHandle(handle);

    NProto::TCreateHandleResponse response;
    response.SetHandle(handle.GetHandle());
    ConvertStats(stat, *response.MutableNodeAttr());

    return response;
}

NProto::TDestroyHandleResponse TLocalFileSystem::DestroyHandle(
    const NProto::TDestroyHandleRequest& request)
{
    STORAGE_TRACE("DestroyHandle " << DumpMessage(request));

    auto session = GetSession(request);
    session->DeleteHandle(request.GetHandle());

    return {};
}

NProto::TReadDataResponse TLocalFileSystem::ReadData(
    const NProto::TReadDataRequest& request)
{
    STORAGE_TRACE("ReadData " << DumpMessage(request));

    auto session = GetSession(request);
    auto handle = session->LookupHandle(request.GetHandle());
    if (!handle.IsOpen()) {
        return TErrorResponse(ErrorInvalidHandle(request.GetHandle()));
    }

    auto buffer = TString::Uninitialized(request.GetLength());
    size_t bytesRead = handle.Pread(
        const_cast<char*>(buffer.data()),
        buffer.size(),
        request.GetOffset());
    buffer.resize(bytesRead);

    NProto::TReadDataResponse response;
    response.SetBuffer(std::move(buffer));

    return response;
}

NProto::TWriteDataResponse TLocalFileSystem::WriteData(
    const NProto::TWriteDataRequest& request)
{
    STORAGE_TRACE("WriteData " << DumpMessage(request));

    auto session = GetSession(request);
    auto handle = session->LookupHandle(request.GetHandle());
    if (!handle.IsOpen()) {
        return TErrorResponse(ErrorInvalidHandle(request.GetHandle()));
    }

    const auto& buffer = request.GetBuffer();
    handle.Pwrite(buffer.data(), buffer.size(), request.GetOffset());
    handle.Flush(); // TODO

    return {};
}

NProto::TAllocateDataResponse TLocalFileSystem::AllocateData(
    const NProto::TAllocateDataRequest& request)
{
    STORAGE_TRACE("AllocateData " << DumpMessage(request));

    auto session = GetSession(request);
    auto handle = session->LookupHandle(request.GetHandle());
    if (!handle.IsOpen()) {
        return TErrorResponse(ErrorInvalidHandle(request.GetHandle()));
    }

    handle.Resize(request.GetLength());
    handle.Flush(); // TODO

    return {};
}

}   // namespace NCloud::NFileStore
