#include "fs_impl.h"

#include <cloud/storage/core/libs/common/helpers.h>

#include <util/string/builder.h>

#include <sys/xattr.h>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////
// node attributes

void TFileSystem::SetAttr(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    struct stat* attr,
    int to_set,
    fuse_file_info* fi)
{
    ui64 handle = fi ? fi->fh : 0;
    STORAGE_DEBUG("SetAttr #" << ino << " @" << handle
        << " mask:" << to_set);

    auto request = StartRequest<NProto::TSetNodeAttrRequest>(ino);
    request->SetHandle(handle);

    auto* update = request->MutableUpdate();
    int flags = 0;
    if (to_set & FUSE_SET_ATTR_MODE) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_MODE);
        update->SetMode(attr->st_mode);
    }
    if (to_set & FUSE_SET_ATTR_UID) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_UID);
        update->SetUid(attr->st_uid);
    }
    if (to_set & FUSE_SET_ATTR_GID) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_GID);
        update->SetGid(attr->st_gid);
    }
    if (to_set & FUSE_SET_ATTR_SIZE) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_SIZE);
        update->SetSize(attr->st_size);
    }
    if (to_set & FUSE_SET_ATTR_ATIME) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_ATIME);
        update->SetATime(ConvertTimeSpec(attr->st_atim).MicroSeconds());
    } else if (to_set & FUSE_SET_ATTR_ATIME_NOW) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_ATIME);
        update->SetATime(MicroSeconds());
    }
    if (to_set & FUSE_SET_ATTR_MTIME) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_MTIME);
        update->SetMTime(ConvertTimeSpec(attr->st_mtim).MicroSeconds());
    } else if (to_set & FUSE_SET_ATTR_MTIME_NOW) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_MTIME);
        update->SetMTime(MicroSeconds());
    }
#if defined(FUSE_VIRTIO)
    if (to_set & FUSE_SET_ATTR_CTIME) {
        flags |= ProtoFlag(NProto::TSetNodeAttrRequest::F_SET_ATTR_CTIME);
        update->SetCTime(ConvertTimeSpec(attr->st_ctim).MicroSeconds());
    }
#endif

    request->SetFlags(flags);

    Session->SetNodeAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyAttr(*callContext, req, response.GetNode());
            }
        });
}

void TFileSystem::GetAttr(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    fuse_file_info* fi)
{
    ui64 handle = fi ? fi->fh : 0;
    STORAGE_DEBUG("GetAttr #" << ino << " @" << handle);

    auto request = StartRequest<NProto::TGetNodeAttrRequest>(ino);
    request->SetHandle(handle);

    // TODO
    // request->SetFlags(flags);

    Session->GetNodeAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                ReplyAttr(*callContext, req, response.GetNode());
            }
        });
}

////////////////////////////////////////////////////////////////////////////////
// extended node attributes

void TFileSystem::SetXAttr(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    TString name,
    TString value,
    int xflags)
{
    STORAGE_DEBUG("SetXAttr #" << ino << " " << name.Quote()
        << " value:" << value.Quote()
        << " flags:" << xflags);

    auto request = StartRequest<NProto::TSetNodeXAttrRequest>(ino);
    request->SetName(name);
    request->SetValue(value);
    ui32 flags = 0;
    if (xflags & XATTR_CREATE) {
        SetProtoFlag(flags, NProto::TSetNodeXAttrRequest::F_CREATE);
    }
    if (xflags & XATTR_REPLACE) {
        SetProtoFlag(flags, NProto::TSetNodeXAttrRequest::F_REPLACE);
    }

    request->SetFlags(flags);

    Session->SetNodeXAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            UpdateXAttrCache(
                ino,
                name,
                value,
                response.GetVersion(),
                response.GetError());
            if (CheckResponse(*callContext, req, response)) {
                ReplyError(
                    *callContext,
                    req,
                    0);
            }
        });
}

void TFileSystem::GetXAttr(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    TString name,
    size_t size)
{
    STORAGE_DEBUG("GetXAttr #" << ino << " " << name.Quote());

    with_lock (XAttrLock) {
        if (auto xattr = XAttrCache.Get(ino, name)) {
            if (xattr->Value) {
                ReplyXAttrInt(*callContext, req, *xattr->Value, size);
            } else {
                ReplyError(*callContext, req, ENODATA);
            }
            return;
        }
    }

    auto request = StartRequest<NProto::TGetNodeXAttrRequest>(ino);
    request->SetName(name);

    Session->GetNodeXAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            UpdateXAttrCache(
                ino,
                name,
                response.GetValue(),
                response.GetVersion(),
                response.GetError());
            if (CheckResponse(*callContext, req, response)) {
                ReplyXAttrInt(
                    *callContext,
                    req,
                    response.GetValue(),
                    size);
            }
        });
}

void TFileSystem::ListXAttr(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size)
{
    STORAGE_DEBUG("ListXAttr #" << ino);

    auto request = StartRequest<NProto::TListNodeXAttrRequest>(ino);

    Session->ListNodeXAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            const auto& response = future.GetValue();
            if (CheckResponse(*callContext, req, response)) {
                TStringBuilder value;
                for (const auto& name: response.GetNames()) {
                    value << name << '\0';
                }

                ReplyXAttrInt(
                    *callContext,
                    req,
                    value,
                    size);
            }
        });
}

void TFileSystem::RemoveXAttr(
    TCallContextPtr callContext,
    fuse_req_t req,
    fuse_ino_t ino,
    TString name)
{
    STORAGE_DEBUG("RemoveXAttr #" << ino << " " << name.Quote());

    auto request = StartRequest<NProto::TRemoveNodeXAttrRequest>(ino);
    request->SetName(name);

    Session->RemoveNodeXAttr(callContext, std::move(request))
        .Subscribe([=] (const auto& future) {
            with_lock (XAttrLock) {
                XAttrCache.Forget(ino, name);
            }

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
