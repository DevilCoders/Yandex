#pragma once

#include <util/string/cast.h>
#include <util/thread/factory.h>

#include <library/cpp/neh/rpc.h>
#include <util/system/event.h>

namespace NAapi {

class TPingThread: public IThreadFactory::IThreadAble {
public:
    TPingThread(const TString& host, ui64 port)
        : Host(host)
        , Port(port)
    {
        Thread = SystemThreadFactory()->Run(this);
    }

    ~TPingThread() {
        Stopped.Signal();
        Thread->Join();
    }

private:
    class TPingService: public NNeh::IService {
    public:
        explicit TPingService() {
        }

        void ServeRequest(const NNeh::IRequestRef& request) override {
            TString dataStr;
            NNeh::TDataSaver dataSaver;
            dataSaver.DoWrite(dataStr.data(), dataStr.size());
            request->SendReply(dataSaver);
        }
    };

    TString Host;
    ui64 Port;
    TAutoEvent Stopped;

    THolder<IThreadFactory::IThread> Thread;

    void DoExecute() override {
        NNeh::IServicesRef httpServer = NNeh::CreateLoop();
        httpServer->Add("http2://" + Host + ":" + ToString(Port) + "/proxy-ping", NNeh::IServiceRef(new TPingService()));
        httpServer->ForkLoop(2);
        Stopped.Wait();
    }
};

}  // namespace NAapi
