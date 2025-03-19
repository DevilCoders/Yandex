#include "vhost_test.h"

#include <cloud/storage/core/libs/common/error.h>

#include <util/folder/path.h>
#include <util/system/mutex.h>
#include <util/thread/lfqueue.h>

namespace NCloud::NBlockStore::NVhost {

using namespace NThreading;

namespace {

////////////////////////////////////////////////////////////////////////////////

class TTestVhostRequest final
    : public TVhostRequest
{
private:
    TPromise<EResult> Promise;

public:
    TTestVhostRequest(
            TPromise<EResult> promise,
            EBlockStoreRequest type,
            ui64 from,
            ui64 length,
            TSgList sgList,
            void* cookie)
        : Promise(std::move(promise))
    {
        Type = type;
        From = from;
        Length = length;
        SgList.SetSgList(std::move(sgList));
        Cookie = cookie;
    }

    void Complete(EResult result) override
    {
        Promise.SetValue(result);
    }
};

////////////////////////////////////////////////////////////////////////////////

class TTestVhostDevice final
    : public ITestVhostDevice
    , public IVhostDevice
{
private:
    const TString SocketPath;
    void* Cookie;

    TLockFreeQueue<TVhostRequest*> Requests;

    bool Stopped = false;

public:
    TTestVhostDevice(TString socketPath, void* cookie)
        : SocketPath(std::move(socketPath))
        , Cookie(cookie)
    {}

    bool Start() override
    {
        TFsPath(SocketPath).Touch();
        return true;
    }

    TFuture<NProto::TError> Stop() override
    {
        Stopped = true;
        return MakeFuture(NProto::TError());
    }

    bool IsStopped() override
    {
        return Stopped;
    }

    TFuture<TVhostRequest::EResult> SendTestRequest(
        EBlockStoreRequest type,
        ui64 from,
        ui64 length,
        TSgList sgList) override
    {
        auto promise = NewPromise<TVhostRequest::EResult>();
        auto future = promise.GetFuture();
        auto request = std::make_unique<TTestVhostRequest>(
            std::move(promise),
            type,
            from,
            length,
            std::move(sgList),
            Cookie);
        Requests.Enqueue(request.release());
        return future;
    }

    TVhostRequestPtr DequeueRequest()
    {
        TVhostRequest* request;
        if (Requests.Dequeue(&request)) {
            return std::unique_ptr<TVhostRequest>(request);
        }
        return nullptr;
    }
};

////////////////////////////////////////////////////////////////////////////////

class TTestVhostQueue final
    : public ITestVhostQueue
    , public IVhostQueue
{
private:
    TManualEvent& FailedEvent;

    enum {
        Undefined = 0,
        Running = 1,
        Stopped = 2,
        Broken = 3,
    };
    TAtomic State = Undefined;

    TMutex Lock;
    TVector<std::weak_ptr<TTestVhostDevice>> Devices;

public:
    TTestVhostQueue(TManualEvent& failedEvent)
        : FailedEvent(failedEvent)
    {}

    int Run() override
    {
        AtomicCas(&State, Running, Undefined);

        switch (AtomicGet(State)) {
            case Running:
                return -EAGAIN;
            case Stopped:
                return 0;
            case Broken:
                FailedEvent.Signal();
                return -1;
            default:
                Y_FAIL();
        }
    }

    void Stop() override
    {
        bool wasRun = AtomicCas(&State, Stopped, Running);
        Y_VERIFY(wasRun || AtomicGet(State) == Broken);
    }

    IVhostDevicePtr CreateDevice(
        TString socketPath,
        TString deviceName,
        ui32 blockSize,
        ui64 blocksCount,
        ui32 queuesCount,
        void* cookie,
        NRdma::TRegisterMemory* registerMemory,
        NRdma::TUnregisterMemory* unregisterMemory) override
    {
        Y_UNUSED(deviceName);
        Y_UNUSED(blockSize);
        Y_UNUSED(blocksCount);
        Y_UNUSED(queuesCount);
        Y_UNUSED(registerMemory);
        Y_UNUSED(unregisterMemory);

        auto vhostDevice = std::make_shared<TTestVhostDevice>(
            std::move(socketPath),
            cookie);

        with_lock (Lock) {
            Devices.push_back(vhostDevice);
        }
        return vhostDevice;
    }

    TVhostRequestPtr DequeueRequest() override
    {
        if (AtomicGet(State) == Running) {
            with_lock (Lock) {
                for (auto& device: Devices) {
                    if (auto ptr = device.lock()) {
                        auto request = ptr->DequeueRequest();
                        if (request) {
                            return request;
                        }
                    }
                }
            }
        }
        return nullptr;
    }

    bool IsRun() override
    {
        return AtomicGet(State) == Running;
    }

    TVector<std::shared_ptr<ITestVhostDevice>> GetDevices() override
    {
        TVector<std::shared_ptr<ITestVhostDevice>> res;
        with_lock (Lock) {
            for (auto& device: Devices) {
                if (auto ptr = device.lock()) {
                    res.push_back(ptr);
                }
            }
        }
        return res;
    }

    void Break() override
    {
        AtomicSet(State, Broken);
    }
};

}   // namespace

////////////////////////////////////////////////////////////////////////////////

IVhostQueuePtr TTestVhostQueueFactory::CreateQueue()
{
    auto queue = std::make_shared<TTestVhostQueue>(FailedEvent);
    Queues.push_back(queue);
    return queue;
}

}   // namespace NCloud::NBlockStore::NVhost
