#pragma once

#include "stats_output.h"
#include "stats_writer.h"

#include <util/thread/pool.h>

/// queue processed by dynamic size thread pool
class TPreemptiveMtpQueue: public IThreadPool, public TThreadFactoryHolder {
public:
    TPreemptiveMtpQueue(IThreadFactory* pool, size_t maxThreadCount); // recommended for usage with TDynamicThreadPool
    ~TPreemptiveMtpQueue() override;

    [[nodiscard]] bool Add(IObjectInQueue* obj) override;
    /** @param queueSizeLimit means "unlimited" when = 0 */
    void Start(size_t minThreadCount, size_t queueSizeLimit = 0) override;
    void Stop() noexcept override;
    size_t Size() const noexcept override;
    void PrintStatistics(NAntiRobot::TStatsWriter& out);
    void PrintStatistics(TStatsOutput& out);

private:
    size_t MaxThreadCount;
    class TImpl;
    THolder<TImpl> Impl_;
};

