#include "neh_requester.h"

namespace NAntiRobot {
    TNehRequester::TNehRequester(size_t maxQueueSize)
        : MaxQueueSize(maxQueueSize)
        , DispatcherThread(DispatchLoop, this)
    {
        Client = NNeh::CreateMultiClient();
        DispatcherThread.Start();
    }

    NThreading::TFuture<TErrorOr<NNeh::TResponseRef>>
    TNehRequester::RequestAsync(const NNeh::TMessage& msg, TInstant deadline) {
        if (Client->QueueSize() >= MaxQueueSize) {
            return NThreading::MakeFuture<TErrorOr<NNeh::TResponseRef>>(TError(__LOCATION__ + TNehQueueOverflowException()));
        }

        auto promisePtr = new NThreading::TPromise<TErrorOr<NNeh::TResponseRef>>();
        *promisePtr = NThreading::NewPromise<TErrorOr<NNeh::TResponseRef>>();
        NNeh::IMultiClient::TRequest req(msg, deadline, promisePtr);
        auto result = promisePtr->GetFuture();

        try {
            Client->Request(req);
        } catch (...) {
            promisePtr->SetValue(TError(__LOCATION__ + yexception() << CurrentExceptionMessage()));
            delete promisePtr;
        }

        return result;
    }

    TNehRequester::~TNehRequester() {
        Client->Interrupt();
        DispatcherThread.Join();
    }

    void* TNehRequester::DispatchLoop(void* ptr) {
        TNehRequester* thisptr = reinterpret_cast<TNehRequester*>(ptr);
        NNeh::IMultiClient::TEvent event;

        while (thisptr->Client->Wait(event)) {
            auto promisePtr = reinterpret_cast<NThreading::TPromise<TErrorOr<NNeh::TResponseRef>>*>(event.UserData);

            if (event.Type == NNeh::IMultiClient::TEvent::Response) {
                try {
                    promisePtr->SetValue(event.Hndl->Get());
                } catch (...) {
                    Cerr << "DEBUG_CAPTCHA-1464 DispatchLoop Set value " << CurrentExceptionMessage();
                }
            } else if (event.Type == NNeh::IMultiClient::TEvent::Timeout) {
                try {
                    promisePtr->SetValue(TError(__LOCATION__ + TTimeoutException()));
                } catch (...) {
                    Cerr << "DEBUG_CAPTCHA-1464 DispatchLoop Set timeout " << CurrentExceptionMessage();
                }
            }

            delete promisePtr;
        }

        return nullptr;
    }
} // namespace NAntiRobot
