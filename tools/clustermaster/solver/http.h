#pragma once

#include "request.h"

#include <tools/clustermaster/communism/util/http_util.h>

#include <util/generic/noncopyable.h>
#include <util/system/hostname.h>

struct TSolverHttpRequestLogger {
    inline static void Log(const TString&) {}
    inline static void ErrorLog(const TString&) {}
};

class TSolverHttpRequest: public THttpClientRequestForHuman<TSolverHttpRequestLogger>, TNonCopyable {
public:
    TSolverHttpRequest()
    {
    }
private:
    TRequestsHandle GetRequests();
    const TString& GetHostName();

    void ServeSimpleStatus(IOutputStream&, HttpCodes, TMaybe<THttpRequestContext>);
    void ReplyUnsafe(IOutputStream&) override;
    void ServeStaticData(IOutputStream& out, const char* name, const char* mime);

    void ServeSummary(IOutputStream& out, const THttpRequestContext& context);
    void ServeConsumedText(IOutputStream& out);

    enum ERequestsType {
        RT_REQUESTED,
        RT_GRANTED
    };

    void ServeRequestsText(IOutputStream& out, ERequestsType type);
    void ServeRequestedText(IOutputStream& out);
    void ServeGrantedText(IOutputStream& out);
};

class TSolverHttpServer: public THttpServer, THttpServer::ICallBack {
public:
    TSolverHttpServer(THttpServer::TOptions options, TRequestsHandle requests)
        : THttpServer(this, options)
        , Requests(requests)
    {
        auto hostname = HostName();
        auto hostnameSb = TStringBuf(hostname);
        hostnameSb.ChopSuffix(".yandex.ru");
        SelfHostName = hostnameSb;
    }

    TRequestsHandle GetRequests() { return Requests; }
    const TString& GetHostName() { return SelfHostName; };

protected:
    TClientRequest* CreateClient() override {
        return new TSolverHttpRequest();
    }

private:
    TRequestsHandle Requests;
    TString SelfHostName;
};
