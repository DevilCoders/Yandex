#pragma once

#include <google/protobuf/message.h>

#include <kikimr/persqueue/sdk/deprecated/cpp/v2/persqueue.h>

#include <util/system/mutex.h>
#include <util/generic/vector.h>
#include <util/generic/deque.h>

class TPqReader : TNonCopyable {
public:
    TPqReader(const NPersQueue::TConsumerSettings& settings, TIntrusivePtr<NPersQueue::TCerrLogger> logger);
    NPersQueue::TReadResponse ReadBatch();
    void Commit(const TVector<ui64>& cookies);

    static void Interrupt() {
        Interrupted = true;
    }

private:
    void RestartConsumer();

private:
    THolder<NPersQueue::TPQLib> PQ;
    NPersQueue::TConsumerSettings ConsumerSettings;
    TIntrusivePtr<NPersQueue::TCerrLogger> Logger;

    TMutex ConsumerLock;
    THolder<NPersQueue::IConsumer> Consumer;

    static bool Interrupted;
};
