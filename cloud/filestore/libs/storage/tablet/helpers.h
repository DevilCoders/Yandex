#pragma once
#include "public.h"

#include <cloud/filestore/libs/service/error.h>
#include <cloud/filestore/libs/service/filestore.h>
#include <cloud/filestore/libs/storage/core/model.h>
#include <cloud/filestore/libs/storage/tablet/model/range_locks.h>
#include <cloud/filestore/libs/storage/tablet/protos/tablet.pb.h>
#include <cloud/filestore/libs/storage/tablet/session.h>
#include <cloud/filestore/private/api/protos/tablet.pb.h>
#include <cloud/filestore/public/api/protos/node.pb.h>

#include <cloud/storage/core/libs/tablet/model/commit.h>
#include <cloud/storage/core/libs/tablet/model/partial_blob_id.h>

#include <ydb/core/base/logoblob.h>

#include <util/system/align.h>
#include <util/system/yassert.h>

namespace NCloud::NFileStore::NStorage {

////////////////////////////////////////////////////////////////////////////////

inline ui64 SafeIncrement(ui64 value, size_t delta)
{
    Y_VERIFY(value <= Max<ui64>() - delta, "v: %lu, d: %lu", value, Max<ui64>() - delta);
    return value + delta;
}

inline ui64 SafeDecrement(ui64 value, size_t delta)
{
    Y_VERIFY(value >= delta, "v: %lu, d: %lu", value, delta);
    return value - delta;
}

////////////////////////////////////////////////////////////////////////////////

inline NKikimr::TLogoBlobID MakeBlobId(ui64 tabletId, const TPartialBlobId& blobId)
{
    return NKikimr::TLogoBlobID(
        tabletId,
        blobId.Generation(),
        blobId.Step(),
        blobId.Channel(),
        blobId.BlobSize(),
        blobId.Cookie());
}

////////////////////////////////////////////////////////////////////////////////

inline ui32 SizeToBlocks(ui64 size, ui32 block)
{
    return AlignUp<ui64>(size, block) / block;
}

inline i64 GetBlocksDifference(ui64 prevSize, ui64 curSize, ui32 block)
{
    return static_cast<i64>(SizeToBlocks(curSize, block)) -
        static_cast<i64>(SizeToBlocks(prevSize, block));
}

////////////////////////////////////////////////////////////////////////////////

enum ECopyAttrsMode {
    E_CM_CTIME = 1,     // CTime

    E_CM_MTIME = 2,     // MTime
    E_CM_CMTIME = 3,    // CTime + MTime

    E_CM_ATIME = 4,     // ATime
    E_CM_CATIME = 5,    // CTime + ATime

    E_CM_REF = 8,       // Links +1
    E_CM_UNREF = 16,    // Links -1
};

NProto::TNode CreateRegularAttrs(ui32 mode, ui32 uid, ui32 gid);
NProto::TNode CreateDirectoryAttrs(ui32 mode, ui32 uid, ui32 gid);
NProto::TNode CreateLinkAttrs(const TString& link, ui32 uid, ui32 gid);
NProto::TNode CreateSocketAttrs(ui32 mode, ui32 uid, ui32 gid);

NProto::TNode CopyAttrs(const NProto::TNode& src, ui32 mode = E_CM_CTIME);

void ConvertNodeFromAttrs(NProto::TNodeAttr& dst, ui64 id, const NProto::TNode& src);

////////////////////////////////////////////////////////////////////////////////

inline ELockMode GetLockMode(NProto::ELockType lock)
{
    return (lock == NProto::E_EXCLUSIVE) ? ELockMode::Exclusive : ELockMode::Shared;
}

////////////////////////////////////////////////////////////////////////////////

inline NProto::TError ValidateNodeName(const TString& name)
{
    static constexpr TStringBuf Dot = ".";
    static constexpr TStringBuf DotDot = "..";

    if (name.empty() || name == Dot || name == DotDot ||
        name.find('\0') != TString::npos ||
        name.find('/') != TString::npos)
    {
        return ErrorInvalidArgument();
    }

    if (name.size() > MaxName) {
        return ErrorNameTooLong(name);
    }

    return {};
}

////////////////////////////////////////////////////////////////////////////////

NProto::TError ValidateXAttrName(const TString& name);

NProto::TError ValidateXAttrValue(const TString& name, const TString& value);

////////////////////////////////////////////////////////////////////////////////

NProto::TFileSystem CopyFrom(const NKikimrFileStore::TConfig& srcProto);

NProtoPrivate::TFileSystemConfig CopyFrom(const NProto::TFileSystem& srcProto);

}   // namespace NCloud::NFileStore::NStorage
