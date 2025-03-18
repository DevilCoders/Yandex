#pragma once

#include <library/cpp/threading/blocking_queue/blocking_queue.h>

#include <util/thread/factory.h>
#include <util/thread/pool.h>

namespace NAapi {

template <class TWriter>
struct TNoopStopWriter {
    inline static void Stop(TWriter*) {
    }
};

template <class TWriter>
struct TStopGrpcClientWriter {
    inline static void Stop(TWriter* writer) {
        writer->WritesDone();
    }
};

template <class TWriter, class TData, class TStopWriter = TNoopStopWriter<TWriter> >
class TAsyncWriter {
public:
    inline explicit TAsyncWriter(TWriter* writer, size_t bufferSize)
        : Writer(writer)
        , Queue(new TThreadPool(TThreadPool::TParams().SetBlocking(true).SetCatching(true)))
    {
        Queue->Start(1, bufferSize);
    }

    inline ~TAsyncWriter() {
        Join();
    }

    inline void Write(const TData& data) {
        Queue->SafeAddFunc([this, data] () {Writer->Write(data);});
    }

    inline void Write(TData&& data) {
        Queue->SafeAddFunc([this, data = std::move(data)] () {Writer->Write(data);});
    }

    inline void Stop() {
        Queue->SafeAddFunc([this] () {TStopWriter::Stop(Writer);});
    }

    inline void Join() {
        Queue->Stop();
    }

private:
    TWriter* Writer;
    THolder<IThreadPool> Queue;
};

}  // namespace NAapi
