#pragma once

#include <memory>

namespace NCloud {

namespace NProbeParam {

////////////////////////////////////////////////////////////////////////////////

#if !defined(PLATFORM_PAGE_SIZE)
#   define PLATFORM_PAGE_SIZE 4096
#endif

#if !defined(PLATFORM_CACHE_LINE)
#   define PLATFORM_CACHE_LINE 64
#endif

#if !defined(Y_CACHE_ALIGNED)
#   define Y_CACHE_ALIGNED alignas(PLATFORM_CACHE_LINE)
#endif

////////////////////////////////////////////////////////////////////////////////

constexpr const char* MediaKind = "mediaKind";
constexpr const char* RequestExecutionTime =
    "requestExecutionTime";

}   // namespace NProbeParam

// namespace std {
//     template <typename T>
//     T&& move(const T&) = delete;
// }

////////////////////////////////////////////////////////////////////////////////

struct IScheduler;
using ISchedulerPtr = std::shared_ptr<IScheduler>;

struct IStartable;
using IStartablePtr = std::shared_ptr<IStartable>;

struct ITask;
using ITaskPtr = std::unique_ptr<ITask>;

struct ITaskQueue;
using ITaskQueuePtr = std::shared_ptr<ITaskQueue>;

struct ITimer;
using ITimerPtr = std::shared_ptr<ITimer>;

struct IFileIOService;
using IFileIOServicePtr = std::shared_ptr<IFileIOService>;

}   // namespace NCloud
