#include "service.h"

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

int TFileStoreService::Lookup(ui64 parent, TString name, struct stat* attr)
{
    STORAGE_TRACE("Lookup #" << parent << " " << name.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TGetNodeAttrRequest>(parent);
    request->SetName(std::move(name));

    auto future = Session->GetNodeAttr(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    const auto& node = response.GetNode();
    ConvertAttr(node, *attr);

    return 0;
}

int TFileStoreService::MkDir(ui64 parent, TString name, mode_t mode, struct stat* attr)
{
    STORAGE_TRACE("MkDir #" << parent << " " << name.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));

    auto* dir = request->MutableDirectory();
    dir->SetMode(mode);

    auto future = Session->CreateNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    const auto& node = response.GetNode();
    ConvertAttr(node, *attr);

    return 0;
}

int TFileStoreService::RmDir(ui64 parent, TString name)
{
    STORAGE_TRACE("RmDir #" << parent << " " << name.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TUnlinkNodeRequest>(parent);
    request->SetName(std::move(name));
    request->SetUnlinkDirectory(true);

    auto future = Session->UnlinkNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

int TFileStoreService::MkNode(ui64 parent, TString name, mode_t mode, dev_t rdev, struct stat* attr)
{
    STORAGE_TRACE("MkNode #" << parent << " " << name.Quote() << " mode: " << mode);

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));

    auto* file = request->MutableFile();
    file->SetMode(mode);

    // TODO
    Y_UNUSED(rdev);

    auto future = Session->CreateNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    const auto& node = response.GetNode();
    ConvertAttr(node, *attr);

    return 0;
}

int TFileStoreService::Unlink(ui64 parent, TString name)
{
    STORAGE_TRACE("Unlink #" << parent << " " << name.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TUnlinkNodeRequest>(parent);
    request->SetName(std::move(name));
    request->SetUnlinkDirectory(false);

    auto future = Session->UnlinkNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

int TFileStoreService::Rename(ui64 parent, TString name, ui64 newparent, TString newname)
{
    STORAGE_TRACE("Rename #" << parent << " " << name.Quote() << " -> #"
        << newparent << " " << newname.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TRenameNodeRequest>(parent);
    request->SetName(std::move(name));
    request->SetNewName(std::move(newname));
    request->SetNewParentId(newparent);

    auto future = Session->RenameNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

int TFileStoreService::SymLink(ui64 parent, TString name, TString target, struct stat* attr)
{
    STORAGE_TRACE("SymLink #" << parent << " " <<  name.Quote() << " -> " << target.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));

    auto* link = request->MutableSymLink();
    link->SetTargetPath(std::move(target));

    auto future = Session->CreateNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    const auto& node = response.GetNode();
    ConvertAttr(node, *attr);

    return 0;
}

int TFileStoreService::Link(ui64 parent, TString name, ui64 ino, struct stat* attr)
{
    STORAGE_TRACE("Link #" << ino << " -> #" << parent << " " << name.Quote());

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));

    auto* link = request->MutableLink();
    link->SetTargetNode(ino);

    auto future = Session->CreateNode(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    const auto& node = response.GetNode();
    ConvertAttr(node, *attr);

    return 0;
}

}   // namespace NCloud::NFileStore::NGateway
