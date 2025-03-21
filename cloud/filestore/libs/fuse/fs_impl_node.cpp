#include "fs_impl.h"

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////
// nodes

void TFileSystem::Lookup(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name)
{
    STORAGE_DEBUG("Lookup #" << parent << " " << name.Quote());

    auto request = StartRequest<NProto::TGetNodeAttrRequest>(parent);
    request->SetName(std::move(name));

    Session->GetNodeAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (!HasError(response)) {
                ReplyEntry(*callContext, req, response.GetNode());
            } else {
                fuse_entry_param entry = {};
                entry.entry_timeout = Config->GetEntryTimeout().Seconds();
                ReplyEntry(
                    *callContext,
                    req,
                    &entry);
            }
        });
}

void TFileSystem::Forget(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    unsigned long nlookup)
{
    with_lock (CacheLock) {
        Cache.ForgetNode(ino, nlookup);
    }

    ReplyNone(
        *callContext,
        req);
}

void TFileSystem::ForgetMulti(
    TCallContextPtr callContext,
    fuse_req_t req,
    size_t count,
    fuse_forget_data* forgets)
{
    with_lock (CacheLock) {
        for (size_t i = 0; i < count; ++i) {
            Cache.ForgetNode(forgets[i].ino, forgets[i].nlookup);
        }
    }

    ReplyNone(
        *callContext,
        req);
}

void TFileSystem::MkDir(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name,
    mode_t mode)
{
    STORAGE_DEBUG("MkDir #" << parent << " " << name.Quote());

    auto request = StartRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));
    SetUserNGroup(*request, fuse_req_ctx(req));

    auto* dir = request->MutableDirectory();
    dir->SetMode(mode & ~(S_IFMT));

    Session->CreateNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyEntry(*callContext, req, response.GetNode());
            }
        });
}

void TFileSystem::RmDir(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name)
{
    STORAGE_DEBUG("RmDir #" << parent << " " << name.Quote());

    auto request = StartRequest<NProto::TUnlinkNodeRequest>(parent);
    request->SetName(std::move(name));
    request->SetUnlinkDirectory(true);

    Session->UnlinkNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

void TFileSystem::MkNode(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name,
    mode_t mode,
    dev_t rdev)
{
    // TODO
    Y_UNUSED(rdev);

    STORAGE_DEBUG("MkNode #" << parent << " " << name.Quote() << " mode: " << mode);
    auto request = StartRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));
    SetUserNGroup(*request, fuse_req_ctx(req));

    if (S_ISREG(mode)) {
        // just an empty file
        auto* file = request->MutableFile();
        file->SetMode(mode & ~(S_IFMT));
    } else if (S_ISSOCK(mode)) {
        // null file type for unix sockets
        auto* socket = request->MutableSocket();
        socket->SetMode(mode & ~(S_IFMT));
    } else {
        ReplyError(
            *callContext,
            req,
            ENOTSUP);
        return;
    }

    Session->CreateNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyEntry(*callContext, req, response.GetNode());
            }
        });
}

void TFileSystem::Unlink(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name)
{
    STORAGE_DEBUG("Unlink #" << parent << " " << name.Quote());

    auto request = StartRequest<NProto::TUnlinkNodeRequest>(parent);
    request->SetName(std::move(name));
    request->SetUnlinkDirectory(false);

    Session->UnlinkNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

void TFileSystem::Rename(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t parent,
    TString name,
    fuse_ino_t newparent,
    TString newname,
    int flags)
{
    STORAGE_DEBUG("Rename #" << parent << " " << name.Quote() << " -> #"
        << newparent << " " << newname.Quote());

    auto request = StartRequest<NProto::TRenameNodeRequest>(parent);
    request->SetName(std::move(name));
    request->SetNewName(std::move(newname));
    request->SetNewParentId(newparent);

    // TODO
    Y_UNUSED(flags);

    Session->RenameNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                // TODO: update tree

                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

void TFileSystem::SymLink(
    TCallContextPtr callContext,
    fuse_req_t req,
    TString target,
    fuse_ino_t parent,
    TString name)
{
    STORAGE_DEBUG("SymLink #" << parent << " " <<  name.Quote() << " -> " << target.Quote());

    auto request = StartRequest<NProto::TCreateNodeRequest>(parent);
    request->SetName(std::move(name));
    SetUserNGroup(*request, fuse_req_ctx(req));

    auto* link = request->MutableSymLink();
    link->SetTargetPath(std::move(target));

    Session->CreateNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyEntry(*callContext, req, response.GetNode());
            }
        });
}

void TFileSystem::Link(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_ino_t newparent,
    TString newname)
{
    STORAGE_DEBUG("Link #" << ino << " -> #" << newparent << " " << newname.Quote());

    auto request = StartRequest<NProto::TCreateNodeRequest>(newparent);
    request->SetName(std::move(newname));

    auto* link = request->MutableLink();
    link->SetTargetNode(ino);

    Session->CreateNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyEntry(*callContext, req, response.GetNode());
            }
        });
}

void TFileSystem::ReadLink(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino)
{
    STORAGE_DEBUG("ReadLink #" << ino);

    auto request = StartRequest<NProto::TReadLinkRequest>(ino);

    Session->ReadLink(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyReadLink(
                    *callContext,
                    req,
                    response.GetSymLink().data());
            }
        });
}

void TFileSystem::Access(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    int mask)
{
    STORAGE_DEBUG("Access #" << ino << " mask " << mask);

    auto request = StartRequest<NProto::TAccessNodeRequest>(ino);
    request->SetMask(mask);

    Session->AccessNode(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

}   // namespace NCloud::NFileStore::NFuse
