#pragma once

#include <memory>

namespace NCloud::NBlockStore::NVhost {

////////////////////////////////////////////////////////////////////////////////

struct TVhostRequest;
using TVhostRequestPtr = std::shared_ptr<TVhostRequest>;

struct IVhostDevice;
using IVhostDevicePtr = std::shared_ptr<IVhostDevice>;

struct IVhostQueue;
using IVhostQueuePtr = std::shared_ptr<IVhostQueue>;

struct IVhostQueueFactory;
using IVhostQueueFactoryPtr = std::shared_ptr<IVhostQueueFactory>;

struct IServer;
using IServerPtr = std::shared_ptr<IServer>;

}   // namespace NCloud::NBlockStore::NVhost
