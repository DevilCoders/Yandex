#include "service.h"

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

int TFileStoreService::GetAttr(ui64 ino, struct stat* attr)
{
    STORAGE_TRACE("GetAttr #" << ino);

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TGetNodeAttrRequest>(ino);

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

int TFileStoreService::SetAttr(ui64 ino, const struct stat* attr, ui32 flags)
{
    STORAGE_TRACE("SetAttr #" << ino);

    auto callContext = PrepareCallContext();

    auto request = CreateRequest<NProto::TSetNodeAttrRequest>(ino);
    request->SetFlags(flags);

    auto* update = request->MutableUpdate();
    if (flags & F_SET_ATTR_MODE) {
        update->SetMode(attr->st_mode);
    }
    if (flags & F_SET_ATTR_UID) {
        update->SetUid(attr->st_uid);
    }
    if (flags & F_SET_ATTR_GID) {
        update->SetGid(attr->st_gid);
    }
    if (flags & F_SET_ATTR_SIZE) {
        update->SetSize(attr->st_size);
    }
    if (flags & F_SET_ATTR_ATIME) {
        update->SetATime(TimeSpecToMicroSeconds(attr->st_atim));
    }
    if (flags & F_SET_ATTR_MTIME) {
        update->SetMTime(TimeSpecToMicroSeconds(attr->st_mtim));
    }

    auto future = Session->SetNodeAttr(
        std::move(callContext),
        std::move(request));

    const auto& response = future.GetValue(Config->GetRequestTimeout());
    int retval = CheckResponse(response);
    if (retval < 0) {
        return retval;
    }

    return 0;
}

}   // namespace NCloud::NFileStore::NGateway
