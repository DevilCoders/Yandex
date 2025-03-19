#include "vhost-queue.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <new>

namespace NVHostQueue {

////////////////////////////////////////////////////////////////////////////////

TQueue::TQueue(void* addr)
    : KickFd(eventfd(0, EFD_NONBLOCK))
    , CallFd(eventfd(0, EFD_NONBLOCK))
    , ErrFd(eventfd(0, EFD_NONBLOCK))
    , PollFds{pollfd{ .fd = ErrFd,
                      .events = POLLIN,
                      .revents = 0 },
              pollfd{ .fd = CallFd,
                      .events = POLLIN,
                      .revents = 0 }}
{
    char* tempAddr = reinterpret_cast<char*>(addr);

    Descriptors = new(tempAddr) TDescriptorTable{};
    tempAddr += sizeof(*Descriptors) * QUEUE_SIZE;

    AvailableRings = new(tempAddr) TAvailableRing{};
    tempAddr += sizeof(*AvailableRings);

    UsedRings = new(tempAddr) TUsedRing{};
    tempAddr += sizeof(*UsedRings);

    DataAddr = tempAddr;

    for (int i = 0; i < static_cast<int>(QUEUE_SIZE); ++i) {
        FreeBuffers.push(i);
    }
}

TQueue::TQueue(TQueue&& other)
    : Descriptors(other.Descriptors)
    , AvailableRings(other.AvailableRings)
    , UsedRings(other.UsedRings)
    , DataAddr(other.DataAddr)
    , KickFd(other.KickFd)
    , CallFd(other.CallFd)
    , ErrFd(other.ErrFd)
    , PollFds(other.PollFds)
    , FreeBuffers(std::move(other.FreeBuffers))
{
    other.Descriptors = nullptr;
    other.AvailableRings = nullptr;
    other.UsedRings = nullptr;
    other.DataAddr = nullptr;
    other.KickFd = -1;
    other.CallFd = -1;
    other.ErrFd = -1;
    other.PollFds = {};
}

TQueue::~TQueue()
{
    if (ErrFd > 0) {
        close(ErrFd);
    }

    if (CallFd > 0) {
        close(CallFd);
    }

    if (KickFd > 0) {
        close(KickFd);
    }
}

TQueue& TQueue::operator=(TQueue&& other)
{
    if (&other == this) {
        return *this;
    }

    if (ErrFd > 0) {
        close(ErrFd);
    }

    if (CallFd > 0) {
        close(CallFd);
    }

    if (KickFd > 0) {
        close(KickFd);
    }

    Descriptors = other.Descriptors;
    AvailableRings = other.AvailableRings;
    UsedRings = other.UsedRings;
    DataAddr = other.DataAddr;
    KickFd = other.KickFd;
    CallFd = other.CallFd;
    ErrFd = other.ErrFd;
    PollFds = other.PollFds;
    FreeBuffers = std::move(other.FreeBuffers);

    other.Descriptors = nullptr;
    other.AvailableRings = nullptr;
    other.UsedRings = nullptr;
    other.DataAddr = nullptr;
    other.KickFd = -1;
    other.CallFd = -1;
    other.ErrFd = -1;
    other.PollFds = {};

    return *this;
}

void* TQueue::GetDescriptorsAddr() const
{
    return Descriptors;
}

void* TQueue::GetAvailableRingsAddr() const
{
    return AvailableRings;
}

void* TQueue::GetUsedRings() const
{
    return UsedRings;
}

int TQueue::GetKickFd() const
{
    return KickFd;
}

int TQueue::GetCallFd() const
{
    return CallFd;
}

int TQueue::GetErrFd() const
{
    return ErrFd;
}

bool TQueue::Write(const TVector<char>& buffer, int timeoutMs)
{
    // В текущей реализации записывать можно только кусок памяти не больше QUEUE_BUFFER_SIZE байт
    // Для упрощения жизни мы не проверяем UsedRing, а просто ждем эвента CALL
    // Подразумевается что отправляем мы одно кольцо с одним буфером,
    //   а значит CALL говорит о том, что оно считано и теперь доступно

    if (buffer.size() > QUEUE_BUFFER_SIZE) {
        return false;
    }

    if (buffer.size() == 0) {
        return true;
    }

    const size_t idx = FreeBuffers.front();
    FreeBuffers.pop();

    Descriptors[idx].Addr = reinterpret_cast<uint64_t>(DataAddr) +
        idx * QUEUE_BUFFER_SIZE;
    memcpy(
        reinterpret_cast<void*>(Descriptors[idx].Addr),
        buffer.data(),
        buffer.size());
    Descriptors[idx].Flags = 0;
    Descriptors[idx].Len = buffer.size();
    Descriptors[idx].Next = 0;

    AvailableRings->Flags = 0;
    AvailableRings->Ring[idx] = idx;
    AvailableRings->UsedEvent = 0;

    ++AvailableRings->Idx;

    if (eventfd_write(KickFd, 1) < 0) {
        FreeBuffers.push(idx);
        return false;
    }

    if (!WaitCallEvent(timeoutMs)) {
        FreeBuffers.push(idx);
        return false;
    }

    FreeBuffers.push(idx);
    return true;
}

bool TQueue::Write(
    const TVector<char>& inBuffer,
    TVector<char>& outBuffer,
    int timeoutMs)
{
    if (inBuffer.size() > QUEUE_BUFFER_SIZE) {
        return false;
    }

    if (inBuffer.size() == 0) {
        return true;
    }

    const size_t inIdx = FreeBuffers.front();
    FreeBuffers.pop();

    const size_t outIdx = FreeBuffers.front();
    FreeBuffers.pop();

    AvailableRings->Flags = 0;
    AvailableRings->Ring[inIdx] = inIdx;
    AvailableRings->UsedEvent = 0;
    ++AvailableRings->Idx;

    Descriptors[inIdx].Addr = reinterpret_cast<uint64_t>(DataAddr) +
        inIdx * QUEUE_BUFFER_SIZE;
    memcpy(
        reinterpret_cast<void*>(Descriptors[inIdx].Addr),
        inBuffer.data(),
        inBuffer.size());
    Descriptors[inIdx].Flags = VIRTQ_DESC_F_NEXT;
    Descriptors[inIdx].Len = inBuffer.size();
    Descriptors[inIdx].Next = outIdx;

    Descriptors[outIdx].Addr = reinterpret_cast<uint64_t>(DataAddr) +
        outIdx * QUEUE_BUFFER_SIZE;
    Descriptors[outIdx].Flags = VIRTQ_DESC_F_WRITE;
    Descriptors[outIdx].Len = QUEUE_BUFFER_SIZE;
    Descriptors[outIdx].Next = 0;

    if (eventfd_write(KickFd, 1) < 0) {
        FreeBuffers.push(inIdx);
        FreeBuffers.push(outIdx);
        return false;
    }

    if (!WaitCallEvent(timeoutMs)) {
        FreeBuffers.push(inIdx);
        FreeBuffers.push(outIdx);
        return false;
    }

    outBuffer.resize(QUEUE_BUFFER_SIZE);
    memcpy(
        outBuffer.data(),
        reinterpret_cast<void*>(Descriptors[outIdx].Addr),
        QUEUE_BUFFER_SIZE);

    FreeBuffers.push(inIdx);
    FreeBuffers.push(outIdx);

    return true;
}

bool TQueue::WaitCallEvent(int timeoutMs)
{
    if (poll(PollFds.data(), PollFds.size(), timeoutMs) < 0) {
        return false;
    }

    uint64_t event;
    if ((PollFds[0].revents & POLLIN))
    {
        eventfd_read(PollFds[0].fd, &event);
        return false;
    }

    if ((PollFds[0].revents & POLLERR) |
        (PollFds[0].revents & POLLHUP) |
        (PollFds[0].revents & POLLNVAL) |
        (PollFds[1].revents & POLLERR) |
        (PollFds[1].revents & POLLHUP) |
        (PollFds[1].revents & POLLNVAL))
    {
        return false;
    }

    if (PollFds[1].revents & POLLIN) {
        return eventfd_read(PollFds[1].fd, &event) >= 0;
    }

    return false;
}

} // namespace NVHostQueue
