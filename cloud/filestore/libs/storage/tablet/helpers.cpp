#include "helpers.h"

#include <ydb/core/protos/filestore_config.pb.h>

namespace NCloud::NFileStore::NStorage {

using namespace NCloud::NFileStore::NProto;

namespace {

////////////////////////////////////////////////////////////////////////////////

const THashSet<TString> AllowedXAttrNamespace = {
    "security",
    "system",
    "trusted",
    "user",
};

const THashSet<TString> UnsupportedXAttrs = {
    "system.posix_acl_access",  // NBS-3322
    "system.posix_acl_default", // NBS-3322
};

////////////////////////////////////////////////////////////////////////////////

NProto::TNode CreateAttrs(NProto::ENodeType type, int mode, ui64 size, ui64 uid, ui64 gid)
{
    ui64 now = MicroSeconds();

    NProto::TNode node;
    node.SetType(type);
    node.SetMode(mode);
    node.SetATime(now);
    node.SetMTime(now);
    node.SetCTime(now);
    node.SetLinks(1);
    node.SetSize(size);
    node.SetUid(uid);
    node.SetGid(gid);

    return node;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

NProto::TNode CreateRegularAttrs(ui32 mode, ui32 uid, ui32 gid)
{
    return CreateAttrs(NProto::E_REGULAR_NODE, mode, 0, uid, gid);
}

NProto::TNode CreateDirectoryAttrs(ui32 mode, ui32 uid, ui32 gid)
{
    return CreateAttrs(NProto::E_DIRECTORY_NODE, mode, 0, uid, gid);
}

NProto::TNode CreateLinkAttrs(const TString& link, ui32 uid, ui32 gid)
{
    auto attrs = CreateAttrs(NProto::E_LINK_NODE, 0, link.size(), uid, gid);
    attrs.SetSymLink(link);
    return attrs;
}

NProto::TNode CreateSocketAttrs(ui32 mode, ui32 uid, ui32 gid)
{
    return CreateAttrs(NProto::E_SOCK_NODE, mode, 0, uid, gid);
}

NProto::TNode CopyAttrs(const NProto::TNode& src, ui32 mode)
{
    ui64 now = MicroSeconds();

    NProto::TNode node = src;
    if (mode & E_CM_CTIME) {
        node.SetCTime(now);
    }
    if (mode & E_CM_MTIME) {
        node.SetMTime(now);
    }
    if (mode & E_CM_ATIME) {
        node.SetATime(now);
    }
    if (mode & E_CM_REF) {
        node.SetLinks(SafeIncrement(node.GetLinks(), 1));
    }
    if (mode & E_CM_UNREF) {
        node.SetLinks(SafeDecrement(node.GetLinks(), 1));
    }

    return node;
}

void ConvertNodeFromAttrs(NProto::TNodeAttr& dst, ui64 id, const NProto::TNode& src)
{
    dst.SetId(id);
    dst.SetType(src.GetType());
    dst.SetMode(src.GetMode());
    dst.SetUid(src.GetUid());
    dst.SetGid(src.GetGid());
    dst.SetATime(src.GetATime());
    dst.SetMTime(src.GetMTime());
    dst.SetCTime(src.GetCTime());
    dst.SetSize(src.GetSize());
    dst.SetLinks(src.GetLinks());
}

NProto::TError ValidateXAttrName(const TString& name)
{
    if (name.size() > MaxXAttrName) {
        return ErrorAttributeNameTooLong(name);
    }

    TStringBuf ns;
    TStringBuf attr;
    if (!TStringBuf(name).TrySplit(".", ns, attr)) {
        return ErrorInvalidAttribute(name);
    }

    if (!AllowedXAttrNamespace.contains(ns)) {
        return ErrorInvalidAttribute(name);
    }

    if (UnsupportedXAttrs.contains(name)) {
        return ErrorInvalidAttribute(name);
    }

    return {};
}

NProto::TError ValidateXAttrValue(const TString& name, const TString& value)
{
    if (value.size() > MaxXAttrValue) {
        return ErrorAttributeValueTooBig(name);
    }

    return {};
}

NProto::TFileSystem CopyFrom(const NKikimrFileStore::TConfig& srcProto)
{
    NProto::TFileSystem dstProto;

    for (size_t i = 0; i < srcProto.ExplicitChannelProfilesSize(); ++i) {
        const auto& src = srcProto.GetExplicitChannelProfiles(i);
        auto& dst = *dstProto.MutableExplicitChannelProfiles()->Add();

        dst.SetDataKind(src.GetDataKind());
        dst.SetPoolKind(src.GetPoolKind());
    }

    const auto mediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
        srcProto.GetStorageMediaKind());

    dstProto.SetVersion(srcProto.GetVersion());
    dstProto.SetFileSystemId(srcProto.GetFileSystemId());
    dstProto.SetProjectId(srcProto.GetProjectId());
    dstProto.SetFolderId(srcProto.GetFolderId());
    dstProto.SetCloudId(srcProto.GetCloudId());
    dstProto.SetBlockSize(srcProto.GetBlockSize());
    dstProto.SetBlocksCount(srcProto.GetBlocksCount());
    dstProto.SetNodesCount(srcProto.GetNodesCount());
    dstProto.SetRangeIdHasherType(srcProto.GetRangeIdHasherType());
    dstProto.SetStorageMediaKind(mediaKind);

    dstProto.MutablePerformanceProfile()->SetMaxReadIops(
        srcProto.GetPerformanceProfileMaxReadIops());
    dstProto.MutablePerformanceProfile()->SetMaxWriteIops(
        srcProto.GetPerformanceProfileMaxWriteIops());
    dstProto.MutablePerformanceProfile()->SetMaxReadBandwidth(
        srcProto.GetPerformanceProfileMaxReadBandwidth());
    dstProto.MutablePerformanceProfile()->SetMaxWriteBandwidth(
        srcProto.GetPerformanceProfileMaxWriteBandwidth());

    return dstProto;
}

NProtoPrivate::TFileSystemConfig CopyFrom(const NProto::TFileSystem& srcProto)
{
    NProtoPrivate::TFileSystemConfig dstProto;

    for (size_t i = 0; i < srcProto.ExplicitChannelProfilesSize(); ++i) {
        const auto& src = srcProto.GetExplicitChannelProfiles(i);
        auto& dst = *dstProto.MutableExplicitChannelProfiles()->Add();

        dst.SetDataKind(src.GetDataKind());
        dst.SetPoolKind(src.GetPoolKind());
    }

    const auto mediaKind = static_cast<NCloud::NProto::EStorageMediaKind>(
        srcProto.GetStorageMediaKind());

    dstProto.SetVersion(srcProto.GetVersion());
    dstProto.SetFileSystemId(srcProto.GetFileSystemId());
    dstProto.SetProjectId(srcProto.GetProjectId());
    dstProto.SetFolderId(srcProto.GetFolderId());
    dstProto.SetCloudId(srcProto.GetCloudId());
    dstProto.SetBlockSize(srcProto.GetBlockSize());
    dstProto.SetBlocksCount(srcProto.GetBlocksCount());
    dstProto.SetNodesCount(srcProto.GetNodesCount());
    dstProto.SetRangeIdHasherType(srcProto.GetRangeIdHasherType());
    dstProto.SetStorageMediaKind(mediaKind);

    dstProto.MutablePerformanceProfile()->CopyFrom(
        srcProto.GetPerformanceProfile());

    return dstProto;
}

}   // namespace NCloud::NFileStore::NStorage
