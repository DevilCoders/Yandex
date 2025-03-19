#include "pq_reader.h"

#include <util/string/join.h>

using namespace NPersQueue;

bool TPqReader::Interrupted = false;

template <class T>
bool WaitFuture(T& future, bool& interrupted) {
    while (true) {
        future.Wait(TDuration::Seconds(1));
        if (interrupted) {
            return false;
        }
        if (future.HasValue())
            return true;
    }
}

TPqReader::TPqReader(const TConsumerSettings& settings, TIntrusivePtr<TCerrLogger> logger)
    : ConsumerSettings(settings)
    , Logger(logger)
{
    TPQLibSettings config;
    config.ThreadsCount = 3;
    PQ.Reset(new TPQLib(config));
    RestartConsumer();
}

void TPqReader::RestartConsumer() {
    TGuard<TMutex> g(ConsumerLock);
    Consumer.Reset(PQ->CreateConsumer(ConsumerSettings, Logger, true));

    auto future = Consumer->Start();

    if (WaitFuture(future, Interrupted)) {
        Cerr << future.GetValue().Response << Endl;
    } else {
        Cerr << "Interrupted" << Endl;
    }
}

TReadResponse TPqReader::ReadBatch() {
    Cerr << "READ_BATCH\n";

    while (true) {
        NThreading::TFuture<TConsumerMessage> msg;
        {
            TGuard<TMutex> g(ConsumerLock);
            msg = Consumer->GetNextMessage();
        }

        if (!WaitFuture(msg, Interrupted)) {
            return TReadResponse();
        }

        switch (msg.GetValue().Type) {
            case EMT_ERROR:
                Cerr << "Got error while reading next message from PQ: " << msg.GetValue().Response << Endl;
                Cerr << "Restarting consumer" << Endl;
                RestartConsumer();
                continue;
                break;
            case EMT_LOCK:
                msg.GetValue().ReadyToRead.SetValue(TLockInfo{0, 0, false});
                Cerr << "Locked, now parts" << Endl;
                continue;
                break;
            case EMT_RELEASE:
                Cerr << "Released, now\n";
                continue;
                break;
            case EMT_DATA: {
                return msg.GetValue().Response;
                break;
            }
            case EMT_COMMIT:
                if (msg.GetValue().Response.HasError()) {
                    Cerr << "Got error while committing offsets to PQ: " << msg.GetValue().Response << Endl;
                    Cerr << "Restarting consumer" << Endl;
                    RestartConsumer();
                }
                continue;
                break;
            default:
                Cerr << "Unsupported response:\n"
                     << msg.GetValue().Response << Endl;
                exit(1);
        }
    }
}

void TPqReader::Commit(const TVector<ui64>& cookies) {
    Cerr << "COMMIT: " << JoinSeq(", ", cookies) << Endl;

    TGuard<TMutex> g(ConsumerLock);
    Consumer->Commit(cookies);
}
