#pragma once

#include <library/cpp/threading/blocking_queue/blocking_queue.h>

#include <util/thread/factory.h>

namespace NAapi {

template <class TReader, class TData>
class TAsyncReader: public IThreadFactory::IThreadAble {
public:
    inline explicit TAsyncReader(TReader* reader, size_t bufferSize)
        : Reader(reader)
        , Queue(bufferSize)
    {
        Thread = SystemThreadFactory()->Run(this);
    }

    inline ~TAsyncReader() {
        Thread->Join();
    }

    inline bool Read(TData& data) {
        TMaybe<TData> front = *Queue.Pop();
        if (!front) {
            return false;
        }
        data = std::move(*front);
        return true;
    }

    inline void Join() {
        Thread->Join();
    }

private:
    TReader* Reader;
    NThreading::TBlockingQueue<TMaybe<TData> > Queue;
    TAutoPtr<IThreadFactory::IThread> Thread;

    inline void DoExecute() {
        bool next;

        do {
            TData data;

            if (next = Reader->Read(&data)) {
                Queue.Push(std::move(data));
            }
        } while (next);

        Queue.Push(TMaybe<TData>());
    }
};

}  // namespace NAapi
