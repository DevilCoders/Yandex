#pragma once

#include "public.h"
#include "request.h"

#include <cloud/filestore/public/api/protos/checkpoint.pb.h>
#include <cloud/filestore/public/api/protos/cluster.pb.h>
#include <cloud/filestore/public/api/protos/const.pb.h>
#include <cloud/filestore/public/api/protos/data.pb.h>
#include <cloud/filestore/public/api/protos/fs.pb.h>
#include <cloud/filestore/public/api/protos/locks.pb.h>
#include <cloud/filestore/public/api/protos/node.pb.h>
#include <cloud/filestore/public/api/protos/ping.pb.h>
#include <cloud/filestore/public/api/protos/session.pb.h>

#include <cloud/storage/core/libs/common/error.h>
#include <cloud/storage/core/libs/common/startable.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/string.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

constexpr ui64 InvalidNodeId = NProto::E_INVALID_NODE_ID;
constexpr ui64 RootNodeId = NProto::E_ROOT_NODE_ID;

constexpr ui32 MaxLink = NProto::E_FS_LIMITS_LINK;
constexpr ui32 MaxName = NProto::E_FS_LIMITS_NAME;
constexpr ui32 MaxPath = NProto::E_FS_LIMITS_PATH;
constexpr ui32 MaxSymlink = NProto::E_FS_LIMITS_SYMLINK;
constexpr ui64 MaxFileBlocks = static_cast<ui32>(NProto::E_FS_LIMITS_FILEBLOCKS);
constexpr ui64 MaxNodes = static_cast<ui32>(NProto::E_FS_LIMITS_INODES);
constexpr ui64 MaxXAttrName = NProto::E_FS_LIMITS_XATTR_NAME;
constexpr ui64 MaxXAttrValue = NProto::E_FS_LIMITS_XATTR_VALUE;

////////////////////////////////////////////////////////////////////////////////

constexpr int ProtoFlag(int value)
{
    return value ? 1 << (value - 1) : value;
}

constexpr bool HasFlag(int flags, int value)
{
    return flags & ProtoFlag(value);
}

////////////////////////////////////////////////////////////////////////////////

std::pair<int, int> SystemFlagsToHandle(int flags);

int HandleFlagsToSystem(int flags);
TString HandleFlagsToString(int flags);

////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IResponseHandler
{
    virtual ~IResponseHandler() = default;

    virtual void HandleResponse(const T& response) = 0;
    virtual void HandleCompletion(const NProto::TError& error) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IFileStore
{
    virtual ~IFileStore() = default;

#define FILESTORE_DECLARE_METHOD(name, ...)                                    \
    virtual NThreading::TFuture<NProto::T##name##Response> name(               \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) = 0;                \
// FILESTORE_DECLARE_METHOD

    FILESTORE_DATA_SERVICE(FILESTORE_DECLARE_METHOD)

#undef FILESTORE_DECLARE_METHOD
};

////////////////////////////////////////////////////////////////////////////////

struct IFileStoreService
    : public IFileStore
    , public IStartable
{
    virtual ~IFileStoreService() = default;

#define FILESTORE_DECLARE_METHOD(name, ...)                                    \
    virtual NThreading::TFuture<NProto::T##name##Response> name(               \
        TCallContextPtr callContext,                                           \
        std::shared_ptr<NProto::T##name##Request> request) = 0;                \
// FILESTORE_DECLARE_METHOD

    FILESTORE_CONTROL_SERVICE(FILESTORE_DECLARE_METHOD)

#undef FILESTORE_DECLARE_METHOD

    virtual void GetSessionEventsStream(
        TCallContextPtr callContext,
        std::shared_ptr<NProto::TGetSessionEventsRequest> request,
        IResponseHandlerPtr<NProto::TGetSessionEventsResponse> responseHandler) = 0;
};

////////////////////////////////////////////////////////////////////////////////

struct IFileStoreEndpoints
    : public IStartable
{
    virtual ~IFileStoreEndpoints() = default;

    virtual IFileStoreServicePtr GetEndpoint(const TString& name) = 0;
};

}   // namespace NCloud::NFileStore
