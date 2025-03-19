#pragma once

#include <kernel/common_server/util/queue.h>

#include <library/cpp/http/server/http.h>
#include <library/cpp/http/static/static.h>

struct THttpServerQueues {
public:
    using TMtpQueueRef = TSimpleSharedPtr<TCountedMtpQueue>;
    using THttpServerMtpQueue = TThreadPoolBinder<TSimpleThreadPool, THttpServer::ICallBack>;

public:
    THttpServerQueues(THttpServer::ICallBack* cb);

    ui64 GetBusyThreadsCount() const {
        return MainQueue ? MainQueue->GetBusyThreadsCount() : 0;
    }
    ui64 GetFreeThreadsCount() const {
        return MainQueue ? MainQueue->GetFreeThreadsCount() : 0;
    }
    ui64 Size() const {
        return MainQueue ? MainQueue->Size() : 0;
    }

protected:
    TMtpQueueRef MainQueue;
    TMtpQueueRef FailQueue;
};

struct TSearchServerBase
    : THttpServer::ICallBack
    , THttpServerQueues
    , THttpServer
    , CHttpServerStatic
{
private:
    bool Running;

public:
    TSearchServerBase(const THttpServerOptions& options);

    bool ClientConnected();
    void Run();
    virtual void Stop();

    virtual void OnFailRequestEx(const TFailLogData& d) override;
    virtual void OnException() override;
    virtual void OnMaxConn() override;

    virtual void OnListenStart() override { Running = true; }
    virtual void OnListenStop() override { Running = false; }
    bool IsRunning() { return Running; }
};

template <class TConfigType>
class TSearchServerAbstract: public TSearchServerBase {
public:
    typedef TConfigType TConfig;

protected:
    const TConfig& Config;

public:
    inline TSearchServerAbstract(const TConfig& config)
        : TSearchServerBase(config.GetHttpServerOptions())
        , Config(config)
    {
    }

    inline const TConfig& GetConfig() const
    {
        return Config;
    }
};

template <class TClientType, class TConfigType>
class TCommonSearchServer: public TSearchServerAbstract<TConfigType> {
public:
    typedef TCommonSearchServer<TClientType,TConfigType> TServer;
    inline TCommonSearchServer(const TConfigType& config)
        : TSearchServerAbstract<TConfigType>(config)
    {
    }

    virtual TClientRequest* CreateClient() {
        return new TClientType;
    }
};
