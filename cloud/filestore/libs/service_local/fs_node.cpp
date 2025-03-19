#include "fs.h"

#include <util/generic/guid.h>

namespace NCloud::NFileStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

TIndexNodePtr TryCreateChildNode(const TIndexNode& parent, const TString& name)
{
    try {
        return TIndexNode::Create(parent, name);
    } catch (...) {
        // TODO: ignore only ENOENT
    }

    return nullptr;
}

}

////////////////////////////////////////////////////////////////////////////////

NProto::TResolvePathResponse TLocalFileSystem::ResolvePath(
    const NProto::TResolvePathRequest& request)
{
    STORAGE_TRACE("ResolvePath " << DumpMessage(request));

    // TODO

    return {};
}

NProto::TCreateNodeResponse TLocalFileSystem::CreateNode(
    const NProto::TCreateNodeRequest& request)
{
    STORAGE_TRACE("CreateNode " << DumpMessage(request));

    auto session = GetSession(request);
    auto parent = session->LookupNode(request.GetNodeId());
    if (!parent) {
        return TErrorResponse(ErrorInvalidParent(request.GetNodeId()));
    }

    TIndexNodePtr target;
    if (request.HasDirectory()) {
        int mode = request.GetDirectory().GetMode();
        if (!mode) {
            mode = Config->GetDefaultPermissions();
        }

        target = parent->CreateDirectory(request.GetName(), mode);
    } else if (request.HasFile()) {
        int mode = request.GetFile().GetMode();
        if (!mode) {
            mode = Config->GetDefaultPermissions();
        }

        target = parent->CreateFile(request.GetName(), mode);
    } else if (request.HasSymLink()) {
        target = parent->CreateSymlink(request.GetSymLink().GetTargetPath(), request.GetName());
    } else if (request.HasLink()) {
        auto node = session->LookupNode(request.GetLink().GetTargetNode());
        if (!node) {
            return TErrorResponse(ErrorInvalidTarget(request.GetNodeId()));
        }

        target = node->CreateLink(*parent, request.GetName());
    } else if (request.HasSocket()) {
        int mode = request.GetSocket().GetMode();
        if (!mode) {
            mode = Config->GetDefaultPermissions();
        }

        target = parent->CreateSocket(request.GetName(), mode);
    } else {
        return TErrorResponse(ErrorInvalidArgument());
    }

    auto stat = target->Stat();
    session->TryInsertNode(std::move(target));

    NProto::TCreateNodeResponse response;
    ConvertStats(stat, *response.MutableNode());

    return response;
}

NProto::TUnlinkNodeResponse TLocalFileSystem::UnlinkNode(
    const NProto::TUnlinkNodeRequest& request)
{
    STORAGE_TRACE("UnlinkNode " << DumpMessage(request));

    auto session = GetSession(request);
    auto parent = session->LookupNode(request.GetNodeId());
    if (!parent) {
        return TErrorResponse(ErrorInvalidParent(request.GetNodeId()));
    }

    auto stat = parent->Stat(request.GetName());
    parent->Unlink(request.GetName(), request.GetUnlinkDirectory());

    // FIXME
    if (stat.NLinks == 1) {
        session->ForgetNode(stat.INode);
    }

    return {};
}

NProto::TRenameNodeResponse TLocalFileSystem::RenameNode(
    const NProto::TRenameNodeRequest& request)
{
    STORAGE_TRACE("RenameNode " << DumpMessage(request));

    auto session = GetSession(request);
    auto parent = session->LookupNode(request.GetNodeId());
    if (!parent) {
        return TErrorResponse(ErrorInvalidParent(request.GetNodeId()));
    }

    auto newparent = session->LookupNode(request.GetNewParentId());
    if (!newparent) {
        return TErrorResponse(ErrorInvalidParent(request.GetNodeId()));
    }

    parent->Rename(request.GetName(), newparent, request.GetNewName());
    return {};
}

NProto::TAccessNodeResponse TLocalFileSystem::AccessNode(
    const NProto::TAccessNodeRequest& request)
{
    STORAGE_TRACE("AccessNode " << DumpMessage(request));

    auto session = GetSession(request);
    auto node = session->LookupNode(request.GetNodeId());
    if (!node) {
        return TErrorResponse(ErrorInvalidTarget(request.GetNodeId()));
    }

    node->Access(request.GetMask());

    return {};
}

NProto::TListNodesResponse TLocalFileSystem::ListNodes(
    const NProto::TListNodesRequest& request)
{
    STORAGE_TRACE("ListNodes " << DumpMessage(request));

    auto session = GetSession(request);
    auto parent = session->LookupNode(request.GetNodeId());
    if (!parent) {
        return TErrorResponse(ErrorInvalidParent(request.GetNodeId()));
    }

    auto entries = parent->List();

    NProto::TListNodesResponse response;
    response.MutableNames()->Reserve(entries.size());
    response.MutableNodes()->Reserve(entries.size());

    for (auto& entry: entries) {
        if (!session->LookupNode(entry.second.INode)) {
            auto node = TryCreateChildNode(*parent, entry.first);
            if (node && node->GetNodeId() == entry.second.INode) {
                session->TryInsertNode(std::move(node));
            }
        }

        response.MutableNames()->Add()->swap(entry.first);
        ConvertStats(entry.second, *response.MutableNodes()->Add());
    }

    return response;
}

NProto::TReadLinkResponse TLocalFileSystem::ReadLink(
    const NProto::TReadLinkRequest& request)
{
    STORAGE_TRACE("ReadLink " << DumpMessage(request));

    auto session = GetSession(request);
    auto node = session->LookupNode(request.GetNodeId());
    if (!node) {
        return TErrorResponse(ErrorInvalidTarget(request.GetNodeId()));
    }

    auto link = node->ReadLink();

    NProto::TReadLinkResponse response;
    response.SetSymLink(link);

    return response;
}

}   // namespace NCloud::NFileStore
