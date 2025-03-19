#pragma once

#include <memory>

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

class TFileStoreServiceConfig;
using TFileStoreServiceConfigPtr = std::shared_ptr<TFileStoreServiceConfig>;

class TFileStoreServiceFactory;
using TFileStoreServiceFactoryPtr = std::shared_ptr<TFileStoreServiceFactory>;

}   // namespace NCloud::NFileStore::NGateway
