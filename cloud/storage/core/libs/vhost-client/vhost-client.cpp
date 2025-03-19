#include "vhost-client.h"

#include "vhost-user-protocol/msg-get-feature.h"
#include "vhost-user-protocol/msg-get-protocol-feature.h"
#include "vhost-user-protocol/msg-get-queue-num.h"
#include "vhost-user-protocol/msg-get-vring-base.h"
#include "vhost-user-protocol/msg-set-feature.h"
#include "vhost-user-protocol/msg-set-mem-table.h"
#include "vhost-user-protocol/msg-set-owner.h"
#include "vhost-user-protocol/msg-set-protocol-feauture.h"
#include "vhost-user-protocol/msg-set-vring-addr.h"
#include "vhost-user-protocol/msg-set-vring-base.h"
#include "vhost-user-protocol/msg-set-vring-call.h"
#include "vhost-user-protocol/msg-set-vring-err.h"
#include "vhost-user-protocol/msg-set-vring-kick.h"
#include "vhost-user-protocol/msg-set-vring-num.h"

#include <util/generic/string.h>
#include <util/string/printf.h>

#include <unistd.h>

#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/un.h>

namespace NVHost {

////////////////////////////////////////////////////////////////////////////////

TClient::TClient(TString sockPath)
    : IsInit(false)
    , SockPath(std::move(sockPath))
    , Sock(-1)
    , Logger(CreateLogBackend("console", TLOG_INFO))
{}

TClient::TClient(TClient&& other)
    : IsInit(other.IsInit)
    , SockPath(std::move(other.SockPath))
    , Sock(other.Sock)
    , Logger(std::move(other.Logger))
{
    other.IsInit = false;
    other.Sock = -1;
}

TClient::~TClient()
{
    DeInit();
}

TClient& TClient::operator=(TClient&& other)
{
    if (&other == this) {
        return *this;
    }

    DeInit();

    IsInit = other.IsInit;
    SockPath = std::move(other.SockPath);
    Sock = other.Sock;
    Logger = std::move(other.Logger);

    other.IsInit = false;
    other.Sock = -1;

    return *this;
}

bool TClient::Execute(NVHostUser::IMessage& msg)
{
    auto requestStr = msg.ToString();
    if(!msg.Execute(Sock)) {
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: error %s. Request %s\n",
            __FUNCTION__,
            strerror(errno),
            requestStr.c_str());
        return false;
    }
    Logger.AddLog(
        ELogPriority::TLOG_DEBUG,
        "%s: Request %s OK\n",
        __FUNCTION__,
        requestStr.c_str());
    return true;
}

bool TClient::Connect()
{
    if ((Sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: Socket error. Error code %d\n",
            __FUNCTION__,
            Sock);
        return false;
    }

    struct sockaddr_un un;
    un.sun_family = AF_UNIX;
    strlcpy(un.sun_path, SockPath.c_str(), sizeof(un.sun_path));

    if (int errorCode = connect(
        Sock,
        reinterpret_cast<sockaddr*>(&un),
        sizeof(un.sun_family) + SockPath.Size()) == -1)
    {
        close(Sock);
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: Connect error. Error code %d\n",
            __FUNCTION__,
            errorCode);
        return false;
    }

    return true;
}

bool TClient::CoordinationFeatures(
    uint64_t virtioFeatures,
    uint64_t virtioProtocolFeatures)
{
    NVHostUser::TGetFeature getFuatureMsg;
    if (!Execute(getFuatureMsg)) {
        return false;
    }

    if (!BitIsSet(
        getFuatureMsg.GetResult(),
        NVHostUser::TGetFeature::VHOST_USER_F_PROTOCOL_FEATURES))
    {
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: Coordination error VHOST_USER_F_PROTOCOL_FEATURES\n",
            __FUNCTION__);
        return false;
    }

    NVHostUser::TSetFeature setFeatureMsg(virtioFeatures);
    if (!Execute(setFeatureMsg)) {
        return false;
    }

    NVHostUser::TGetProtocolFeature getProtocolFeature;
    if (!Execute(getProtocolFeature)) {
        return false;
    }

    NVHostUser::TSetProtocolFeature setProtocolFeatureMsg(virtioProtocolFeatures);
    if (!Execute(setProtocolFeatureMsg)) {
        return false;
    }

    return true;
}

