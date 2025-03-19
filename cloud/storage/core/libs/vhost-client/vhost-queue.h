#pragma once

#include <util/generic/vector.h>

#include <inttypes.h>
#include <poll.h>
#include <stdlib.h>

#include <array>
#include <queue>


namespace NVHostQueue {

////////////////////////////////////////////////////////////////////////////////

class TQueue
{
private:
    enum EDescriptorFlags
    {
        //Don't rename these constants. Names are taken from specification
        VIRTQ_DESC_F_NEXT     = 0x1,
        VIRTQ_DESC_F_WRITE    = 0x2,
        VIRTQ_DESC_F_INDIRECT = 0x4
    };

    static constexpr size_t QUEUE_SIZE = 128;
    static constexpr size_t QUEUE_BUFFER_SIZE = 512;

    struct alignas(16) TDescriptorTable
    {
        uint64_t Addr;
        uint32_t Len;
        uint16_t Flags;
        uint16_t Next;
    };

    enum EAvailableFlags
    {
        //Don't rename these constants. Names are taken from specification
        VIRTQ_AVAIL_F_NO_INTERRUPT = 1
    };

    struct alignas(2) TAvailableRing
    {
        uint16_t Flags;
        uint16_t Idx;
        uint16_t Ring[QUEUE_SIZE];
        uint16_t UsedEvent;
    };

    struct alignas(4) TUsedRing
    {
        uint16_t Flags;
        uint16_t Idx;
        struct alignas(4)
        {
            uint32_t Id;
            uint32_t Len;
        } Ringp[QUEUE_SIZE];
        uint16_t avail_event;
    };

    TDescriptorTable* Descriptors = nullptr;
    TAvailableRing* AvailableRings = nullptr;
    TUsedRing* UsedRings = nullptr;
    void* DataAddr = nullptr;
    int KickFd = -1;
    int CallFd = -1;
    int ErrFd = -1;
    std::array<pollfd, 2> PollFds = {};
    std::queue<int> FreeBuffers;

public:
    static constexpr size_t GetQueueMemSize()
    {
        return
            QUEUE_SIZE *
            QUEUE_BUFFER_SIZE +
            sizeof(TDescriptorTable) * QUEUE_SIZE +
            sizeof(TAvailableRing) +
            sizeof(TUsedRing);
    }

    static constexpr size_t GetQueueSize()
    {
        return QUEUE_SIZE;
    }

    TQueue() = default;
    explicit TQueue(void* addr);
    TQueue(const TQueue&) = delete;
    TQueue(TQueue&& other);
    ~TQueue();

    TQueue& operator=(const TQueue&) = delete;
    TQueue& operator=(TQueue&& other);

    void* GetDescriptorsAddr() const;
    void* GetAvailableRingsAddr() const;
    void* GetUsedRings() const;

    int GetKickFd() const;
    int GetCallFd() const;
    int GetErrFd() const;

    bool Write(const TVector<char>& buffer, int timeoutMs = -1);
    bool Write(const TVector<char>& inBuffer, TVector<char>& outBuffer, int timeoutMs = -1);
    bool WaitCallEvent(int timeoutMs = -1);
};

} // namespace NVHostQueue
