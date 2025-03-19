#pragma once

#include <memory>

#include <util/generic/ptr.h>

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

struct TCallContext;
using TCallContextPtr = TIntrusivePtr<TCallContext>;

struct TServerCallContext;
using TServerCallContextPtr = TIntrusivePtr<TServerCallContext>;

template <typename T>
struct IResponseHandler;

template <typename T>
using IResponseHandlerPtr = std::shared_ptr<IResponseHandler<T>>;

struct IFileStore;
using IFileStorePtr = std::shared_ptr<IFileStore>;

struct IFileStoreService;
using IFileStoreServicePtr = std::shared_ptr<IFileStoreService>;

struct IFileStoreEndpoints;
using IFileStoreEndpointsPtr = std::shared_ptr<IFileStoreEndpoints>;

struct IEndpointManager;
using IEndpointManagerPtr = std::shared_ptr<IEndpointManager>;

}   // namespace NCloud::NFileStore