bool TClient::CoordinationMemMap(size_t size)
{
    TVector<NVHostUser::TSetMemTable::MemoryRegion> regions;
    TVector<int> fds;

    //size of memory must be power of 2
    int index = 0;
    while(size) {
        size >>= 1;
        ++index;
    }
    size = 1 << index;

    for (auto& region: MemTable) {
        region.Fd = syscall(__NR_memfd_create, "vhost-client-memtable", 0);
        if (region.Fd < 0) {
            Logger.AddLog(
                ELogPriority::TLOG_ERR,
                "%s: syscall error %s\n",
                __FUNCTION__,
                strerror(errno));
            return false;
        }

        if (ftruncate(region.Fd, size) < 0) {
            Logger.AddLog(
                ELogPriority::TLOG_ERR,
                "%s: ftruncate error %s\n",
                __FUNCTION__,
                strerror(errno));
            return false;
        }

        region.Size = size;
        region.MemAddr = mmap(
            NULL,
            region.Size,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            region.Fd,
            0);

        if (region.MemAddr == MAP_FAILED) {
            Logger.AddLog(
                ELogPriority::TLOG_ERR,
                "%s: mmap error %s\n",
                __FUNCTION__,
                strerror(errno));
            return false;
        }

        regions.push_back(NVHostUser::TSetMemTable::MemoryRegion{
            .GuestAddress = reinterpret_cast<uint64_t>(region.MemAddr),
            .Size = region.Size,
            .UserAddress = reinterpret_cast<uint64_t>(region.MemAddr),
            .MmapOffset = 0});

        fds.push_back(region.Fd);
    }

    NVHostUser::TSetMemTable setMemTableMsg(std::move(regions), std::move(fds));
    if (!Execute(setMemTableMsg)) {
        return false;
    }

    return true;
}

bool TClient::CoordinationQueue()
{
    for (size_t i = 0; i < QUEUE_COUNT; ++i) {
        NVHostUser::TSetVringNum setVringNumMsg(
            i, NVHostQueue::TQueue::GetQueueSize());
        if (!Execute(setVringNumMsg)) {
            return false;
        }

        NVHostUser::TSetVringBase setVringBaseMsg(i, 0);
        if (!Execute(setVringBaseMsg)) {
            return false;
        }

        void* addr =
            reinterpret_cast<char*>(MemTable[i].MemAddr) +
            i * NVHostQueue::TQueue::GetQueueMemSize();
        Queues[i] = NVHostQueue::TQueue(addr);

        NVHostUser::TSetVringAddr setVringAddrMsg(
            i,
            0,
            reinterpret_cast<uint64_t>(Queues[i].GetDescriptorsAddr()),
            reinterpret_cast<uint64_t>(Queues[i].GetUsedRings()),
            reinterpret_cast<uint64_t>(Queues[i].GetAvailableRingsAddr()),
            0);
        if (!Execute(setVringAddrMsg)) {
            return false;
        }

        NVHostUser::TSetVringErr setVringErrMsg(i, Queues[i].GetErrFd());
        if (!Execute(setVringErrMsg)) {
            return false;
        }

        NVHostUser::TSetVringKick setVringKickMsg(i, Queues[i].GetKickFd());
        if (!Execute(setVringKickMsg)) {
            return false;
        }

        NVHostUser::TSetVringCall setVringCallMsg(i, Queues[i].GetCallFd());
        if (!Execute(setVringCallMsg)) {
            return false;
        }

        if (!Queues[i].WaitCallEvent()) {
            return false;
        }
    }

    return true;
}

 void TClient::DeInit()
 {
    for (auto& region: MemTable) {
        if (region.Fd != -1) {
            close(region.Fd);
            region.Fd = -1;
        }
    }

    if (Sock != -1) {
        close(Sock);
        Sock = -1;
    }

    IsInit = false;
 }

bool TClient::Init()
{
    if (IsInit)
    {
        return true;
    }

    if(!Connect()) {
        return false;
    }

    uint64_t virtioFeatures = 0;
    NVHostUser::SetBit(
        virtioFeatures,
        NVHostUser::TGetFeature::VIRTIO_F_VERSION_1);

    uint64_t virtioProtocolFeatures = 0;
    NVHostUser::SetBit(
        virtioProtocolFeatures,
        NVHostUser::TGetProtocolFeature::VHOST_USER_PROTOCOL_F_MQ);

    if(!CoordinationFeatures(virtioFeatures, virtioProtocolFeatures)) {
        return false;
    }

    NVHostUser::TSetOwner setOwnerMsg;
    if (!Execute(setOwnerMsg)) {
        return false;
    }

    NVHostUser::TGetQueueNum getQueueNumMsg;
    if (!Execute(getQueueNumMsg)) {
        return false;
    }

    if (getQueueNumMsg.GetResult() != QUEUE_COUNT) {
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: Queue count error %lu\n",
            __FUNCTION__,
            getQueueNumMsg.GetResult());
        return false;
    }

    if (!CoordinationMemMap(NVHostQueue::TQueue::GetQueueMemSize()))
    {
        return false;
    }

    if (!CoordinationQueue()){
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: queue error %s\n",
            __FUNCTION__,
            strerror(errno));
        return false;
    }

    IsInit = true;
    return true;
}

bool TClient::Write(const TVector<char>& data, int timeoutMs)
{
    if (!TClient::Init()) {
        return false;
    }

    if (!Queues[0].Write(data, timeoutMs)) {
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: error %s\n",
            __FUNCTION__,
            strerror(errno));
        return false;
    }
    return true;
}

bool TClient::Write(
    const TVector<char>& inBuffer,
    TVector<char>& outBuffer,
    int timeoutMs)
{
    if (!TClient::Init()) {
        return false;
    }

    if (!Queues[0].Write(inBuffer, outBuffer, timeoutMs)) {
        Logger.AddLog(
            ELogPriority::TLOG_ERR,
            "%s: error %s\n",
            __FUNCTION__,
            strerror(errno));
        return false;
    }
    return true;
}

} // namespace NVHost
