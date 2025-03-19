#pragma once
//https://stefanha.github.io/virtio/vhost-user-slave.html

#include "vhost-queue.h"
#include "vhost-user-protocol/message.h"

#include <util/generic/string.h>

#include <library/cpp/logger/log.h>

#include <array>

namespace NVHost {

////////////////////////////////////////////////////////////////////////////////

struct VhostUserMsg;

class TClient
{
private:
    static constexpr size_t QUEUE_COUNT = 2;

    struct TMemTable
    {
        int Fd = -1;
        size_t Size = 0;
        void* MemAddr = nullptr;
    };

    bool IsInit;
    TString SockPath;
    int Sock;
    TLog Logger;

    std::array<TMemTable, QUEUE_COUNT> MemTable;
    std::array<NVHostQueue::TQueue, QUEUE_COUNT> Queues;

    bool Execute(NVHostUser::IMessage& msg);

    bool Connect();
    bool CoordinationFeatures(
        uint64_t virtioFeatures = 0,
        uint64_t virtioProtocolFeatures = 0);
    bool CoordinationMemMap(size_t size);
    bool CoordinationQueue();

public:
    explicit TClient(TString sockPath);
    TClient(const TClient&) = delete;
    TClient(TClient&& other);
    ~TClient();

    TClient& operator=(const TClient&) = delete;
    TClient& operator=(TClient&& other);

    bool Write(const TVector<char>& buffer, int timeoutMs = -1);
    bool Write(const TVector<char>& inBuffer, TVector<char>& outBuffer, int timeoutMs = -1);

    bool Init();
    void DeInit();
};

} // namespace NVHost
