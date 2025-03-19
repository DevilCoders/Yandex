#pragma once

#include <memory>

namespace NCloud::NFileStore::NFuse {

////////////////////////////////////////////////////////////////////////////////

struct TFuseConfig;
using TFuseConfigPtr = std::shared_ptr<TFuseConfig>;

struct TFileSystemConfig;
using TFileSystemConfigPtr = std::shared_ptr<TFileSystemConfig>;

struct IFileSystem;
using IFileSystemPtr = std::shared_ptr<IFileSystem>;

struct IFileSystemFactory;
using IFileSystemFactoryPtr = std::shared_ptr<IFileSystemFactory>;

struct IFileSystemDriver;
using IFileSystemDriverPtr = std::shared_ptr<IFileSystemDriver>;

struct ICompletionQueue;
using ICompletionQueuePtr = std::shared_ptr<ICompletionQueue>;

}   // namespace NCloud::NFileStore::NFuse
